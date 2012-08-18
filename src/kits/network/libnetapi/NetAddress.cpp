/*
 * Copyright 2002-2006,2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Scott T. Mansfield, thephantom@mac.com
 *		Oliver Tappe, zooey@hirschkaefer.de
 */

/*!
	NetAddress.cpp -- Implementation of the BNetAddress class.
	Remarks:
	 * In all accessors, non-struct output values are converted from network to
	   host byte order.
	 * In all mutators, non-struct input values are converted from host to
	   network byte order.
	 * No trouts were harmed during the development of this class.
*/

#include <r5_compatibility.h>

#include <ByteOrder.h>
#include <NetAddress.h>
#include <Message.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <new>
#include <string.h>


BNetAddress::BNetAddress(const char* hostname, unsigned short port)
	:
	fInit(B_NO_INIT)
{
	SetTo(hostname, port);
}


BNetAddress::BNetAddress(const struct sockaddr_in& addr)
	:
	fInit(B_NO_INIT)
{
	SetTo(addr);
}


BNetAddress::BNetAddress(in_addr addr, int port)
	:
	fInit(B_NO_INIT)
{
	SetTo(addr, port);
}


BNetAddress::BNetAddress(uint32 addr, int port)
	:
	fInit(B_NO_INIT)
{
	SetTo(addr, port);
}


BNetAddress::BNetAddress(const char* hostname, const char* protocol,
	const char* service)
	:
	fInit(B_NO_INIT)
{
	SetTo(hostname, protocol, service);
}


BNetAddress::BNetAddress(const BNetAddress& other)
{
	*this = other;
}


BNetAddress::BNetAddress(BMessage* archive)
{
	int16 int16value;
	if (archive->FindInt16("bnaddr_family", &int16value) != B_OK)
		return;

	fFamily = int16value;

	if (archive->FindInt16("bnaddr_port", &int16value) != B_OK)
		return;

	fPort = int16value;

	if (archive->FindInt32("bnaddr_addr", &fAddress) != B_OK)
		return;

	fInit = B_OK;
}


BNetAddress::~BNetAddress()
{
}


BNetAddress&
BNetAddress::operator=(const BNetAddress& other)
{
	fInit = other.fInit;
	fFamily = other.fFamily;
	fPort = other.fPort;
	fAddress = other.fAddress;

	return *this;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Class accessor.
 *
 * Output parameters:
 *		hostname:		Host name associated with this instance (default: NULL).
 *						In this particular implementation, hostname will be an
 *						ASCII-fied representation of an IP address.
 *
 *		port:			Port number associated with this instance
 *						(default: NULL).  Will be converted to host byte order
 *						here, so it is not necessary to call ntohs() after
 *						calling this method.
 *
 * Returns:
 *	B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *		Hostname and/or port can be NULL; although having them both NULL would
 *		be a pointless waste of CPU cycles.  ;-)
 *
 *		The hostname output parameter can be a variety of things, but in this
 *		method we convert the IP address to a string.  See the relevant
 *		documentation about inet_ntoa() for details.
 *
 *		Make sure hostname is large enough or you will step on someone
 *		else's toes.  (Can you say "buffer overflow exploit" boys and girls?)
 *		You can generally be safe using the MAXHOSTNAMELEN define, which
 *		defaults to 64 bytes--don't forget to leave room for the NULL!
 */
status_t
BNetAddress::GetAddr(char* hostname, unsigned short* port) const
{
	if (fInit != B_OK)
		return B_NO_INIT;

	if (port != NULL)
		*port = ntohs(fPort);

	if (hostname != NULL) {
		struct in_addr addr;
		addr.s_addr = fAddress;

		char* text = inet_ntoa(addr);
		if (text != NULL)
			strcpy(hostname, text);
	}

	return B_OK;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Class accessor.
 *
 * Output parameter:
 *		sa:					sockaddr_in struct to be filled.
 *
 * Returns:
 *		B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *		This method fills in the sin_addr, sin_family, and sin_port fields of
 *		the output parameter, all other fields are untouched so we can work
 *		with both POSIX and non-POSIX versions of said struct.  The port and
 *		address values added to the output parameter are in network byte order.
 */
status_t BNetAddress::GetAddr(struct sockaddr_in& sa) const
{
	if (fInit != B_OK)
		return B_NO_INIT;

	sa.sin_port = fPort;
	sa.sin_addr.s_addr = fAddress;
	if (check_r5_compatibility()) {
		r5_sockaddr_in* r5Addr = (r5_sockaddr_in*)&sa;
		if (fFamily == AF_INET)
			r5Addr->sin_family = R5_AF_INET;
		else
			r5Addr->sin_family = fFamily;
	} else
		sa.sin_family = fFamily;

	return B_OK;
}


/* GetAddr
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Class accessor.
 *
 * Output parameters:
 *		addr:			in_addr struct to fill.
 *		port:			optional port number to fill.
 *
 * Returns:
 *		B_OK for success, B_NO_INIT if instance was not properly constructed.
 *
 * Remarks:
 *		Output port will be in host byte order, but addr will be in the usual
 *		network byte order (ready to be used by other network functions).
 */
status_t BNetAddress::GetAddr(in_addr& addr, unsigned short* port) const
{
	if (fInit != B_OK)
		return B_NO_INIT;

	addr.s_addr = fAddress;

	if (port != NULL)
		*port = ntohs(fPort);

	return B_OK;
}


/* InitCheck
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Determine whether or not this instance is properly initialized.
 *
 * Returns:
 *		B_OK if this instance is initialized, B_ERROR if not.
 */
status_t BNetAddress::InitCheck() const
{
	return fInit == B_OK ? B_OK : B_ERROR;
}


status_t BNetAddress::InitCheck()
{
	return const_cast<const BNetAddress*>(this)->InitCheck();
}


/* Archive
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Serialize this instance into the passed BMessage parameter.
 *
 * Input parameter:
 *		deep:			[ignored] default==true.
 *
 * Output parameter:
 *		into:			BMessage object to serialize into.
 *
 * Returns:
 *		B_OK/BERROR on success/failure.  Returns B_NO_INIT if instance not
 *		properly initialized.
 */
status_t BNetAddress::Archive(BMessage* into, bool deep) const
{
	if (fInit != B_OK)
		return B_NO_INIT;

	if (into->AddInt16("bnaddr_family", fFamily) != B_OK)
		return B_ERROR;

	if (into->AddInt16("bnaddr_port", fPort) != B_OK)
		return B_ERROR;

	if (into->AddInt32("bnaddr_addr", fAddress) != B_OK)
		return B_ERROR;

	return B_OK;
}


/* Instantiate
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Un-serialize and instantiate from the passed BMessage parameter.
 *
 * Input parameter:
 *		archive:		Archived BMessage object for (de)serialization.
 *
 * Returns:
 *		NULL if a BNetAddress instance can not be initialized, otherwise
 *		a new BNetAddress object instantiated from the BMessage parameter.
 */
BArchivable*
BNetAddress::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BNetAddress"))
		return NULL;

	BNetAddress* address = new (std::nothrow) BNetAddress(archive);
	if (address == NULL)
		return NULL;

	if (address->InitCheck() != B_OK) {
		delete address;
		return NULL;
	}

	return address;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *	 Set hostname and port network address data.
 *
 * Input parameters:
 *		hostname:		Can be one of three things:
 *						1. An ASCII-string representation of an IP address.
 *						2. A canonical hostname.
 *						3. NULL.  If NULL, then by default the address will be
 *							set to INADDR_ANY (0.0.0.0).
 *		port:			Duh.
 *
 * Returns:
 *		B_OK/B_ERROR for success/failure.
 */
status_t
BNetAddress::SetTo(const char* hostname, unsigned short port)
{
	if (hostname == NULL)
		return B_ERROR;

	in_addr_t addr = INADDR_ANY;

	// Try like all git-out to set the address from the given hostname.

	// See if the string is an ASCII-fied IP address.
	addr = inet_addr(hostname);
	if (addr == INADDR_ANY || addr == (in_addr_t)-1) {
		// See if we can resolve the hostname to an IP address.
		struct hostent* host = gethostbyname(hostname);
		if (host != NULL)
			addr = *(int*)host->h_addr_list[0];
		else
			return B_ERROR;
	}

	fFamily = AF_INET;
	fPort = htons(port);
	fAddress = addr;

	return fInit = B_OK;
}


/*!
	Set from passed in sockaddr_in address.

	\param addr address
	\return B_OK.
*/
status_t
BNetAddress::SetTo(const struct sockaddr_in& addr)
{
	fPort = addr.sin_port;
	fAddress = addr.sin_addr.s_addr;

	if (check_r5_compatibility()) {
		const r5_sockaddr_in* r5Addr = (const r5_sockaddr_in*)&addr;
		if (r5Addr->sin_family == R5_AF_INET)
			fFamily = AF_INET;
		else
			fFamily = r5Addr->sin_family;
	} else
		fFamily = addr.sin_family;

	return fInit = B_OK;
}


/*!
	Set from passed in address and port.

	\param addr IP address in network form.
	\param port Optional port number.

	\return B_OK.
*/
status_t
BNetAddress::SetTo(in_addr addr, int port)
{
	fFamily = AF_INET;
	fPort = htons((short)port);
	fAddress = addr.s_addr;

	return fInit = B_OK;
}


/*!
	Set from passed in address and port.

	\param addr IP address in network form.
	\param port Optional port number.

	\return B_OK.
*/
status_t
BNetAddress::SetTo(uint32 addr, int port)
{
	fFamily = AF_INET;
	fPort = htons((short)port);
	fAddress = addr;

	return fInit = B_OK;
}


/* SetTo
 *=--------------------------------------------------------------------------=*
 * Purpose:
 *		Set from passed in hostname and protocol/service information.
 *
 * Input parameters:
 *		hostname:		Can be one of three things:
 *						1. An ASCII-string representation of an IP address.
 *						2. A canonical hostname.
 *						3. NULL.  If NULL, then by default the address will be
 *							set to INADDR_ANY (0.0.0.0).
 *		protocol:		Datagram type, typically "TCP" or "UDP"
 *		service:		The name of the service, such as http, ftp, et al. This
 *						must be one of the official service names listed in
 *						/etc/services -- but you already knew that because
 *						you're doing network/sockets programming, RIIIGHT???.
 *
 * Returns:
 *		B_OK/B_ERROR on success/failure.
 *
 * Remarks:
 *		The protocol and service input parameters must be one of the official
 *		types listed in /etc/services.  We use these two parameters to
 *		determine the port number (see getservbyname(3)).  This method will
 *		fail if the aforementioned precondition is not met.
 */
status_t
BNetAddress::SetTo(const char* hostname, const char* protocol,
	const char* service)
{
	struct servent* serviceEntry = getservbyname(service, protocol);
	if (serviceEntry == NULL)
		return B_ERROR;

	return SetTo(hostname, serviceEntry->s_port);
}


//	#pragma mark - FBC


void BNetAddress::_ReservedBNetAddressFBCCruft1() {}
void BNetAddress::_ReservedBNetAddressFBCCruft2() {}
void BNetAddress::_ReservedBNetAddressFBCCruft3() {}
void BNetAddress::_ReservedBNetAddressFBCCruft4() {}
void BNetAddress::_ReservedBNetAddressFBCCruft5() {}
void BNetAddress::_ReservedBNetAddressFBCCruft6() {}
