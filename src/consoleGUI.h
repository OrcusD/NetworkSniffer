/*
============================================================================
 Name        : consoleGUI.h
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#ifndef CONSOLEGUI_H_
#define CONSOLEGUI_H_

#include <vector>
#include <iostream>
using namespace std;

class packetFarame{
public:
	unsigned int frameCount;
	string macSRC;
	string macDST;
	string ipVersion;         //IPv4 /IPv6
	string protocolName;      //UDP/TCP/ICMP ...
	vector<string> raw_data;  //raw data
	string ethType;           //800h for IP  ...
	string ipSRC;
	string ipDST;
	string byteSize;
	packetFarame(){}
	~packetFarame(){}
};

void startGui(int socket_no, string interfaceName);
#endif /* CONSOLEGUI_H_ */
