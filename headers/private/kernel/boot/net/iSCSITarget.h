/*
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_NET_ISCSITARGET_H
#define BOOT_NET_ISCSITARGET_H


#include <SupportDefs.h>
#include <util/kernel_cpp.h>

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/vfs.h>
#include <boot/net/NetDefs.h>


#define ISCSI_PORT 3260

class TCPSocket;

class iSCSISession;

struct iscsi_basic_header_segment;

#define kTcpTimeout	10000000ULL


class iSCSIConnection {
public:
	iSCSIConnection();
	virtual ~iSCSIConnection();

	status_t Init(iSCSISession* session);
	status_t Open(ip_addr_t address, uint16 port);
	status_t Login(const char* targetName = NULL, char** targetAlias = NULL);
	status_t Logout(bool closeSession = false);
	status_t GetText(const char* request, size_t requestLength, char** response, size_t* responseLength);
	status_t SendCommand(const void* command, size_t commandSize,
		bool r, bool w, uint32 expectedDataTransferLength,
		void* response, uint32 responseOffset, size_t responseLength);

	bool Active() const { return fConnected; }

private:
	status_t	_ReadResponse(iscsi_basic_header_segment* buffer, bigtime_t timeout = kTcpTimeout);
	status_t	_Read(void* buffer, size_t bufferSize, bigtime_t timeout = kTcpTimeout);

	iSCSISession* fSession;
	TCPSocket* fSocket;
	bool fConnected;
	uint16 fConnectionID;
	uint32 fStatusSequenceNumber;
};

class iSCSISession {
public:
	iSCSISession(const char* targetName = NULL);
	virtual ~iSCSISession();

	status_t Init(ip_addr_t address, uint16 port, char** targetAlias = NULL);
	status_t Close();

	uint32 CommandSequenceNumber() const { return fCommandSequenceNumber; }
	uint32 NextCommandSequenceNumber() { return fCommandSequenceNumber++; }
	iSCSIConnection* Connection() const { return fConnection; }

	bool Active() { return fConnection != NULL && fConnection->Active(); }

private:
	bool fDiscovery;
	const char* fTargetName;
	uint32 fCommandSequenceNumber;
	iSCSIConnection* fConnection;
		// XXX should allow for multiple
};

class iSCSITarget : public Node {
public:
	iSCSITarget();
	virtual ~iSCSITarget();
	status_t Init(ip_addr_t address, uint16 port, const char* targetName);

	virtual ssize_t ReadAt(void* cookie, off_t pos, void* buffer,
		size_t bufferSize);
	virtual ssize_t WriteAt(void* cookie, off_t pos, const void* buffer,
		size_t bufferSize);

	virtual status_t GetName(char* buffer, size_t bufferSize) const;
	virtual off_t Size() const;

	static bool DiscoverTargets(ip_addr_t address, uint16 port,
		NodeList* devicesList);
	static bool _AddDevice(ip_addr_t address, uint16 port,
		const char* targetName, NodeList* devicesList);

private:
	status_t _GetSize();

	iSCSISession* fSession;
	char* fTargetName;
	char* fTargetAlias;
	uint32 fLastLBA;
	uint32 fBlockSize;
};


#endif
