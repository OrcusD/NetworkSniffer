/*
============================================================================
 Name        : consoleGUI.cpp
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <string.h>
#include <netinet/ip6.h>

#include "consoleGUI.h"
#include "ipv4_clss.h"
#include "ipv6_clss.h"

//local definition
#define BUFFER_FRAME_SIZE 2000
#define FRAME_COUNT_MAX 65535

//GUI geometry
#define MAX_SCREEN_WIDTH 80
#define MAX_SCREEN_HIGHT 50
#define DATA_VIEWER_SIZE_H 10 //raw data viewer
#define OFFSET_START_FRAME 4
#define COUNT_POS  1
#define VERSION_POS 10
#define PROTOCOL_POS 20
#define SRC_POS 31
#define DST_POS 51
#define PAUSE_POS 24

//system colors
#define BLACK_ON_WHITE  1
#define WHITE_ON_BLUE   2
#define BLACK_ON_YELLOW 3
#define RED_TXT         4
#define GREEN_TXT       5
#define YELLOW_TXT      6
#define BLUE_TXT        7
#define MAGENTA_TXT     8
#define CYAN_TXT        9

pthread_t userEventLoopThr; 								  //user event thread
pthread_t snifferThr;       								  //sniffer thread

pthread_mutex_t pause_mtx = PTHREAD_MUTEX_INITIALIZER;        //pause MUTEX
pthread_cond_t pause_cnd  = PTHREAD_COND_INITIALIZER;         //pause condition
bool pause_c_flg=false;                                       //pause flag
pthread_mutex_t eventLoopExit_mtx = PTHREAD_MUTEX_INITIALIZER;//exit flag MUTEX
bool eventLoopExit_flg=false;                                 //exit flag
pthread_mutex_t data_mtx = PTHREAD_MUTEX_INITIALIZER;         //data access MUTEX
WINDOW *packetFrameWin, *dataViewerWin;                       //display windows
packetFarame frameBuffer[BUFFER_FRAME_SIZE];                  //Ethernet buffer
int availableFrameLines=0;                                    //available write line per window
int buffFrameWriteBoundIdx=0;                                 //next write location
int buffFrameReadBoundIdx=0;                                  //last available frame line in screen
int highlightFrameIdx=0;                                      //frame highlight index line
int highlightDataIdx=0;                                       //data viewer  highlight index line
int vectRawReadBoundIdx=0;                                    //last available raw line in screen
int screen_width=0;
int screen_hight=0;
vector<string> rawDataVector;
unsigned int frameCounter=0;
int pause_gui_flg=false;
string ifrName;

//IP data structure
ipv4_clss dataIpv4;
ipv6_clss dataIpv6;

void resizeHandler(int sig);
void *userEventLoop(void *);
void *snifferThread(void *socket_id);
void startGui(int socket_no);
void destroyWin(void);
void initializeColors(void);
void allocateWin(void);
void updateFrameViewer(int lastFrameEntry,int indexFrameWin);
void bulidNewDataTabel(int inputDataindex);
int updateFrameHighlight(int indexHigPos);
int updateViewerHighlight(int indexHigPos);
void colorizeRawLine(string inputLine,string versionLine ,int lineWritePos, int vectorIndex);
void drawGUI(void);

//------------------------------------------
//Allocate GUI resources
//------------------------------------------
void allocateWin(void)
{
	//create windows
	getmaxyx(stdscr, screen_hight, screen_width);
	if(screen_width>MAX_SCREEN_WIDTH){
		screen_width=MAX_SCREEN_WIDTH;
	}
	if(screen_hight>MAX_SCREEN_HIGHT){
		screen_hight=MAX_SCREEN_HIGHT;
	}

	packetFrameWin=newwin((screen_hight-DATA_VIEWER_SIZE_H),screen_width,0,0);
	dataViewerWin=newwin(DATA_VIEWER_SIZE_H,screen_width,(screen_hight-DATA_VIEWER_SIZE_H),0);
	availableFrameLines=screen_hight-DATA_VIEWER_SIZE_H-OFFSET_START_FRAME-1;
	drawGUI();
}

//------------------------------------------
//Draw GUI interface
//------------------------------------------
void drawGUI(void)
{
		int hight_w;
		int width_w;

		refresh();

		//clear window before start
	    getmaxyx(packetFrameWin, hight_w, width_w);
	    for(int x=0;x<hight_w;++x){
	    	mvwhline(packetFrameWin,x, 0,' ', width_w);
	    }
	    getmaxyx(dataViewerWin, hight_w, width_w);
	    for(int x=0;x<hight_w;++x){
	    	mvwhline(dataViewerWin,x, 0,' ', width_w);
	    }

	    //-----draw windows----------
		//--top window
		box(packetFrameWin,0,0);

		mvwaddstr(packetFrameWin, 1, 1,"(Q)uit (P)auseToggle       ");

		if(pause_gui_flg){
			wattron(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
			mvwaddstr(packetFrameWin, 1, PAUSE_POS," PAUSED ");
			wattroff(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
		}

		wattron(packetFrameWin,COLOR_PAIR(CYAN_TXT)|A_BOLD);
		mvwaddstr(packetFrameWin, 1, (PAUSE_POS+12),ifrName.c_str());
		wattroff(packetFrameWin,COLOR_PAIR(CYAN_TXT)|A_BOLD);

		mvwhline(packetFrameWin, 2, 1, ACS_HLINE, screen_width-2);
		mvwaddstr(packetFrameWin, 2, screen_width-10," (Up) w ");

		wattron(packetFrameWin,COLOR_PAIR(WHITE_ON_BLUE)|A_BOLD);
		mvwhline(packetFrameWin,  3, 1,' ', screen_width-2);
		mvwaddstr(packetFrameWin, 3,COUNT_POS," #");
		mvwaddstr(packetFrameWin, 3,VERSION_POS-1, "| VERSION");
		mvwaddstr(packetFrameWin, 3,PROTOCOL_POS-1,"| PROTOCOL");
		mvwaddstr(packetFrameWin, 3,SRC_POS-1,"| SRC");
		mvwaddstr(packetFrameWin, 3,DST_POS-1,"| DST");
		wattroff(packetFrameWin,COLOR_PAIR(WHITE_ON_BLUE)|A_BOLD);

		//draw grid
		for(int ix=4;ix<(availableFrameLines+OFFSET_START_FRAME);++ix){
			mvwaddstr(packetFrameWin, ix,VERSION_POS-1, "|");
			mvwaddstr(packetFrameWin, ix,PROTOCOL_POS-1,"|");
			mvwaddstr(packetFrameWin, ix,SRC_POS-1,"|");
			mvwaddstr(packetFrameWin, ix,DST_POS-1,"|");
		}

		mvwaddstr(packetFrameWin, (screen_hight-DATA_VIEWER_SIZE_H-1), screen_width-12," (Down) s ");
		wrefresh(packetFrameWin);

		//--bottom window
		box(dataViewerWin,0,0);
		mvwaddstr(dataViewerWin, 0, screen_width-12," ( Up ) a ");
		mvwaddstr(dataViewerWin, DATA_VIEWER_SIZE_H-1, screen_width-12," (Down) d ");
		wrefresh(dataViewerWin);
}

//------------------------------------------
//destroy GUI
//------------------------------------------
void destroyWin(void)
{
	delwin(packetFrameWin);
	delwin(dataViewerWin);
}


//------------------------------------------
//define system colors type
//------------------------------------------
void initializeColors(void)
{	//pair system color
	init_pair(BLACK_ON_WHITE,COLOR_BLACK,COLOR_WHITE);
	init_pair(WHITE_ON_BLUE,COLOR_WHITE,COLOR_BLUE);
	init_pair(BLACK_ON_YELLOW,COLOR_BLACK,COLOR_YELLOW);
	init_pair(RED_TXT,    COLOR_RED,-1);
	init_pair(GREEN_TXT,  COLOR_GREEN,-1);
	init_pair(YELLOW_TXT, COLOR_YELLOW,-1);
	init_pair(BLUE_TXT,   COLOR_BLUE,-1);
	init_pair(MAGENTA_TXT,COLOR_MAGENTA,-1);
	init_pair(CYAN_TXT,   COLOR_CYAN,-1);
}

//------------------------------------------
//main function ->call this to start GUI
//------------------------------------------
void startGui(int socket_no, string interfaceName)
{
	ifrName="Interface: "+interfaceName;
	signal(SIGWINCH, resizeHandler); //signal trigger -->terminal change size

 	initscr();            //start NCURSSES
	curs_set(0);          //remove cursor from screen
	start_color();        //start color mode
	use_default_colors(); //keep current color of host console.
 	cbreak();             //immediately acquire each keystroke
	keypad(stdscr, true); //enable detection of function keys
	noecho();             //do not echo user keystrokes

	initializeColors();
	allocateWin();
	refresh();

	pthread_create(&userEventLoopThr, NULL, &userEventLoop,NULL);
	pthread_create(&snifferThr, NULL, &snifferThread,(void *)&socket_no);
	pthread_join(snifferThr,NULL);
	pthread_join(userEventLoopThr,NULL);

	destroyWin();
	endwin();
}

//------------------------------------------
//sniffer thread
//------------------------------------------
void *snifferThread(void *socket_id)
{
	int socket_no;
	unsigned char packet_buffer[65536];
	int len, ipV;;
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);
	char str_buff[10];

	socket_no=*static_cast<int*>(socket_id);

	while(1)
	{
		if((len = recvfrom(socket_no,packet_buffer,65536,0,(struct sockaddr*)&ss, &ss_len)) == -1)
		{
			perror("Recv from returned -1: ");
			pthread_mutex_lock(&eventLoopExit_mtx);
				eventLoopExit_flg=true; //set exit check point
			pthread_mutex_unlock(&eventLoopExit_mtx);
			return NULL;
		}
		else
		{
			pthread_mutex_lock( &pause_mtx);//pause check point
				while(pause_c_flg){
					pthread_cond_wait(&pause_cnd, &pause_mtx);}
			pthread_mutex_unlock( &pause_mtx);

			ipV=dataIpv4.get_ipVersion_int(packet_buffer);

			if (ipV==4) {// version 4
				dataIpv4.parsePacket(packet_buffer,len);
				pthread_mutex_lock(&data_mtx);
					//update buffer
					frameBuffer[buffFrameWriteBoundIdx].macSRC=dataIpv4.get_MAC_Source();
					frameBuffer[buffFrameWriteBoundIdx].macDST=dataIpv4.get_MAC_Destination();
					frameBuffer[buffFrameWriteBoundIdx].ipVersion=dataIpv4.get_ipVersion_str();
					frameBuffer[buffFrameWriteBoundIdx].protocolName=dataIpv4.get_ipProtocol();
					frameBuffer[buffFrameWriteBoundIdx].ethType=dataIpv4.get_ethType();
					frameBuffer[buffFrameWriteBoundIdx].ipSRC=dataIpv4.get_ipSource();
					frameBuffer[buffFrameWriteBoundIdx].ipDST=dataIpv4.get_ipDestination();
					frameBuffer[buffFrameWriteBoundIdx].frameCount=frameCounter;
					sprintf(str_buff,"%d",len);
					frameBuffer[buffFrameWriteBoundIdx].byteSize=string(str_buff);
					dataIpv4.get_rawData( frameBuffer[buffFrameWriteBoundIdx].raw_data,packet_buffer,len);
					//write data on console
					updateFrameViewer(buffFrameWriteBoundIdx,0);

					//increment buffer ;index
					buffFrameReadBoundIdx=buffFrameWriteBoundIdx;
					buffFrameWriteBoundIdx=(buffFrameWriteBoundIdx+1)%BUFFER_FRAME_SIZE;
					frameCounter=(frameCounter+1)%FRAME_COUNT_MAX;
					//reset highlight index
					highlightFrameIdx=0;
				pthread_mutex_unlock(&data_mtx);
			}
			else if(ipV==6){// version 6
				dataIpv6.parsePacket(packet_buffer,len);
				pthread_mutex_lock(&data_mtx);
					//update buffer
					frameBuffer[buffFrameWriteBoundIdx].macSRC=dataIpv6.get_MAC_Source();
					frameBuffer[buffFrameWriteBoundIdx].macDST=dataIpv6.get_MAC_Destination();
					frameBuffer[buffFrameWriteBoundIdx].ipVersion=dataIpv6.get_ipVersion_str();
					frameBuffer[buffFrameWriteBoundIdx].protocolName=dataIpv6.get_ipProtocol();
					frameBuffer[buffFrameWriteBoundIdx].ethType=dataIpv6.get_ethType();
					frameBuffer[buffFrameWriteBoundIdx].ipSRC=dataIpv6.get_ipSource();
					frameBuffer[buffFrameWriteBoundIdx].ipDST=dataIpv6.get_ipDestination();
					frameBuffer[buffFrameWriteBoundIdx].frameCount=frameCounter;
					sprintf(str_buff,"%d",len);
					frameBuffer[buffFrameWriteBoundIdx].byteSize=string(str_buff);
					dataIpv6.get_rawData( frameBuffer[buffFrameWriteBoundIdx].raw_data,packet_buffer,len);
					//write data on console
					updateFrameViewer(buffFrameWriteBoundIdx,0);

					//increment buffer index
					buffFrameReadBoundIdx=buffFrameWriteBoundIdx;
					buffFrameWriteBoundIdx=(buffFrameWriteBoundIdx+1)%BUFFER_FRAME_SIZE;
					frameCounter=(frameCounter+1)%FRAME_COUNT_MAX;
					//reset highlight index
					highlightFrameIdx=0;
				pthread_mutex_unlock(&data_mtx);

			}
			else{ /*OTHER*/ }
		}
	}
	return NULL;
}

//------------------------------------------
// handler user key event
//------------------------------------------
void *userEventLoop(void *)
{
    int c=0,resVal;
    bool exit_cond;
	nodelay(stdscr, true);//not wait for user keystroke

    while((c!='q') && (c!='Q')){

		pthread_mutex_lock(&eventLoopExit_mtx);
			exit_cond=eventLoopExit_flg;
		pthread_mutex_unlock(&eventLoopExit_mtx);

		if(exit_cond==true){ //something is wrong with the sniffer, need to exit
			return NULL;
		}

		c=getch();
		usleep(30000); //milliseconds delays  because program does not wait in getch
		switch(c){
			case 'p': //pause toggle
			case 'P':
				if(pause_gui_flg==false){
					pthread_mutex_lock(&pause_mtx);
						pause_c_flg=true;
					pthread_mutex_unlock( &pause_mtx);

					pthread_mutex_lock(&data_mtx);
						refresh();
						wattron(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
						mvwaddstr(packetFrameWin, 1, PAUSE_POS," PAUSED ");
						wattroff(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
						wrefresh(packetFrameWin);
						pause_gui_flg=true;
					pthread_mutex_unlock(&data_mtx);
				}
				else{
					pthread_mutex_lock(&pause_mtx);
						pause_c_flg=false;
						pthread_cond_signal(&pause_cnd);
					pthread_mutex_unlock( &pause_mtx);

					pthread_mutex_lock(&data_mtx);
						refresh();
						mvwaddstr(packetFrameWin, 1, PAUSE_POS,"          ");
						wrefresh(packetFrameWin);
						pause_gui_flg=false;
					pthread_mutex_unlock(&data_mtx);
				}
			break;

			case'w': //move highlight line up in frame window
			case'W':
				pthread_mutex_lock(&data_mtx);
					resVal=updateFrameHighlight(--highlightFrameIdx);
					if (resVal<0){
						++highlightFrameIdx; //fail to highlight line ->reset index
					}
					else{
						highlightFrameIdx=resVal; //get new highlight line position
					}
				pthread_mutex_unlock(&data_mtx);
			break;

			case's': //move highlight line down in frame window
			case'S':
				pthread_mutex_lock(&data_mtx);
					resVal=updateFrameHighlight(++highlightFrameIdx);
					if (resVal<0){
						--highlightFrameIdx; //fail to move cursor ->reset index
					}
					else{
						highlightFrameIdx=resVal; //get new highlight line position
					}
				pthread_mutex_unlock(&data_mtx);
			break;


			case'a':
			case'A':
				pthread_mutex_lock(&data_mtx);
					resVal=updateViewerHighlight(--highlightDataIdx);
					if (resVal<0){
						++highlightDataIdx; //fail to highlight line ->reset index
					}
					else{
						highlightDataIdx=resVal; //get new highlight line position
					}
					pthread_mutex_unlock(&data_mtx);
				pthread_mutex_unlock(&data_mtx);
			break;

			case'd':
			case'D':
				pthread_mutex_lock(&data_mtx);
					resVal=updateViewerHighlight(++highlightDataIdx);
					if (resVal<0){
						--highlightDataIdx; //fail to move cursor ->reset index
					}
					else{
						highlightDataIdx=resVal; //get new highlight line position
					}
				pthread_mutex_unlock(&data_mtx);
			break;

			default:
			break;
		}
    }
  pthread_cancel(snifferThr);
  return NULL;
}

//------------------------------------------
//called when terminal size change
//------------------------------------------
void resizeHandler(int sig)
{
	pthread_mutex_lock(&data_mtx);
		endwin();//---]-->force NCURSES initialization
		refresh();//--]
		clear();  // clear entire screen
		destroyWin();
		allocateWin();
	pthread_mutex_unlock(&data_mtx);
}

//------------------------------------------
//update frame viewer
//------------------------------------------
void updateFrameViewer(int lastFrameEntry,int idxHilightWin)
{
	int PrintIdx=0;//print position from frame buffer
	int indexWriteLine=0;//position index to write line frame

	PrintIdx=lastFrameEntry-availableFrameLines;//get first print index

	if(PrintIdx<0){//because is an is circular buffer
		PrintIdx*=-1;
		PrintIdx=BUFFER_FRAME_SIZE-PrintIdx;
	}

	for(int i=0;i<availableFrameLines;++i){//print as long it fit in the window
		if(!frameBuffer[PrintIdx].macSRC.empty()){//not empty

			if(indexWriteLine==idxHilightWin){//highlight on if match
				wattron(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
			}

			mvwhline( packetFrameWin, OFFSET_START_FRAME+indexWriteLine, 1,' ', screen_width-2);
			mvwprintw(packetFrameWin, OFFSET_START_FRAME+indexWriteLine,COUNT_POS, " %d",frameBuffer[PrintIdx].frameCount);
			mvwprintw(packetFrameWin, OFFSET_START_FRAME+indexWriteLine,VERSION_POS-1, "| %s",frameBuffer[PrintIdx].ipVersion.c_str());
			mvwprintw(packetFrameWin, OFFSET_START_FRAME+indexWriteLine,PROTOCOL_POS-1,"| %s",frameBuffer[PrintIdx].protocolName.c_str());
			mvwprintw(packetFrameWin, OFFSET_START_FRAME+indexWriteLine,SRC_POS-1,"| %s",frameBuffer[PrintIdx].macSRC.c_str());
			mvwprintw(packetFrameWin, OFFSET_START_FRAME+indexWriteLine,DST_POS-1,"| %s",frameBuffer[PrintIdx].macDST.c_str());

			if(indexWriteLine==idxHilightWin){//highlight off if match
				wattroff(packetFrameWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
			}

			++indexWriteLine;
		}

		//update print index
		PrintIdx=(PrintIdx+1)%BUFFER_FRAME_SIZE;
	}
	wrefresh(packetFrameWin);
}

//------------------------------------------
//print raw data on terminal
//------------------------------------------
void updateRawData(int lasEntry,int idxHilight)
{
	int PrintIdx=0,ix=0;
	PrintIdx=lasEntry-(DATA_VIEWER_SIZE_H-2);//get first print index

	if(PrintIdx<0){//less lines than window size
		PrintIdx=0;//start from first index
	}

	//print data on terminal
	for(ix=0;(ix<(DATA_VIEWER_SIZE_H-2) && ix<(int)rawDataVector.size());++ix){
		if(ix==idxHilight){//enable highlight if match
			wattron(dataViewerWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
		}

		mvwhline(dataViewerWin, ix+1, 1,' ', screen_width-2);

		if( ((PrintIdx+ix)<4) && (ix!=idxHilight) ){
			colorizeRawLine(rawDataVector[PrintIdx+ix],rawDataVector[0],ix+1,PrintIdx+ix);
		}
		else{
			mvwaddstr(dataViewerWin, ix+1,1,rawDataVector[PrintIdx+ix].c_str());
		}

		if(ix==idxHilight){//disable highlight if match
			wattroff(dataViewerWin,COLOR_PAIR(BLACK_ON_YELLOW)|A_BOLD);
		}
	}

	//clear garbage data
	while(ix<(DATA_VIEWER_SIZE_H-2) ){
		mvwhline(dataViewerWin, ix+1, 1,' ', screen_width-2);
		++ix;
	}

	wrefresh(dataViewerWin);
}

//------------------------------------------
//build new raw data table
//------------------------------------------
void bulidNewDataTabel(int inputDataIndex)
{
	char str_buff[10];
	string cntFrame;
	sprintf(str_buff,"%d",frameBuffer[inputDataIndex].frameCount);
	cntFrame=string(str_buff);

	//update data
	rawDataVector.clear();
	if(frameBuffer[inputDataIndex].ipVersion=="IPv4"){//IP version 4
		rawDataVector.push_back("#: "+cntFrame+"  Type: "+frameBuffer[inputDataIndex].ethType+"  Version: "+frameBuffer[inputDataIndex].ipVersion+" Protocol: "+frameBuffer[inputDataIndex].protocolName+"  Bytes: "+frameBuffer[inputDataIndex].byteSize);
		rawDataVector.push_back("MAC_SRC: "+frameBuffer[inputDataIndex].macSRC+" MAC_DST: "+frameBuffer[inputDataIndex].macDST);
		rawDataVector.push_back("IP_SRC: "+frameBuffer[inputDataIndex].ipSRC+" IP_DST: "+frameBuffer[inputDataIndex].ipDST);
		rawDataVector.insert(rawDataVector.end(), frameBuffer[inputDataIndex].raw_data.begin(), frameBuffer[inputDataIndex].raw_data.end());
	}
	else if(frameBuffer[inputDataIndex].ipVersion=="IPv6"){//IP version 6
		rawDataVector.push_back("#: "+cntFrame+"  Type: "+frameBuffer[inputDataIndex].ethType+"  Version: "+frameBuffer[inputDataIndex].ipVersion+" Protocol: "+frameBuffer[inputDataIndex].protocolName+"  Bytes: "+frameBuffer[inputDataIndex].byteSize);
		rawDataVector.push_back("MAC_SRC: "+frameBuffer[inputDataIndex].macSRC+" MAC_DST: "+frameBuffer[inputDataIndex].macDST);
		rawDataVector.push_back("IP_SRC: "+frameBuffer[inputDataIndex].ipSRC);
		rawDataVector.push_back("IP_DST: "+frameBuffer[inputDataIndex].ipDST);
		rawDataVector.insert(rawDataVector.end(), frameBuffer[inputDataIndex].raw_data.begin(), frameBuffer[inputDataIndex].raw_data.end());
	}
	else{//other
		rawDataVector.insert(rawDataVector.end(), frameBuffer[inputDataIndex].raw_data.begin(), frameBuffer[inputDataIndex].raw_data.end());
	}

	//reset index
	highlightDataIdx=0;
	vectRawReadBoundIdx = ((rawDataVector.size()<(DATA_VIEWER_SIZE_H-2))?rawDataVector.size():(DATA_VIEWER_SIZE_H-2)); //smallest  one

	//print on terminal now
	updateRawData(vectRawReadBoundIdx,highlightDataIdx);
}

//------------------------------------------
//move highlight line inside frame window
//------------------------------------------
int updateFrameHighlight(int indexHigPos)
{
	int retVal; // negative if out of range
	int prevIdx, buildIndexData;
	int lowBoundaryIndex=0; //first item to print in window
	int line2write=availableFrameLines;

	prevIdx=buffFrameReadBoundIdx;
	if(indexHigPos<0){//need to move page up by one element
		indexHigPos=0;//set highlight position to first line
		--buffFrameReadBoundIdx;
		if(buffFrameReadBoundIdx<0){//because is a circular buffer
			buffFrameReadBoundIdx*=-1;
			buffFrameReadBoundIdx=BUFFER_FRAME_SIZE-buffFrameReadBoundIdx;
		}
	}
	else if(indexHigPos>=availableFrameLines){//need to move page down by one element
		buffFrameReadBoundIdx = (buffFrameReadBoundIdx+1)%BUFFER_FRAME_SIZE;
		indexHigPos=availableFrameLines-1;//set highlight position to last line
	}
	else{}

	lowBoundaryIndex=buffFrameReadBoundIdx-availableFrameLines;//get boundary index
	if(lowBoundaryIndex<0){//because is an is circular buffer
		lowBoundaryIndex*=-1;
		lowBoundaryIndex=BUFFER_FRAME_SIZE-lowBoundaryIndex;
	}

	for(int ix=0;ix<availableFrameLines;++ix){ //move low boundary up till you find some valid data in the range of available lines
	 if(frameBuffer[lowBoundaryIndex].macSRC.empty()){//empty data found
		 lowBoundaryIndex=(lowBoundaryIndex+1)%BUFFER_FRAME_SIZE;// got next position
		 buffFrameReadBoundIdx=prevIdx;//restore read index
		 --line2write;
	 }
	}

	//get highlight item position
	buildIndexData=(lowBoundaryIndex+indexHigPos)%BUFFER_FRAME_SIZE;

	if((buffFrameReadBoundIdx==buffFrameWriteBoundIdx)||(lowBoundaryIndex==buffFrameWriteBoundIdx)||(indexHigPos>=line2write)){//request out of range
		retVal=-1;
		buffFrameReadBoundIdx=prevIdx;//restore value
	}
	else{//refresh screen data
		retVal=indexHigPos;
		updateFrameViewer(buffFrameReadBoundIdx,indexHigPos);//update screen to move highlight line
		bulidNewDataTabel(buildIndexData); //get new data from the new highlight element
	}
	return retVal;
}

//------------------------------------------
//move highlight line inside raw data window
//------------------------------------------
int updateViewerHighlight(int indexHigPos)
{
	int retVal,prevIdx;
	prevIdx=vectRawReadBoundIdx;
	int vectSize=(int)rawDataVector.size();

	if(indexHigPos<0){//need to move page up by one element
		--vectRawReadBoundIdx;
		indexHigPos=0;
	}
	else if(indexHigPos>=(DATA_VIEWER_SIZE_H-2)){//need to move page down by one element
		++vectRawReadBoundIdx;
		indexHigPos=DATA_VIEWER_SIZE_H-3;
	}
	else{}

	 if((vectRawReadBoundIdx<0)||(vectRawReadBoundIdx>vectSize) || (indexHigPos>=vectSize) ){//out of range
		 vectRawReadBoundIdx=prevIdx;
		 retVal=-1;
	 }else{ // valid range , print it
		 retVal=indexHigPos;
		 updateRawData(vectRawReadBoundIdx,indexHigPos);
	 }
	return retVal;
}

//------------------------------------------
//Colonize lines in raw data view
//-----------------------------------------
void colorizeRawLine(string inputLine,string versionLine ,int lineWritePos, int vectorIndex)
{
	int ipVersion=-1;
	size_t foundIndex1;
	size_t foundIndex2;
	string str_tmp1;
	string str_tmp2;
	int strLen1;
	int strLen2;

	if(versionLine.find("IPv4")!=string::npos){
		ipVersion=4;
	}
	else if((versionLine.find("IPv6")!=string::npos))
	{
		ipVersion=6;
	}
	else{
		vectorIndex=-1;//go to default case, version not found
	}

	switch(vectorIndex){
		case 0: //#: 3671  Type: 800h  Version: IPv4 Protocol: TCP  Bytes: 6274
			foundIndex1=inputLine.find("#:");
			foundIndex2=inputLine.find("Type:");
			strLen1=strlen("#:");
			strLen2=strlen("Type:")-1;
			str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"#:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(RED_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(RED_TXT)|A_BOLD);

			foundIndex1=inputLine.find("Type:");
			foundIndex2=inputLine.find("Version:");
			strLen1=strlen("Type:");
			strLen2=strlen("Version:");
			str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"Type:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);

			foundIndex1=inputLine.find("Version:");
			foundIndex2=inputLine.find("Protocol:");
			strLen1=strlen("Version:");
			strLen2=strlen("Protocol:");
			str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"Version:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);

			foundIndex1=inputLine.find("Protocol:");
			foundIndex2=inputLine.find("Bytes:");
			strLen1=strlen("Protocol:");
			strLen2=strlen("Bytes:");
			str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"Protocol:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(YELLOW_TXT)|A_BOLD);

			foundIndex1=inputLine.find("Bytes:");
			strLen1=strlen("Bytes:");
			str_tmp1 = inputLine.substr(foundIndex2+strLen1);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"Bytes:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
		break;

		case 1: //MAC_SRC: 	NN NN NN NN NN NN MAC_DST: NN NN NN NN NN NN
			foundIndex1=inputLine.find("MAC_SRC:");
			foundIndex2=inputLine.find("MAC_DST:");
			strLen1=strlen("MAC_SRC:");
			strLen2=strlen("MAC_DST:");;
			str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);
			str_tmp2 = inputLine.substr(foundIndex2+strLen2);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"MAC_SRC:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);

			wattron(dataViewerWin,A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex2)+1,"MAC_DST:");
			wattroff(dataViewerWin,A_BOLD);

			wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
			mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex2)+1+strLen2,str_tmp2.c_str());
			wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
		break;

		case 2: //if IPv4--> IP_SRC: NN.NN.NNN.NN IP_DST: NNN.NNN.N.NN  if IpV6-->IP_SRC: NNNN:NNNN:NNNN ...
				if(ipVersion==4){
					foundIndex1=inputLine.find("IP_SRC:");
					foundIndex2=inputLine.find("IP_DST:");
					strLen1=strlen("IP_SRC:");
					strLen2=strlen("IP_DST:");;
					str_tmp1 = inputLine.substr(foundIndex1+strLen1 , foundIndex2-strLen2);
					str_tmp2 = inputLine.substr(foundIndex2+strLen2);

					wattron(dataViewerWin,A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"IP_SRC:");
					wattroff(dataViewerWin,A_BOLD);

					wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
					wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);

					wattron(dataViewerWin,A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex2)+1,"IP_DST:");
					wattroff(dataViewerWin,A_BOLD);

					wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex2)+1+strLen2,str_tmp2.c_str());
					wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
				}
				else {
					foundIndex1=inputLine.find("IP_SRC:");
					strLen1=strlen("IP_SRC:");
					str_tmp1 = inputLine.substr(foundIndex1+strLen1);

					wattron(dataViewerWin,A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"IP_SRC:");
					wattroff(dataViewerWin,A_BOLD);

					wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
					wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
				}
		break;

		case 3: //if IPv4> first raw data   if IpV6>IP_DST: NNNN:NNNN:NNNN ...
				if(ipVersion==4){
					mvwaddstr(dataViewerWin, lineWritePos,1,inputLine.c_str());
				}else{
					foundIndex1=inputLine.find("IP_DST:");
					strLen1=strlen("IP_DST:");
					str_tmp1 = inputLine.substr(foundIndex1+strLen1);

					wattron(dataViewerWin,A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1,"IP_DST:");
					wattroff(dataViewerWin,A_BOLD);

					wattron(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
					mvwaddstr(dataViewerWin, lineWritePos,((int)foundIndex1)+1+strLen1,str_tmp1.c_str());
					wattroff(dataViewerWin,COLOR_PAIR(GREEN_TXT)|A_BOLD);
				}
		break;

		default:
			mvwaddstr(dataViewerWin, lineWritePos,1,inputLine.c_str());
		break;
	}
}
