/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef H_NETENDPOINT
#define H_NETENDPOINT


#include <BeBuild.h>
#include <Archivable.h>
#include <NetAddress.h>
#include <NetBuffer.h>
#include <SupportDefs.h>

#include <sys/socket.h>


class BNetEndpoint : public BArchivable {
	public:
		BNetEndpoint(int type = SOCK_STREAM);
		BNetEndpoint(int family, int type, int protocol);
		BNetEndpoint(const BNetEndpoint& other);
		BNetEndpoint(BMessage* archive);
		virtual ~BNetEndpoint();

		BNetEndpoint& operator=(const BNetEndpoint& other);

		status_t InitCheck() const;

		virtual	status_t Archive(BMessage* into, bool deep = true) const;
		static BArchivable* Instantiate(BMessage* archive);

		status_t SetProtocol(int protocol);
		int SetOption(int32 option, int32 level, const void* data, 
			unsigned int dataSize);
		int SetNonBlocking(bool on = true);
		int SetReuseAddr(bool on = true);

		const BNetAddress& LocalAddr() const;
		const BNetAddress& RemoteAddr() const;

		int Socket() const;

		virtual void Close();

		virtual status_t Bind(const BNetAddress& addr);
		virtual status_t Bind(int port = 0);
		virtual status_t Connect(const BNetAddress& addr);
		virtual status_t Connect(const char* addr, int port);
		virtual status_t Listen(int backlog = 5);
		virtual BNetEndpoint* Accept(int32 timeout = -1);

		int Error() const;
		char* ErrorStr() const;

		virtual int32 Send(const void* buffer, size_t size, int flags = 0);
		virtual int32 Send(BNetBuffer& buffer, int flags = 0);

		virtual int32 SendTo(const void* buffer, size_t size,
			const BNetAddress& to, int flags = 0);
		virtual int32 SendTo(BNetBuffer& buffer, const BNetAddress& to,
			int flags = 0);

		void SetTimeout(bigtime_t usec);

		virtual int32 Receive(void* buffer, size_t size, int flags = 0);
		virtual int32 Receive(BNetBuffer& buffer, size_t size, int flags = 0);
		virtual int32 ReceiveFrom(void* buffer, size_t size, BNetAddress& from,
			int flags = 0);
		virtual int32 ReceiveFrom(BNetBuffer& buffer, size_t size,
			BNetAddress& from, int flags = 0);

		virtual bool IsDataPending(bigtime_t timeout = 0);

		// TODO: drop these compatibility cruft methods after R1
		status_t InitCheck();
		const BNetAddress& LocalAddr();
		const BNetAddress& RemoteAddr();

	private:
		status_t _SetupSocket();

		virtual	void _ReservedBNetEndpointFBCCruft1();
		virtual	void _ReservedBNetEndpointFBCCruft2();
		virtual	void _ReservedBNetEndpointFBCCruft3();
		virtual	void _ReservedBNetEndpointFBCCruft4();
		virtual	void _ReservedBNetEndpointFBCCruft5();
		virtual	void _ReservedBNetEndpointFBCCruft6();

		status_t	fStatus;
		int			fFamily;
		int			fType;
		int			fProtocol;
		int			fSocket;
		bigtime_t	fTimeout;
		BNetAddress fAddr;
		BNetAddress fPeer;

		int32		_reserved[16];
};


#endif	// H_NETENDPOINT
