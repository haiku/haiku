/*
 * Copyright 2003-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SyslogDaemon.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Path.h>
#include <TextView.h>

#include <LaunchRoster.h>
#include <syscalls.h>
#include <syslog_daemon.h>

#include "listener_output.h"
#include "syslog_output.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SyslogDaemon"


static const int32 kQuitDaemon = 'quit';


SyslogDaemon::SyslogDaemon()
	:
	BApplication(B_SYSTEM_LOGGER_SIGNATURE),
	fHandlerLock("handler lock")
{
}


void
SyslogDaemon::ReadyToRun()
{
	fPort = BLaunchRoster().GetPort("logger");
	fDaemon = spawn_thread(_DaemonThread, "daemon", B_NORMAL_PRIORITY, this);

	if (fPort >= 0 && fDaemon >= 0) {
		_kern_register_syslog_daemon(fPort);

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
	find_directory(B_SYSTEM_LOG_DIRECTORY, &path);
	path.Append("syslog");

	BString name(B_TRANSLATE("Syslog Daemon"));
	BString message;
	snprintf(message.LockBuffer(512), 512,
		B_TRANSLATE("%s\n\nThis daemon is responsible for collecting "
			"all system messages and write them to the system-wide log "
			"at \"%s\".\n\n"), name.String(), path.Path());
	message.UnlockBuffer();

	BAlert* alert = new BAlert(name.String(), message.String(),
		B_TRANSLATE("OK"));
	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(21);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, name.Length(), &font);

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}


bool
SyslogDaemon::QuitRequested()
{
	write_port(fPort, kQuitDaemon, NULL, 0);
	wait_for_thread(fDaemon, NULL);

	return true;
}


void
SyslogDaemon::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SYSLOG_ADD_LISTENER:
		{
			BMessenger messenger;
			if (message->FindMessenger("target", &messenger) == B_OK)
				add_listener(&messenger);
			break;
		}
		case SYSLOG_REMOVE_LISTENER:
		{
			BMessenger messenger;
			if (message->FindMessenger("target", &messenger) == B_OK)
				remove_listener(&messenger);
			break;
		}

		default:
			BApplication::MessageReceived(message);
	}
}


void
SyslogDaemon::AddHandler(handler_func function)
{
	fHandlers.AddItem((void*)function);
}


void
SyslogDaemon::_Daemon()
{
	char buffer[SYSLOG_MESSAGE_BUFFER_SIZE + 1];
	syslog_message& message = *(syslog_message*)buffer;
	int32 code;

	while (true) {
		ssize_t bytesRead = read_port(fPort, &code, &message, sizeof(buffer));
		if (bytesRead == B_BAD_PORT_ID) {
			// we've been quit
			break;
		}

		if (code == kQuitDaemon)
			return;

		// if we don't get what we want, ignore it
		if (bytesRead < (ssize_t)sizeof(syslog_message)
			|| code != SYSLOG_MESSAGE)
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
SyslogDaemon::_DaemonThread(void* data)
{
	((SyslogDaemon*)data)->_Daemon();
	return B_OK;
}


// #pragma mark -


int
main(int argc, char** argv)
{
	SyslogDaemon daemon;
	daemon.Run();

	return 0;
}
