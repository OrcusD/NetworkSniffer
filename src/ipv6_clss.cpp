/*
============================================================================
 Name        : ipv6_clss.cpp
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#include "ipv6_clss.h"
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


void ipv6_clss::parsePacket(unsigned char *packet,int lenght)
{
	struct ethhdr *eth_header;
	struct ip6_hdr *ip_header;
	char ipbuf[INET6_ADDRSTRLEN];
	char str_buff[60];

	eth_header = (struct ethhdr *)packet;
	ip_header = (struct ip6_hdr*)(packet + sizeof(struct ethhdr));

	if(ntohs(eth_header->h_proto) == ETH_P_IPV6){
		//Ethernet type
		sprintf(str_buff,"%.1X%.2Xh",((unsigned char  *)&eth_header->h_proto)[0],((unsigned char  *)&eth_header->h_proto)[1]);
		ethType = string(str_buff);

		//MAC address
		sprintf(str_buff,"%.2X %.2X %.2X %.2X %.2X %.2X", eth_header->h_dest[0] , eth_header->h_dest[1] , eth_header->h_dest[2] , eth_header->h_dest[3] , eth_header->h_dest[4] , eth_header->h_dest[5] );
		MAC_Destination=string(str_buff);
		sprintf(str_buff, "%.2X %.2X %.2X %.2X %.2X %.2X", eth_header->h_source[0] , eth_header->h_source[1] , eth_header->h_source[2] , eth_header->h_source[3] , eth_header->h_source[4] , eth_header->h_source[5]);
		MAC_Source=string(str_buff);

		//IP address
		sprintf(str_buff,"%s",inet_ntop(AF_INET6, &ip_header->ip6_src, ipbuf, sizeof(ipbuf)));
		ipSource=string(str_buff);
		sprintf(str_buff,"%s",inet_ntop(AF_INET6, &ip_header->ip6_dst, ipbuf, sizeof(ipbuf)));
		ipDestination=string(str_buff);

		//IP version
		sprintf(str_buff,"IPv%d",get_ipVersion_int(packet));
		ipVersion=string(str_buff);

		//IP protocol
		ipProtocol=getProtocolName(ip_header->ip6_nxt);
	}
}
