/******************************************************************************
/
/	File:			NetEndpoint.cpp
/
/   Description:    The Network API.
/
/	Copyright 2002, OpenBeOS Project, All Rights Reserved.
/
******************************************************************************/

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <Message.h>
#include <NetEndpoint.h>


BNetEndpoint::BNetEndpoint(int protocol)
	:	m_init(B_NO_INIT), m_socket(-1), m_timeout(B_INFINITE_TIMEOUT), m_last_error(0)
{
	if ((m_socket = socket(AF_INET, protocol, 0)) < 0)
		m_last_error = errno;
	else
		m_init = B_OK;
}


BNetEndpoint::BNetEndpoint(int family, int type, int protocol)
	:	m_init(B_NO_INIT), m_socket(-1), m_timeout(B_INFINITE_TIMEOUT), m_last_error(0)
{
	if ((m_socket = socket(family, type, protocol)) < 0)
		m_last_error = errno;
	else
		m_init = B_OK;
}


BNetEndpoint::BNetEndpoint(BMessage *archive)
	:	m_init(B_NO_INIT),
		m_socket(-1),
		m_timeout(B_INFINITE_TIMEOUT),
		m_last_error(0)
{
	// TODO
	if (! archive)
		return;
	
	BMessage msg;
	
	if (archive->FindMessage("bnendp_peer", &msg) != B_OK)
		return;
	m_peer = BNetAddress(&msg);
	
	if (archive->FindMessage("bnendp_addr", &msg) != B_OK)
		return;
	m_addr = BNetAddress(&msg);
	
	if (archive->FindMessage("bnendp_conaddr", &msg) != B_OK)
		return;
	m_conaddr = BNetAddress(&msg);

	m_init = B_OK;
}


BNetEndpoint::BNetEndpoint(const BNetEndpoint & ep)
	:	m_init(ep.m_init), m_timeout(ep.m_timeout),	m_last_error(ep.m_last_error),
		m_addr(ep.m_addr), m_peer(ep.m_peer)
{
	m_socket = -1;
	if (ep.m_socket >= 0)
		m_socket = dup(ep.m_socket);
}


BNetEndpoint & BNetEndpoint::operator=(const BNetEndpoint & ep)
{
	Close();

	m_init 			= ep.m_init;
	m_timeout 		= ep.m_timeout;
	m_last_error	= ep.m_last_error;
	m_addr			= ep.m_addr;
	m_peer			= ep.m_peer;

	m_socket = -1;
	if (ep.m_socket >= 0)
		m_socket = dup(ep.m_socket);

	if (m_socket >= 0)
		m_init = B_OK;

    return *this;
}

BNetEndpoint::~BNetEndpoint()
{
	Close();
}

// #pragma mark -

status_t BNetEndpoint::Archive(BMessage *into, bool deep = true) const
{
	// TODO
	
	if( into == 0 )
		return B_ERROR;

	if ( m_init != B_OK )
		return B_NO_INIT;
	
	BMessage msg;
	
	if (m_peer.Archive(&msg) != B_OK)
		return B_ERROR;
	if (into->AddMessage("bnendp_peer", &msg) != B_OK)
		return B_ERROR;
	
	if (m_addr.Archive(&msg) != B_OK)
		return B_ERROR;
	if (into->AddMessage("bnendp_addr", &msg) != B_OK)
		return B_ERROR;
	
	if (m_conaddr.Archive(&msg) != B_OK)
		return B_ERROR;
	if (into->AddMessage("bnendp_conaddr", &msg) != B_OK)
		return B_ERROR;
	
	return B_OK;
}


BArchivable *BNetEndpoint::Instantiate(BMessage *archive)
{
	if (! archive)
		return NULL;
	
	if (! validate_instantiation(archive, "BNetAddress") )
		return NULL;
	
	BNetEndpoint *ep = new BNetEndpoint(archive);
	if (ep && ep->InitCheck() == B_OK)
		return ep;
	
	delete ep;
	return NULL;
}

// #pragma mark -


status_t BNetEndpoint::InitCheck()
{
	return m_init;
}


int BNetEndpoint::Socket() const
{
	return m_socket;
}


const BNetAddress & BNetEndpoint::LocalAddr()
{
	return m_addr;
}


const BNetAddress & BNetEndpoint::RemoteAddr()
{
	return m_peer;
}


status_t BNetEndpoint::SetProtocol(int protocol)
{
	Close();
	if ((m_socket = socket(AF_INET, protocol, 0)) < 0) {
		m_last_error = errno;
		return m_last_error;
	}
	m_init = B_OK;	
	return m_init;
}


int BNetEndpoint::SetOption(int32 option, int32 level,
							 	 const void * data, unsigned int length)
{
	if (m_socket < 0)
		return B_ERROR;

	if (setsockopt(m_socket, level, option, data, length) < 0) {
		m_last_error = errno;
		return B_ERROR;
	}
	return B_OK;
}


int BNetEndpoint::SetNonBlocking(bool enable)
{
	int flags = fcntl(m_socket, F_GETFL);

	if (enable)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(m_socket, F_SETFL, flags) < 0) {
		m_last_error = errno;
		return B_ERROR;
	}

	return B_OK;
}


int BNetEndpoint::SetReuseAddr(bool enable)
{
	int onoff = (int) enable;
	return SetOption(SO_REUSEADDR, SOL_SOCKET, &onoff, sizeof(onoff));
}


void BNetEndpoint::SetTimeout(bigtime_t timeout)
{
	m_timeout = timeout;
}


int BNetEndpoint::Error() const
{
	return m_last_error;
}


char * BNetEndpoint::ErrorStr() const
{
	return strerror(m_last_error);
}


// #pragma mark -

void BNetEndpoint::Close()
{
	if (m_socket >= 0)
		close(m_socket);
	m_socket = -1;
	m_init = B_NO_INIT;
}


status_t BNetEndpoint::Bind(const BNetAddress & address)
{
	struct sockaddr_in addr;
	status_t status;
	
	status = address.GetAddr(addr);
	if (status != B_OK)
		return status;
		
	if (bind(m_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		m_last_error = errno;
		Close();
		return B_ERROR;
	}
		
	int sz = sizeof(addr);

	if (getsockname(m_socket, (struct sockaddr *) &addr, &sz) < 0) {
		m_last_error = errno;
		Close();
		return B_ERROR;
	}

#ifdef _NETDB_H_
	if (addr.sin_addr.s_addr == 0) {
		// Grrr, buggy getsockname!
		char hostname[MAXHOSTNAMELEN];
		struct hostent *he;
		gethostname(hostname, sizeof(hostname));
		he = gethostbyname(hostname);
		if (he)
			memcpy(&addr.sin_addr.s_addr, he->h_addr, sizeof(addr.sin_addr.s_addr));
	}
#endif	
	m_addr.SetTo(addr);
	return B_OK;
}


status_t BNetEndpoint::Bind(int port)
{
	BNetAddress addr(INADDR_ANY, port);
	return Bind(addr);
}


status_t BNetEndpoint::Connect(const BNetAddress & address)
{
	sockaddr_in addr;

	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;
		
	if (connect(m_socket, (sockaddr *) &addr, sizeof(addr)) < 0) {
		Close();
		m_last_error = errno;
		return B_ERROR;
	}

	int sz = sizeof(addr);
	if (getpeername(m_socket, (sockaddr *) &addr, &sz) < 0) {
		Close();
		return B_ERROR;
	}
		
	m_peer.SetTo(addr);
	return B_OK;
}


status_t BNetEndpoint::Connect(const char *hostname, int port)
{
	BNetAddress addr(hostname, port);
	return Connect(addr);
}


status_t BNetEndpoint::Listen(int backlog)
{
	if (listen(m_socket, backlog) < 0) {
		Close();
		m_last_error = errno;
		return B_ERROR;
	}
	return B_OK;
}


BNetEndpoint * BNetEndpoint::Accept(int32 timeout)
{
	if (! IsDataPending(timeout < 0 ? B_INFINITE_TIMEOUT : 1000LL * timeout))
		return NULL;
		
	struct sockaddr_in addr;
	int sz = sizeof(addr);

	int handle = accept(m_socket, (struct sockaddr *) &addr, &sz);
	if (handle < 0) {
		Close();
		m_last_error = errno;
		return NULL;
	}

	BNetEndpoint * ep = new BNetEndpoint(*this);
	if (! ep) {
		close(handle);
		return NULL;
	}
	
	ep->m_socket = handle;
	ep->m_peer.SetTo(addr);
	if (getsockname(handle, (struct sockaddr *) &addr, &sz) < 0) {
		ep->Close();
		delete ep;
		return NULL;
	}
	
	ep->m_addr.SetTo(addr);
	return ep;
}

// #pragma mark -

bool BNetEndpoint::IsDataPending(bigtime_t timeout)
{	
	fd_set fds;
	struct timeval tv, *tv_ptr = NULL;
	
	FD_ZERO(&fds);
	FD_SET(m_socket, &fds);
	
	if (timeout > 0) {
		tv.tv_sec = timeout / 1000000;
		tv.tv_usec = (timeout % 1000000);
		tv_ptr = &tv;	
	}

	if (select(m_socket + 1, &fds, NULL, NULL, &tv) < 0) {
		m_last_error = errno;
		return false;
	}
	
	return FD_ISSET(m_socket, &fds);
}


int32 BNetEndpoint::Receive(void * buffer, size_t length, int flags)
{
	if (m_timeout >= 0 && IsDataPending(m_timeout) == false)
		return 0;
		
	ssize_t sz = recv(m_socket, buffer, length, flags);
	if (sz < 0)
		m_last_error = errno;
	return sz;
}


int32 BNetEndpoint::Receive(BNetBuffer & buffer, size_t length, int flags)
{
	BNetBuffer chunk(length);
	length = Receive(chunk.Data(), length, flags);
	buffer.AppendData(chunk.Data(), length);
	return length;
}


int32 BNetEndpoint::ReceiveFrom(void * buffer, size_t length,
							    BNetAddress & address, int flags)
{
	if (m_timeout >= 0 && IsDataPending(m_timeout) == false)
		return 0;
		
	struct sockaddr_in addr;
	int sz = sizeof(addr);
	
	length = recvfrom(m_socket, buffer, length, flags,
						(struct sockaddr *) &addr, &sz);
	if (length < 0)
		m_last_error = errno;
	else
		address.SetTo(addr);
	return length;
}


int32 BNetEndpoint::ReceiveFrom(BNetBuffer & buffer, size_t length,
							    BNetAddress & address, int flags)
{
	BNetBuffer chunk(length);
	length = ReceiveFrom(chunk.Data(), length, address, flags);
	buffer.AppendData(chunk.Data(), length);
	return length;
}


int32 BNetEndpoint::Send(const void * buffer, size_t length, int flags)
{
	ssize_t sz;

	sz = send(m_socket, (const char *) buffer, length, flags);
	if (sz < 0)
		m_last_error = errno;
	return sz;
}


int32 BNetEndpoint::Send(BNetBuffer & buffer, int flags)
{
	return Send(buffer.Data(), buffer.Size(), flags);
}


int32 BNetEndpoint::SendTo(const void * buffer, size_t length,
						   const BNetAddress & address, int flags)
{
	ssize_t	sz;
	struct sockaddr_in addr;

	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;
		
	sz = sendto(m_socket, buffer, length, flags,
				  (struct sockaddr *) &addr, sizeof(addr));
	if (sz < 0)
		m_last_error = errno;
	return sz;
}

int32 BNetEndpoint::SendTo(BNetBuffer & buffer,
						   const BNetAddress & address, int flags)
{
	return SendTo(buffer.Data(), buffer.Size(), address, flags);
}

// #pragma mark -

// These are virtuals, implemented for binary compatibility purpose
void BNetEndpoint::_ReservedBNetEndpointFBCCruft1()
{
}


void BNetEndpoint::_ReservedBNetEndpointFBCCruft2()
{
}


void BNetEndpoint::_ReservedBNetEndpointFBCCruft3()
{
}


void BNetEndpoint::_ReservedBNetEndpointFBCCruft4()
{
}


void BNetEndpoint::_ReservedBNetEndpointFBCCruft5()
{
}


void BNetEndpoint::_ReservedBNetEndpointFBCCruft6()
{
}

