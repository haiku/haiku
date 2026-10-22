/*
 * Copyright 2001-2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin,
 */


#include "HPJetDirectTransport.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Message.h>
#include <NetEndpoint.h>
#include <String.h>
#include <SupportKit.h>

#include "SetupWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HPJetDirectTransport"


HPJetDirectPort::HPJetDirectPort(BDirectory* printer, BMessage* msg)
	:
	fHost(""),
	fPort(9100),
	fEndpoint(NULL),
	fReady(B_ERROR)
{
	BString address;

	if (printer->ReadAttrString("transport_address", &address) < 0
		|| address.Length() == 0) {
		SetupWindow *setup = new SetupWindow(printer);
		if (setup->Go() == B_ERROR)
			return;
	}

	if (printer->ReadAttrString("transport_address", &address) < 0)
		return;

	printf("address = %s\n", address.String());

	int32 index = address.FindLast(':');
	if (index >= 0) {
		fPort = atoi(address.String() + index + 1);
		address.MoveInto(fHost, 0, index);
	} else
		fHost = address;

	printf("fHost = %s\n", fHost.String());
	printf("fPort = %d\n", fPort);


	fEndpoint = new(std::nothrow) BNetEndpoint(SOCK_STREAM);
	if (fEndpoint == NULL || (fReady = fEndpoint->InitCheck()) != B_OK) {
		BAlert* alert = new BAlert("", B_TRANSLATE("Fail to create the NetEndpoint!"), kOKLabel);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		delete fEndpoint;
		fEndpoint = NULL;
		return;
	}

	if (fEndpoint->Connect(fHost, fPort) == B_OK) {
		printf("Connected to HP JetDirect printer port at %s:%d\n",
			fHost.String(), fPort);
		fReady = B_OK;
	} else {
		BAlert* alert
			= new BAlert("", B_TRANSLATE("Can't connect to HP JetDirect printer port!"), kOKLabel);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		fReady = B_ERROR;
	}
}


HPJetDirectPort::~HPJetDirectPort()
{
	if (fEndpoint) {
		shutdown(fEndpoint->Socket(), SHUT_WR);
		fEndpoint->Close();
	}
	delete fEndpoint;
}


ssize_t
HPJetDirectPort::Read(void* buffer, size_t size)
{
	// printf("HPJetDirectPort::Read(%ld bytes)\n", size);
	if (fEndpoint)
		return (ssize_t) fEndpoint->Receive(buffer, size);
	return 0;
}


ssize_t
HPJetDirectPort::Write(const void* buffer, size_t size)
{
	// printf("HPJetDirectPort::Write(%ld bytes)\n", size);
	if (fEndpoint)
		return (ssize_t) fEndpoint->Send(buffer, size);
	return 0;
}

