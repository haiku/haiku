/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Philippe Houdoin
 */
#ifndef HP_JETDIRECT_TRANSPORT_H
#define HP_JETDIRECT_TRANSPORT_H

#include <DataIO.h>
#include <String.h>

class BDirectory;
class BMessage;
class BNetEndpoint;

class HPJetDirectPort : public BDataIO {
public:
							HPJetDirectPort(BDirectory* printer, BMessage* msg);
							~HPJetDirectPort();

		status_t 			InitCheck() { return fReady; }

		ssize_t 			Read(void* buffer, size_t size);
		ssize_t 			Write(const void* buffer, size_t size);

private:
		BString fHost;
		uint16 fPort;		// default is 9100
		BNetEndpoint *fEndpoint;
		status_t fReady;
};

#endif
