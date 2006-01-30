/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SyslogDaemon.h"
#include "listener_output.h"
#include "syslog_output.h"

#include <Alert.h>
#include <TextView.h>
#include <Font.h>
#include <FindDirectory.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>


const char *kSignature = "application/x-vnd.Be-SYSL";


SyslogDaemon::SyslogDaemon()
	: BApplication(kSignature),
	fHandlerLock("handler lock")
{
}


void
SyslogDaemon::ReadyToRun()
{
	fPort = create_port(256, SYSLOG_PORT_NAME);
	fDaemon = spawn_thread(daemon_thread, "daemon", B_NORMAL_PRIORITY, this);

	if (fPort >= B_OK && fDaemon >= B_OK) {
		init_syslog_output(this);
		init_listener_output(this);

		resume_thread(fDaemon);
	} else
		Quit();
}


void
SyslogDaemon::AboutRequested()
{
	BPath path;
	find_directory(B_COMMON_LOG_DIRECTORY, &path);
	path.Append("syslog");

	char message[512];
	sprintf(message,
		"Syslog Daemon\n\n"
		"This daemon is responsible for collecting "
		"all system messages and write them to the "
		"system-wide log at \"%s\".\n\n", path.Path());

	BAlert *alert = new BAlert("Syslog Daemon", message, "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(21);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 13, &font);

	alert->Go(NULL);
}


bool
SyslogDaemon::QuitRequested()
{
	delete_port(fPort);

	int32 returnCode;
	wait_for_thread(fDaemon, &returnCode);

	return true;
}


void
SyslogDaemon::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case SYSLOG_ADD_LISTENER:
		{
			BMessenger messenger;
			if (msg->FindMessenger("target", &messenger) == B_OK)
				add_listener(&messenger);
			break;
		}
		case SYSLOG_REMOVE_LISTENER:
		{
			BMessenger messenger;
			if (msg->FindMessenger("target", &messenger) == B_OK)
				remove_listener(&messenger);
			break;
		}

		default:
			BApplication::MessageReceived(msg);
	}
}


void
SyslogDaemon::AddHandler(handler_func function)
{
	fHandlers.AddItem((void *)function);
}


void
SyslogDaemon::Daemon()
{
	char buffer[SYSLOG_MESSAGE_BUFFER_SIZE + 1];
	syslog_message &message = *(syslog_message *)buffer;
	int32 code;

	while (true) {
		ssize_t bytesRead = read_port(fPort, &code, &message, sizeof(buffer));
		if (bytesRead == B_BAD_PORT_ID) {
			// we've been quit
			break;
		}

		// if we don't get what we want, ignore it
		if (bytesRead < (ssize_t)sizeof(syslog_message) || code != SYSLOG_MESSAGE)
			continue;

		// add terminating null byte
		message.message[bytesRead - sizeof(syslog_message)] = '\0';

		if (!message.message[0]) {
			// ignore empty messages
			continue;
		}

		fHandlerLock.Lock();

		for (int32 i = fHandlers.CountItems(); i-- > 0;) {
			handler_func handle = (handler_func)fHandlers.ItemAt(i);

			handle(message);
		}

		fHandlerLock.Unlock();
	}
}


int32
SyslogDaemon::daemon_thread(void *data)
{
	((SyslogDaemon *)data)->Daemon();
	return B_OK;
}


int
main(int argc, char **argv)
{
	SyslogDaemon daemon;
	daemon.Run();

	return 0;
}
