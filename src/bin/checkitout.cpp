/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Jonas Sundström, jonas@kirilla.com
 */

/*
 * CheckItOut: wraps SCM URL mime types around command line apps
 * to check out sources in a user-friendly fashion.
 */
#define DEBUG 1

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include <Alert.h>
#include <Debug.h>
#include <FilePanel.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>

#include "checkitout.h"


const char* kAppSig = "application/x-vnd.Haiku-checkitout";
const char* kTrackerSig = "application/x-vnd.Be-TRAK";
#if __HAIKU__
const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";
#else
const char* kTerminalSig = "application/x-vnd.Be-SHEL";
#endif

CheckItOut::CheckItOut() : BApplication(kAppSig)
{
}


CheckItOut::~CheckItOut()
{
}


status_t
CheckItOut::_Warn(const char* url)
{
	BString message("An application has requested the system to open the "
		"following url: \n");
	message << "\n" << url << "\n\n";
	message << "This type of URL has a potential security risk.\n";
	message << "Proceed anyway?";
	BAlert* alert = new BAlert("Warning", message.String(), "Proceed", "Stop", NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 button;
	button = alert->Go();
	if (button == 0)
		return B_OK;
		
	return B_ERROR;
}


status_t
CheckItOut::_FilePanel(uint32 nodeFlavors, BString &name)
{
	BFilePanel *panel = new BFilePanel(B_SAVE_PANEL);
	/*
			BMessenger* target = NULL, const entry_ref* directory = NULL,
			uint32 nodeFlavors = 0, bool allowMultipleSelection = true,
			BMessage* message = NULL, BRefFilter* refFilter = NULL,
			bool modal = false, bool hideWhenDone = true);
	*/
	panel->Window()->SetTitle("Check It Out to...");
	panel->SetSaveText(name.String());
	panel->Show();
	return B_OK;
}


void
CheckItOut::RefsReceived(BMessage* msg)
{
}


void
CheckItOut::MessageReceived(BMessage* msg)
{
			msg->PrintToStream();
	switch (msg->what) {
		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			BString name;
			msg->FindRef("directory", &ref);
			msg->FindString("name", &name);
			_DoCheckItOut(&ref, name.String());
			break;
		}
		case B_CANCEL:
			Quit();
			break;
	}
}


void
CheckItOut::ArgvReceived(int32 argc, char** argv)
{
	if (argc <= 1) {
		exit(1);
		return;
	}
	
	BPrivate::Support::BUrl url(argv[1]);
	fUrlString = url;

	BString full = url.Full();
	BString proto = url.Proto();
	BString host = url.Host();
	BString port = url.Port();
	BString user = url.User();
	BString pass = url.Pass();
	BString path = url.Path();

	if (url.InitCheck() < 0) {
		fprintf(stderr, "malformed url: '%s'\n", url.String());
		return;
	}
	
	// XXX: debug
	PRINT(("PROTO='%s'\n", proto.String()));
	PRINT(("HOST='%s'\n", host.String()));
	PRINT(("PORT='%s'\n", port.String()));
	PRINT(("USER='%s'\n", user.String()));
	PRINT(("PASS='%s'\n", pass.String()));
	PRINT(("PATH='%s'\n", path.String()));

	BString leaf(url.Path());
	if (leaf.FindLast('/') > -1)
		leaf.Remove(0, leaf.FindLast('/') + 1);
	PRINT(("leaf %s\n", leaf.String()));
	_FilePanel(B_DIRECTORY_NODE, leaf);

}

status_t
CheckItOut::_DoCheckItOut(entry_ref *ref, const char *name)
{
	const char* failc = " || read -p 'Press any key'";
	const char* pausec = " ; read -p 'Press any key'";
	char* args[] = { (char *)"/bin/sh", (char *)"-c", NULL, NULL};

	BPrivate::Support::BUrl url(fUrlString.String());
	BString full = url.Full();
	BString proto = url.Proto();
	BString host = url.Host();
	BString port = url.Port();
	BString user = url.User();
	BString pass = url.Pass();
	BString path = url.Path();
	PRINT(("url %s\n", url.String()));
	BPath refPath(ref);

	if (proto == "git") {
		BString cmd("git clone ");
		cmd << url;
		cmd << " '" << refPath.Path() << "/" << name << "'";
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << " && open '" << refPath.Path() << "/" << name << "'";
		cmd << failc;
		PRINT(("CMD='%s'\n", cmd.String()));
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return B_OK;
	}
	if (proto == "rsync") {
		BString cmd("rsync ");
		cmd << url;
		cmd << " '" << refPath.Path() << "/" << name << "'";
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << " && open '" << refPath.Path() << "/" << name << "'";
		cmd << failc;
		PRINT(("CMD='%s'\n", cmd.String()));
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return B_OK;
	}
	if (proto == "svn" || proto == "svn+ssh") {
		BString cmd("svn checkout ");
		cmd << url;
		cmd << " '" << refPath.Path() << "/" << name << "'";
		PRINT(("CMD='%s'\n", cmd.String()));
		cmd << " && open '" << refPath.Path() << "/" << name << "'";
		cmd << failc;
		PRINT(("CMD='%s'\n", cmd.String()));
		args[2] = (char*)cmd.String();
		be_roster->Launch(kTerminalSig, 3, args);
		return B_OK;
	}
	return B_ERROR;
}


status_t
CheckItOut::_DecodeUrlString(BString& string)
{
	// TODO: check for %00 and bail out!
	int32 length = string.Length();
	int i;
	for (i = 0; string[i] && i < length - 2; i++) {
		if (string[i] == '%' && isxdigit(string[i+1])
			&& isxdigit(string[i+2])) {
			int c;
			sscanf(string.String() + i + 1, "%02x", &c);
			string.Remove(i, 3);
			string.Insert((char)c, 1, i);
			length -= 2;
		}
	}
	
	return B_OK;
}


void
CheckItOut::ReadyToRun(void)
{
	//Quit();
}


// #pragma mark


int main(int argc, char** argv)
{
	CheckItOut app;
	if (be_app)
		app.Run();
	return 0;
}

