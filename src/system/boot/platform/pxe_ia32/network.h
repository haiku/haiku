#ifndef _PXE_NETWORK_H
#define _PXE_NETWORK_H
/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Copyright 2005-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>

struct PXE_STRUCT;


class PXEService {
protected:
								PXEService();
								~PXEService();

			status_t			Init();

public:
			mac_addr_t			MACAddress() const		{ return fMACAddress; }
			ip_addr_t			IPAddress() const		{ return fClientIP; }
			ip_addr_t			ServerIPAddress() const	{ return fServerIP; }
			const char*			RootPath() const		{ return fRootPath; }

protected:
			PXE_STRUCT*			fPxeData;
			ip_addr_t			fClientIP;
			ip_addr_t			fServerIP;
			mac_addr_t			fMACAddress;
			char*				fRootPath;
};


class UNDI : public EthernetInterface, public PXEService
{
public:
						UNDI();
	virtual 			~UNDI();

	status_t 			Init();

	virtual mac_addr_t	MACAddress() const;

	virtual	void *		AllocateSendReceiveBuffer(size_t size);
	virtual	void 		FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t		Send(const void *buffer, size_t size);
	virtual ssize_t		Receive(void *buffer, size_t size);

private:
	bool				fRxFinished;
	PXE_STRUCT *		fPxeData;
};


class TFTP : public PXEService {
public:
								TFTP();
								~TFTP();

			status_t			Init();

			uint16				ServerPort() const;

			status_t			ReceiveFile(const char* fileName,
									uint8** data, size_t* size);

private:
			status_t			_Close();
};

#endif
