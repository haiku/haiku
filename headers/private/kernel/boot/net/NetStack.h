/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BOOT_NET_STACK_H
#define _BOOT_NET_STACK_H

#include <SupportDefs.h>

class EthernetInterface;
class EthernetService;
class ARPService;
class IPService;
class UDPService;
class TCPService;


class NetStack {
private:
	NetStack();
	~NetStack();

	status_t Init();

public:
	static status_t CreateDefault();
	static NetStack *Default();

	status_t AddEthernetInterface(EthernetInterface *interface);

	EthernetInterface *GetEthernetInterface() const
		{ return fEthernetInterface; }
	EthernetService *GetEthernetService() const	{ return fEthernetService; }
	ARPService *GetARPService() const			{ return fARPService; }
	IPService *GetIPService() const				{ return fIPService; }
	UDPService *GetUDPService() const			{ return fUDPService; }
	TCPService *GetTCPService() const			{ return fTCPService; }

private:
	static NetStack		*sNetStack;

	EthernetInterface	*fEthernetInterface;
	EthernetService		*fEthernetService;
	ARPService			*fARPService;
	IPService			*fIPService;
	UDPService			*fUDPService;
	TCPService			*fTCPService;
};


// net_stack_init() creates the NetStack and calls platform_net_stack_init()
// afterwards, which is supposed to add network interfaces.
status_t net_stack_init();
status_t platform_net_stack_init();


#endif	// _BOOT_NET_STACK_H
