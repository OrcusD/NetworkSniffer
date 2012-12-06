/*
============================================================================
 Name        : ipv4_clss.cpp
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#include "ipv4_clss.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <features.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip6.h>


void ipv4_clss::parsePacket(unsigned char *packet,int lenght)
{
	struct ethhdr *eth_header;
	struct iphdr *ip_header;
	char ipbuf[INET_ADDRSTRLEN];
	char str_buff[50];

	eth_header = (struct ethhdr *)packet;
	ip_header = (struct iphdr*)(packet + sizeof(struct ethhdr));

	if(ntohs(eth_header->h_proto) == ETH_P_IP){
		//Ethernet type
		sprintf(str_buff,"%.1X%.2Xh",((unsigned char  *)&eth_header->h_proto)[0],((unsigned char  *)&eth_header->h_proto)[1]);
		ethType = string(str_buff);

		//MAC address
		sprintf(str_buff,"%.2X %.2X %.2X %.2X %.2X %.2X", eth_header->h_dest[0] , eth_header->h_dest[1] , eth_header->h_dest[2] , eth_header->h_dest[3] , eth_header->h_dest[4] , eth_header->h_dest[5] );
		MAC_Destination=string(str_buff);
		sprintf(str_buff, "%.2X %.2X %.2X %.2X %.2X %.2X", eth_header->h_source[0] , eth_header->h_source[1] , eth_header->h_source[2] , eth_header->h_source[3] , eth_header->h_source[4] , eth_header->h_source[5]);
		MAC_Source=string(str_buff);

		//IP address
		sprintf(str_buff,"%s",inet_ntop(AF_INET, &ip_header->saddr, ipbuf, sizeof(ipbuf)));
		ipSource=string(str_buff);
		sprintf(str_buff,"%s",inet_ntop(AF_INET, &ip_header->daddr, ipbuf, sizeof(ipbuf)));
		ipDestination=string(str_buff);

		//IP version
		sprintf(str_buff,"IPv%d",ip_header->version);
		ipVersion=string(str_buff);

		//IP protocol
		ipProtocol=getProtocolName(ip_header->protocol);
	}
}

string ipv4_clss::getProtocolName(int id){
string protName="???";
switch (id)
{
	case IPPROTO_ICMP:
		protName="ICMP";
	break;

	case IPPROTO_TCP:
		protName="TCP";
	break;

	case IPPROTO_UDP:
		protName="UDP";
	break;

	case IPPROTO_ICMPV6:
		protName="ICMPv6";
	break;

	default: //Other protocol
		protName="OTHER";
	break;
}
return protName;
}

int ipv4_clss::get_ipVersion_int(unsigned char *packet)
{
	struct iphdr *ipHead = (struct iphdr *)(packet  + sizeof(struct ethhdr) );
	return (int)ipHead->version;
}

void ipv4_clss::get_rawData(vector<string>& data2Update,unsigned char *packet,int lenght)
{ //TODO: do this in multithreading, it could take too much time for long data
	string hexData, charData;
	char strBuff[5];
	data2Update.clear();
	for(int ix=0;ix<lenght;++ix){
		if(ix!=0 && ix%16==0){//one line complete, push to data vector
			data2Update.push_back(hexData+"    "+charData);
			hexData.clear();
			charData.clear();
		}

		//hex format
		sprintf(strBuff,"%.2X ",(unsigned char)packet[ix]);
		hexData+=string(strBuff);

		//char format
		if(packet[ix]>31 && packet[ix]<127){//is an alphabet char
		  sprintf(strBuff,"%c",(unsigned char)packet[ix]);
		  charData+=string(strBuff);
		}
		else{//replace invalid char with "."
		  charData+=".";
		}

		if (ix==lenght-1){//last line
			for(int jx=0;jx<(15-ix%16);++jx){ //fill with space remaining unoccupied fields
				hexData+="   ";
			}
			data2Update.push_back(hexData+"    "+charData);
		}
	}
}
