/*
============================================================================
 Name        : ipv6_clss.h
 Author      : Sava Claudiu Gigel
 Email       : csava.dev@gmail.com
 Copyright   : You can redistribute it and/or modify freely, but WITHOUT ANY WARRANTY .
 Description :
============================================================================
*/

#ifndef IPV6_CLSS_H_
#define IPV6_CLSS_H_

#include "ipv4_clss.h"

class ipv6_clss:public ipv4_clss{
public:
	ipv6_clss(){}
	void parsePacket(unsigned char *packet,int lenght);
};

#endif /* IPV6_CLSS_H_ */
