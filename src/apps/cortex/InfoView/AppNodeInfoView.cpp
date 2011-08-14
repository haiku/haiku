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


// AppNodeInfoView.cpp

#include "AppNodeInfoView.h"
// NodeManager
#include "NodeRef.h"
// Support
#include "MediaIcon.h"
#include "MediaString.h"

// Application Kit
#include <Roster.h>
// Media Kit
#include <MediaNode.h>
#include <MediaRoster.h>
// Storage Kit
#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

AppNodeInfoView::AppNodeInfoView(
	const NodeRef *ref)
	: LiveNodeInfoView(ref)
{
	D_METHOD(("AppNodeInfoView::AppNodeInfoView()\n"));

	// adjust view properties
	setSideBarWidth(be_plain_font->StringWidth(" File Format ") + 2 * InfoView::M_H_MARGIN);
	setSubTitle("Application-Owned Node");

	// add separator
	addField("", "");

	port_info portInfo;
	app_info appInfo;

	if ((get_port_info(ref->node().port, &portInfo) == B_OK)
	 && (be_roster->GetRunningAppInfo(portInfo.team, &appInfo) == B_OK))
	{
		BEntry appEntry(&appInfo.ref);
		char appName[B_FILE_NAME_LENGTH];
		if ((appEntry.InitCheck() == B_OK)
		 && (appEntry.GetName(appName) == B_OK))
		{
			addField("Application", appName);
		}
		BFile appFile(&appInfo.ref, B_READ_ONLY);
		if (appFile.InitCheck() == B_OK)
		{
			BAppFileInfo appFileInfo(&appFile);
			if (appFileInfo.InitCheck() == B_OK)
			{
				version_info appVersion;
				if (appFileInfo.GetVersionInfo(&appVersion, B_APP_VERSION_KIND) == B_OK)
				{
					addField("Version", appVersion.long_info);
				}
			}
		}
		addField("Signature", appInfo.signature);
	}
}

AppNodeInfoView::~AppNodeInfoView()
{
	D_METHOD(("AppNodeInfoView::~AppNodeInfoView()\n"));
}

// END -- AppNodeInfoView.cpp --
