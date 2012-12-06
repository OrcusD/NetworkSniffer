/*
============================================================================
 Name        : NetworkSniffer.cpp
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#include <algorithm>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<linux/if_packet.h>
#include<linux/if_ether.h>
#include<errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <ifaddrs.h>

#include "consoleGUI.h"

void bind2interface(char *dev_name, int sock_id, int protocol);
int openRawSocket(int protocolType);
int getEthInterfaces(vector<string>& interfaceTable);

void bind2interface(char *dev_name, int sock_id, int protocol)
{
	struct ifreq ifc_rq;
	struct sockaddr_ll sock_add;

	bzero(&sock_add, sizeof(sock_add));
	bzero(&ifc_rq, sizeof(ifc_rq));

	strncpy((char *)ifc_rq.ifr_name, dev_name, IFNAMSIZ);
	if((ioctl(sock_id, SIOCGIFINDEX, &ifc_rq)) == -1){
		printf("Fail to get interface index !!!\n");
		exit(-1);
	}

	sock_add.sll_family = AF_PACKET;
	sock_add.sll_ifindex = ifc_rq.ifr_ifindex;
	sock_add.sll_protocol = htons(protocol);

	if((bind(sock_id, (struct sockaddr *)&sock_add, sizeof(sock_add)))== -1){
		perror("Fail to bind socket to interface !!!\n");
		exit(-1);
	}
}

int openRawSocket(int protocolType)
{
	int sock_n=-1;

	sock_n=socket(PF_PACKET, SOCK_RAW, htons(protocolType));	//open RAW socket

	if(sock_n==-1){
		perror("Fail to open socket");
		exit(-1);
	}
	return sock_n;
}


int getEthInterfaces(vector<string>& interfaceTable)
{
	struct ifaddrs *ifaddr, *ifa;
	string tpStr;

	if (getifaddrs(&ifaddr) == -1) {
	   return -1;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	   if (ifa->ifa_addr == NULL){
		   continue;
	   }

	   tpStr=string(ifa->ifa_name);

	   if(!(std::find(interfaceTable.begin(), interfaceTable.end(), tpStr) != interfaceTable.end())) {
		   interfaceTable.push_back(tpStr);
	   }
	}

	freeifaddrs(ifaddr);
	return 0;
}

int main(void)
{
	int socket_id;
	int userInput;
	vector<string> ifrTable; //interfaces table

	getEthInterfaces(ifrTable);
	ifrTable.push_back("Sniff ALL");

	cout<<"\n---------------------------\n";
	cout<<"Choose interface to sniff \n";
	for(int x=0;x<(int)ifrTable.size();++x){
		cout<<" "<<x<<" - "<<" "<<ifrTable[x]<<"\n";
	}
	cout<<"---------------------------\n";

	while(1){
		cout << ": ";
	    cin>>userInput;
	    if(cin.fail()){
	    	cin.clear();
	    	cin.ignore (20, '\n');
			cout << "Please enter a valid integer" << endl;
		}
		else if(userInput>=(int)ifrTable.size()){
			cout << "Please enter a valid option" << endl;
		}
		else{
			break;
		}
	}

	socket_id=openRawSocket(ETH_P_ALL);

	if(ifrTable[userInput]!="Sniff ALL"){ //bind to specific interface
		bind2interface((char*)ifrTable[userInput].c_str(),socket_id,ETH_P_ALL);
	}

	startGui(socket_id,ifrTable[userInput]);

	close(socket_id);
	return 0;
}
