/*
============================================================================
 Name        : ipv4_clss.h
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#ifndef IPV4_CLSS_H_
#define IPV4_CLSS_H_

#include <string>
#include <iostream>
#include <vector>
using namespace std;

class ipv4_clss {
protected:
	string ipSource;
	string ipDestination;
	string MAC_Source;
	string MAC_Destination;
	string ethType;    //800h for IP ...
	string ipProtocol; //UDP/TCP/ICMP ...
	string ipVersion;  //IPv4/IPv6
	string getProtocolName(int id);
public:
	ipv4_clss(){}
    virtual void parsePacket(unsigned char *packet,int lenght);
	void  get_rawData(vector<string>& data2Update,unsigned char *packet,int lenght);
	string get_ipSource(void){return ipSource;}
	string get_ipDestination(void){return ipDestination;}
	string get_MAC_Source(void){return MAC_Source;}
	string get_MAC_Destination(void){return MAC_Destination;}
	string get_ethType(void){return ethType;}
	string get_ipProtocol(void){return ipProtocol;}
	string get_ipVersion_str(void){return ipVersion;}
	int get_ipVersion_int(unsigned char *packet);
};
#endif /* IPV4_CLSS_H_ */
