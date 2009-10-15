/*
 * Copyright 2008, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "hdb.h"
#include "TeamWindow.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <String.h>
#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <debugger.h>


TeamWindow::TeamWindow(team_id team)
	: BWindow(BRect(100, 100, 500, 250), "Running Teams", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fTeam(team), 
	fNubThread(-1),
	fNubPort(-1),
	fListenerThread(-1), 
	fListenerPort(-1)
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("team window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	team_info info;
	get_team_info(team, &info);

	BString title;
	title << "Team " << fTeam << ": " << info.args;
	
	SetTitle(title.String());

	_InstallDebugger();
}


TeamWindow::~TeamWindow()
{
	_RemoveDebugger();
}


void
TeamWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamWindow::QuitRequested()
{
/*	_SaveSettings();

	BMessage update(kMsgSettingsChanged);
	update.AddRect("window_frame", Frame());
	be_app_messenger.SendMessage(&update);
*/
	be_app_messenger.SendMessage(kMsgWindowClosed);
	return true;
}


// #pragma mark --


status_t
TeamWindow::_InstallDebugger()
{
	char name[32];
	status_t result;
	
	snprintf(name, sizeof(name), "team %ld nub listener", fTeam);
	printf("Starting %s...\n", name);
	
	// create listener port
	fListenerPort = create_port(10, name);
	if (fListenerPort < 0)
		return fListenerPort;

	// spawn the listener thread
	fListenerThread = spawn_thread(_ListenerEntry, name, 
		B_NORMAL_PRIORITY, this);
	if (fListenerThread < 0)
		return fListenerThread;

	// register as this team debugger
	printf("Installing team %ld debugger on port 0x%lx...\n", 
		fTeam, fListenerPort);
	fNubPort = install_team_debugger(fTeam, fListenerPort);
	if (fNubPort < 0)
		return fNubPort;
		
	// init the debug context
	result = init_debug_context(&fDebugContext, fTeam, fNubPort);
	if (result != B_OK) {
		fprintf(stderr, "Failed to init debug context for team %ld: %s\n",
			fTeam, strerror(result));
	}

	// get team nub thread
	team_info teamInfo;
	result = get_team_info(fTeam, &teamInfo);
	if (result != B_OK) {
		fprintf(stderr, "Failed to get info for team %ld: %s\n",
			fTeam, strerror(result));
	}
	fNubThread = teamInfo.debugger_nub_thread;

	// set the team debug flags
	debug_nub_set_team_flags message;
	message.flags = 
		B_TEAM_DEBUG_SIGNALS | 
		// B_TEAM_DEBUG_PRE_SYSCALL |
		// B_TEAM_DEBUG_POST_SYSCALL | 
		B_TEAM_DEBUG_TEAM_CREATION |
		B_TEAM_DEBUG_THREADS | 
		B_TEAM_DEBUG_IMAGES |
		// B_TEAM_DEBUG_STOP_NEW_THREADS |
		0;

	send_debug_message(&fDebugContext,
			B_DEBUG_MESSAGE_SET_TEAM_FLAGS, &message, sizeof(message), NULL, 0);

	// resume the listener
	resume_thread(fListenerThread);

	return B_OK;
}


status_t
TeamWindow::_RemoveDebugger()
{
	if (fListenerPort < 0)
		// No debugger installed (yet?)
		return B_OK;

	printf("Stopping team %ld nub listener...\n", fTeam);

	printf("Removing team %ld debugger installed on port 0x%lx...\n", 
		fTeam, fListenerPort);
	status_t status = remove_team_debugger(fTeam);

	delete_port(fListenerPort);
	if (fListenerThread >= 0 && find_thread(NULL) != fListenerThread) {
		status_t result;
		wait_for_thread(fListenerThread, &result);
		fListenerThread = -1;
	}
	
	destroy_debug_context(&fDebugContext);
	fListenerPort = -1;
	
	return status;
}


status_t
TeamWindow::_ListenerEntry(void *data)
{
	return ((TeamWindow*) data)->_Listener();
}

// _Listener
status_t
TeamWindow::_Listener()
{
	printf("Team %ld nub listener on port 0x%lx started...\n", fTeam, fListenerPort);

	while (true) {
		// receive the next debug message
		debug_debugger_message		code;
		debug_debugger_message_data	data;
		ssize_t bytesRead;
		
		do {
			bytesRead = read_port(fListenerPort, (int32 *) &code, &data,
				sizeof(debug_debugger_message_data));
		} while (bytesRead == B_INTERRUPTED);

		if (bytesRead == B_BAD_PORT_ID)
			break;

		if (bytesRead < 0) {
			fprintf(stderr, "Team %ld nub listener: failed to read from "
					"listener port: %s. Terminating!\n", 
					fTeam, strerror(bytesRead));
			break;
		}
		
		_HandleDebugMessage(code, data);
	}

	printf("Team %ld nub listener on port 0x%lx stopped.\n", fTeam, fListenerPort);

	return B_OK;
}


status_t
TeamWindow::_HandleDebugMessage(debug_debugger_message code, 
	debug_debugger_message_data & data)
{
	char dump[512];
	get_debug_message_string(code, dump, sizeof(dump));
		
	printf("Team %ld nub listener: code %d from team %ld received: %s\n", 
		fTeam, code, data.origin.team, dump);

	switch (code) {
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
		{
			// print the debugger message
			char debuggerMessage[1024];
			ssize_t bytesRead = debug_read_string(&fDebugContext,
				data.debugger_call.message, debuggerMessage,
				sizeof(debuggerMessage));
			if (bytesRead > 0) {
				printf("  Thread %ld called debugger(): %s\n",
					data.origin.thread, debuggerMessage);
			} else {
				fprintf(stderr, "  Thread %ld called debugger(), but failed"
					"to get the debugger message.\n",
					data.origin.thread);
			}
			// fall through...
		}
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
		case B_DEBUGGER_MESSAGE_SINGLE_STEP: {
			// ATM, just continue
			debug_nub_continue_thread message;
			message.thread = data.origin.thread;
			message.handle_event = B_THREAD_DEBUG_HANDLE_EVENT;
			message.single_step = false;	// run full speed
		
			status_t result=  send_debug_message(&fDebugContext,
				B_DEBUG_MESSAGE_CONTINUE_THREAD, &message, sizeof(message), 
				NULL, 0);
			if (result != B_OK) {
				fprintf(stderr, "Failed to resume thread %ld: %s\n", 		
					message.thread, strerror(result));
			}
			break;
		}
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			printf("  pre_syscall.syscall = %ld\n", data.pre_syscall.syscall);
			break;

		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			printf("  post_syscall.syscall = %ld\n", data.post_syscall.syscall);
			break;

		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			printf("  signal_received.signal = %d\n", 
				data.signal_received.signal);
			break;

		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
		{
			// print the exception message
			char exception[1024];
			get_debug_exception_string(data.exception_occurred.exception,
				exception, sizeof(exception));
			printf("  Thread %ld caused an exception: signal %d, %s\n",
				data.origin.thread, data.exception_occurred.signal, exception);
			break;
		}

		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			break;

		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			printf("  thread_created.new_thread = %ld\n", 
				data.thread_created.new_thread);
			break;
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			printf("  thread_deleted.origin.thread = %ld\n", 
				data.thread_deleted.origin.thread);
			break;

		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			printf("  id = %ld, type = %d, name = %s\n", 
				data.image_created.info.id, 
				data.image_created.info.type, 
				data.image_created.info.name);
			break;

		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			printf("  id = %ld, type = %d, name = %s\n", 
				data.image_deleted.info.id, 
				data.image_deleted.info.type, 
				data.image_deleted.info.name);
			break;

		default:
			// unknown message, ignore
			break;
	}
	
	return B_OK;
	
}

// ----

status_t
TeamWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Debugger settings");

	return file.SetTo(path.Path(), mode);
}


status_t
TeamWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
TeamWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, 
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	
	if (status < B_OK)
		return status;

	BMessage settings('hdbg');
	status = settings.AddRect("team window frame", Frame());
	if (status != B_OK)
		return status;

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}
