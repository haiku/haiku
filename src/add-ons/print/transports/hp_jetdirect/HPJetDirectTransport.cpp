/*
 * Copyright 2001-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin,
 */


#include "HPJetDirectTransport.h"

#include <stdio.h>

#include <Alert.h>
#include <Message.h>
#include <Directory.h>
#include <SupportKit.h>
#include <String.h>
#include <NetEndpoint.h>

#include "SetupWindow.h"


HPJetDirectPort::HPJetDirectPort(BDirectory* printer, BMessage *msg)
	: fHost(""),
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


	fEndpoint = new BNetEndpoint(SOCK_STREAM);
	if ((fReady = fEndpoint->InitCheck()) != B_OK) {
		BAlert *alert = new BAlert("", "Fail to create the NetEndpoint!", "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}

	if (fEndpoint->Connect(fHost, fPort) == B_OK) {
		printf("Connected to HP JetDirect printer port at %s:%d\n",
			fHost.String(), fPort);
		fReady = B_OK;
	} else {
		BAlert *alert = new BAlert("",
			"Can't connect to HP JetDirect printer port!", "OK");
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

