#ifndef _PXE_NETWORK_H
#define _PXE_NETWORK_H
/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>

struct PXE_STRUCT;

class UNDI : public EthernetInterface
{
public:
						UNDI();
	virtual 			~UNDI();

	status_t 			Init();

	ip_addr_t			ServerIPAddress() const;
	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void 		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
	mac_addr_t			fMACAddress;
	bool				fRxFinished;
	PXE_STRUCT *		fPxeData;
};

#endif
