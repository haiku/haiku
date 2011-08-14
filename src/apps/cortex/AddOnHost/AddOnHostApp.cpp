/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// AddOnHostApp.cpp

#include "AddOnHostApp.h"
#include "AddOnHostProtocol.h"

#include <Alert.h>
#include <Debug.h>
#include <MediaRoster.h>

#include <cstdlib>
#include <cstring>

__USE_CORTEX_NAMESPACE
using namespace addon_host;


App::App()
	: BApplication(g_appSignature)
{
}


App::~App()
{
}


bool
App::QuitRequested()
{
	return true;
}


void
App::MessageReceived(BMessage* message)
{
	status_t err;
	
//	message->PrintToStream();

	switch(message->what) {
		case M_INSTANTIATE:
		{
			// fetch node info
			dormant_node_info info;
			const void *data;
			ssize_t dataSize;
			err = message->FindData("info", B_RAW_TYPE, &data, &dataSize);
			if (err < B_OK) {
				PRINT((
					"!!! App::MessageReceived(M_INSTANTIATE):\n"
					"    missing 'info'\n"));
				break;
			}
			if (dataSize != sizeof(info)) {
				PRINT((
					"*   App::MessageReceived(M_INSTANTIATE):\n"
					"    warning: 'info' size mismatch\n"));
				if (dataSize > ssize_t(sizeof(info)))
					dataSize = sizeof(info);
			}
			memcpy(reinterpret_cast<void *>(&info), data, dataSize);				

			// attempt to instantiate
			BMediaRoster* r = BMediaRoster::Roster();
			media_node node;
			err = r->InstantiateDormantNode(info, &node);
			
//			if(err == B_OK)	
//				// reference it
//				err = r->GetNodeFor(node.node, &node);

			// send status			
			if (err == B_OK) {
				BMessage m(M_INSTANTIATE_COMPLETE);
				m.AddInt32("node_id", node.node);
				message->SendReply(&m);
			}
			else {
				BMessage m(M_INSTANTIATE_FAILED);
				m.AddInt32("error", err);
				message->SendReply(&m);
			}

			break;
		}

		case M_RELEASE:
		{
			// fetch node info
			live_node_info info;
			const void *data;
			ssize_t dataSize;
			err = message->FindData("info", B_RAW_TYPE, &data, &dataSize);
			if (err < B_OK) {
				PRINT((
					"!!! App::MessageReceived(M_RELEASE):\n"
					"    missing 'info'\n"));
				break;
			}
			if (dataSize != sizeof(info)) {
				PRINT((
					"*   App::MessageReceived(M_RELEASE):\n"
					"    warning: 'info' size mismatch\n"));
				if(dataSize > ssize_t(sizeof(info)))
					dataSize = sizeof(info);
			}
			memcpy(reinterpret_cast<void *>(&info), data, dataSize);				

			// attempt to release
			BMediaRoster* r = BMediaRoster::Roster();
			media_node node;
			err = r->ReleaseNode(info.node);

			// send status			
			if (err == B_OK) {
				BMessage m(M_RELEASE_COMPLETE);
				m.AddInt32("node_id", info.node.node);
				message->SendReply(&m);
			}
			else {
				BMessage m(M_RELEASE_FAILED);
				m.AddInt32("error", err);
				message->SendReply(&m);
			}

			break;
		}

		default:
			_inherited::MessageReceived(message);
	}
}


int
main(int argc, char** argv)
{
	App app;
	if (argc < 2 || strcmp(argv[1], "--addon-host") != 0) {
		int32 response = (new BAlert(
			"Cortex AddOnHost",
			"This program runs in the background, and is started automatically "
			"by Cortex when necessary.  You probably don't want to start it manually.",
			"Continue", "Quit"))->Go();
		if(response == 1)
			return 0;
	}
	app.Run();
	return 0;
}
