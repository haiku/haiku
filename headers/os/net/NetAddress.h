/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef H_NETADDRESS
#define H_NETADDRESS


#include <BeBuild.h>
#include <SupportDefs.h>
#include <Archivable.h>

#include <netinet/in.h>
#include <sys/socket.h>


class BNetAddress : public BArchivable {
	public:
		BNetAddress(BMessage* archive);
		virtual ~BNetAddress();

		virtual status_t Archive(BMessage* into, bool deep = true) const;
		static BArchivable* Instantiate(BMessage* archive);

		BNetAddress(const char* hostname = 0, unsigned short port = 0);
		BNetAddress(const struct sockaddr_in& addr);
		BNetAddress(in_addr addr, int port = 0);
		BNetAddress(uint32 addr, int port = 0);
		BNetAddress(const BNetAddress& other);
		BNetAddress(const char* hostname, const char* protocol, 
			const char* service);

		BNetAddress& operator=(const BNetAddress&);

		status_t InitCheck() const;

		status_t SetTo(const char* hostname, const char* protocol, 
			const char* service);
		status_t SetTo(const char* hostname = NULL, unsigned short port = 0);
		status_t SetTo(const struct sockaddr_in& addr);
		status_t SetTo(in_addr addr, int port = 0);
		status_t SetTo(uint32 addr = INADDR_ANY, int port = 0);

		status_t GetAddr(char* hostname = NULL, 
			unsigned short* port = NULL) const;
		status_t GetAddr(struct sockaddr_in& addr) const;
		status_t GetAddr(in_addr& addr, unsigned short* port = NULL) const;

		// TODO: drop this compatibility cruft method after R1
		status_t InitCheck();

	private:
		virtual void _ReservedBNetAddressFBCCruft1();
		virtual void _ReservedBNetAddressFBCCruft2();
		virtual void _ReservedBNetAddressFBCCruft3();
		virtual void _ReservedBNetAddressFBCCruft4();
		virtual void _ReservedBNetAddressFBCCruft5();
		virtual void _ReservedBNetAddressFBCCruft6();

		status_t fInit;
		int16	fFamily;
		int16	fPort;
		int32	fAddress;
		int32	fPrivateData[7];
};

#endif	// H_NETADDRESS
