/******************************************************************************
/
/	File:			NetAddress.cpp
/
/   Description:    The Network API.
/
/	Copyright 2002, OpenBeOS Project, All Rights Reserved.
/
******************************************************************************/

#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "NetAddress.h"

BNetAddress::BNetAddress(BMessage *archive)
	:	fStatus(B_OK),
		fName()
{
	// TODO
	fStatus = B_ERROR;
}

BNetAddress::~BNetAddress()
{
}

status_t BNetAddress::Archive(BMessage *into, bool deep) const
{
	// TODO
	return B_ERROR;
}

BArchivable *BNetAddress::Instantiate(BMessage *archive)
{
	// TODO
	return NULL;
}

BNetAddress::BNetAddress(const char * hostname, unsigned short port)
	:	fStatus(B_OK),
		fName()
{
	fStatus = SetTo(hostname, port);
}

BNetAddress::BNetAddress(const sockaddr_in & address)
	:	fStatus(B_OK),
		fName()
{
	fStatus = SetTo(address);
}

BNetAddress::BNetAddress(in_addr address, int port)
	:	fStatus(B_OK),
		fName()
{
	fStatus = SetTo(address, port);
}

BNetAddress::BNetAddress(uint32 address, int port)
	:	fStatus(B_OK),
		fName()
{
	fStatus = SetTo(address, port);
}

BNetAddress::BNetAddress(const BNetAddress & address)
	:	fStatus(B_OK),
		fName(address.fName)
{
}

BNetAddress::BNetAddress(const char * hostname,
						 const char * protocol,
						 const char * service)
	:	fStatus(B_OK),
		fName()
{
	fStatus = SetTo(hostname, protocol, service);
}

BNetAddress & BNetAddress::operator = (const BNetAddress & address)
{
	fName = address.fName;
	return *this;
}

status_t BNetAddress::InitCheck() const
{
	return fStatus;
}


status_t BNetAddress::SetTo(const char * hostname, const char * protocol,
							const char * service)
{
	struct hostent *host = gethostbyname(hostname);
	struct servent *serv = getservbyname(service, protocol);

	if (host != NULL && serv != NULL) {
		fName.sin_family = host->h_addrtype;
		fName.sin_port = serv->s_port;
		fName.sin_addr = *((in_addr *) host->h_addr);
		return B_OK;
	}
	else {
		fName.sin_family = AF_UNSPEC;
		fName.sin_port = 0;
		fName.sin_addr.s_addr = INADDR_ANY;
		return B_ERROR;
	}
}

status_t BNetAddress::SetTo(const char * hostname, unsigned short port)
{
	struct hostent *host;

	if (hostname != NULL && (host = gethostbyname(hostname)) != NULL) {
		fName.sin_family = host->h_addrtype;
		fName.sin_port = htons(port);
		fName.sin_addr = *((in_addr *) host->h_addr);
		return B_OK;
	}
	else {
		fName.sin_family = AF_UNSPEC;
		fName.sin_port = 0;
		fName.sin_addr.s_addr = INADDR_ANY;
		return B_ERROR;
	}
}

status_t BNetAddress::SetTo(const sockaddr_in & address)
{
	fName = address;
	return (fName.sin_family != AF_UNSPEC ? B_OK : B_ERROR);
}

status_t BNetAddress::SetTo(in_addr address, int port)
{
	fName.sin_family = AF_INET;
	fName.sin_port = port;
	fName.sin_addr = address;
	return B_OK;
}

status_t BNetAddress::SetTo(uint32 address, int port)
{
	fName.sin_family = AF_INET;
	fName.sin_port = port;
	fName.sin_addr.s_addr = address;
	return B_OK;
}

status_t BNetAddress::GetAddr(char * hostname = 0, unsigned short * port = 0) const
{
	struct hostent *host = gethostbyaddr((const char *) &fName.sin_addr,
	    sizeof(fName.sin_addr), fName.sin_family);
	if (host != NULL) {
		strcpy(hostname, host->h_name);
		if (port != 0)
			*port = ntohs(fName.sin_port);
		return B_OK;
	}
	return B_ERROR;
}

status_t BNetAddress::GetAddr(sockaddr_in & address) const
{
	address = fName;
	return B_OK;
}

status_t BNetAddress::GetAddr(in_addr & address, unsigned short * port) const
{
	address = fName.sin_addr;
	if (port != 0)
		*port = ntohs(fName.sin_port);
	return B_OK;
}
