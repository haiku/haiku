/*****************************************************************************/
// HP JetDirect (TCP/IP only) transport add-on,
//
// Author
//   Philippe Houdoin
// 
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include <stdio.h>

#include <Alert.h>
#include <Message.h>
#include <Directory.h>
#include <SupportKit.h>
#include <String.h>
#include <NetEndpoint.h>

#include "HPJetDirectTransport.h"
#include "SetupWindow.h"

// Implementation of HPJetDirectPort

HPJetDirectPort::HPJetDirectPort(BDirectory* printer, BMessage *msg) 
	: fPort(9100), fEndpoint(NULL), fReady(B_ERROR)
{
	fHost[0] = '\0';

	const char *spool_path = msg->FindString("printer_file");
	if (spool_path && *spool_path) {
		BDirectory dir(spool_path);

		dir.ReadAttr("hp_jetdirect:host", B_STRING_TYPE, 0, fHost, sizeof(fHost));
		if (fHost[0] == '\0') {
			SetupWindow *setup = new SetupWindow(&dir);
			if (setup->Go() == B_ERROR)
				return;
		}
		
		dir.ReadAttr("hp_jetdirect:host", B_STRING_TYPE, 0, fHost, sizeof(fHost));
		dir.ReadAttr("hp_jetdirect:port", B_UINT16_TYPE, 0, &fPort, sizeof(fPort));
	};

	fEndpoint = new BNetEndpoint(SOCK_STREAM);
	if ((fReady = fEndpoint->InitCheck()) != B_OK) {
		BAlert *alert = new BAlert("", "Fail to create the NetEndpoint!", "Damn");
		alert->Go();
		return;
	};
		
	if (fEndpoint->Connect(fHost, fPort) == B_OK) {
		printf("Connected to HP JetDirect printer port at %s:%d\n", fHost, fPort);
		fReady = B_OK;
	} else {
		BAlert *alert = new BAlert("", "Can't connect to HP JetDirect printer port!", "Bad luck");
		alert->Go();
	};
}


HPJetDirectPort::~HPJetDirectPort()
{
	delete fEndpoint;
}


ssize_t
HPJetDirectPort::Read(void* buffer, size_t size)
{
	// printf("HPJetDirectPort::Read(%ld bytes)\n", size);
	return (ssize_t) fEndpoint->Receive(buffer, size);
}


ssize_t
HPJetDirectPort::Write(const void* buffer, size_t size)
{
	// printf("HPJetDirectPort::Write(%ld bytes)\n", size);
	return (ssize_t) fEndpoint->Send(buffer, size);
}

