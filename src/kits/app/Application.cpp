//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku, inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Application.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BApplication class is the center of the application
//					universe.  The global be_app and be_app_messenger 
//					variables are defined here as well.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <new>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// System Includes -------------------------------------------------------------
#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <AppMisc.h>
#include <Cursor.h>
#include <Entry.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <Locker.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <RegistrarDefs.h>
#include <Resources.h>
#include <Roster.h>
#include <RosterPrivate.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------
#include <LooperList.h>
#include <ObjectLocker.h>
#include <AppServerLink.h>
#include <ServerProtocol.h>
#include <PortLink.h>
#include <PrivateScreen.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
BApplication*	be_app = NULL;
BMessenger		be_app_messenger;

BResources*	BApplication::_app_resources = NULL;
BLocker		BApplication::_app_resources_lock("_app_resources_lock");

BPrivateScreen *gPrivateScreen = NULL;

property_info gApplicationPropInfo[] =
{
	{
		"Window",
			{},
			{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
			NULL, 0,
			{},
			{},
			{}
	},
	{
		"Window",
			{},
			{B_NAME_SPECIFIER},
			NULL, 1,
			{},
			{},
			{}
	},
	{
		"Looper",
			{},
			{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
			NULL, 2,
			{},
			{},
			{}
	},
	{
		"Looper",
			{},
			{B_ID_SPECIFIER},
			NULL, 3,
			{},
			{},
			{}
	},
	{
		"Looper",
			{},
			{B_NAME_SPECIFIER},
			NULL, 4,
			{},
			{},
			{}
	},
	{
		"Name",
			{B_GET_PROPERTY},
			{B_DIRECT_SPECIFIER},
			NULL, 5,
			{B_STRING_TYPE},
			{},
			{}
	},
	{
		"Window",
			{B_COUNT_PROPERTIES},
			{B_DIRECT_SPECIFIER},
			NULL, 5,
			{B_INT32_TYPE},
			{},
			{}
	},
	{
		"Loopers",
			{B_GET_PROPERTY},
			{B_DIRECT_SPECIFIER},
			NULL, 5,
			{B_MESSENGER_TYPE},
			{},
			{}
	},
	{
		"Windows",
			{B_GET_PROPERTY},
			{B_DIRECT_SPECIFIER},
			NULL, 5,
			{B_MESSENGER_TYPE},
			{},
			{}
	},
	{
		"Looper",
			{B_COUNT_PROPERTIES},
			{B_DIRECT_SPECIFIER},
			NULL, 5,
			{B_INT32_TYPE},
			{},
			{}
	},
	{}
};

// argc/argv
extern const int __libc_argc;
extern const char * const *__libc_argv;

class BMenuWindow : public BWindow
{
};
//------------------------------------------------------------------------------

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

enum {
	NOT_IMPLEMENTED	= B_ERROR
};

// prototypes of helper functions
static const char* looper_name_for(const char *signature);
static status_t check_app_signature(const char *signature);

//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature)
			: BLooper(looper_name_for(signature)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
	InitData(signature, NULL);
}
//------------------------------------------------------------------------------
BApplication::BApplication(const char* signature, status_t* error)
			: BLooper(looper_name_for(signature)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
	InitData(signature, error);
}
//------------------------------------------------------------------------------
BApplication::~BApplication()
{
	// tell all loopers(usually windows) to quit. Also, wait for them.
	
	// TODO: As Axel suggested, this functionality should probably be moved
	// to quit_all_windows(), and that function should be called from both 
	// here and QuitRequested().

	BWindow*	window = NULL;
	BList		looperList;
	{
		using namespace BPrivate;
		BObjectLocker<BLooperList> ListLock(gLooperList);
		if (ListLock.IsLocked())
			gLooperList.GetLooperList(&looperList);
	}

	for (int32 i = 0; i < looperList.CountItems(); i++)
	{
		window	= dynamic_cast<BWindow*>((BLooper*)looperList.ItemAt(i));
		if (window)
		{
			window->Lock();
			window->Quit();
		}
	}

	// unregister from the roster
	BRoster::Private().RemoveApp(Team());

	// tell app_server we're quitting...
	BPortLink link(fServerFrom);
	link.StartMessage(B_QUIT_REQUESTED);
	link.Flush();

	// uninitialize be_app and be_app_messenger
	be_app = NULL;
	
	// R5 doesn't uninitialize be_app_messenger.
	//be_app_messenger = BMessenger();
}
//------------------------------------------------------------------------------
BApplication::BApplication(BMessage* data)
			: BLooper(looper_name_for(NULL)),
			  fAppName(NULL),
			  fServerFrom(-1),
			  fServerTo(-1),
			  fServerHeap(NULL),
			  fPulseRate(500000),
			  fInitialWorkspace(0),
			  fDraggedMessage(NULL),
			  fPulseRunner(NULL),
			  fInitError(B_NO_INIT),
			  fReadyToRunCalled(false)
{
}
//------------------------------------------------------------------------------
BArchivable* BApplication::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BApplication"))
		return new BApplication(data);
	
	return NULL;	
}
//------------------------------------------------------------------------------
status_t BApplication::Archive(BMessage* data, bool deep) const
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
status_t BApplication::InitCheck() const
{
	return fInitError;
}
//------------------------------------------------------------------------------
thread_id BApplication::Run()
{
	AssertLocked();

	if (fRunCalled)	
		debugger("BApplication::Run was already called. Can only be called once.");

	// Note: We need a local variable too (for the return value), since
	// fTaskID is cleared by Quit().
	thread_id thread = fTaskID = find_thread(NULL);

	if (fMsgPort < B_OK)
		return fMsgPort;

	fRunCalled = true;

	run_task();

	return thread;
}
//------------------------------------------------------------------------------
void BApplication::Quit()
{
	bool unlock = false;
	if (!IsLocked()) {
		const char* name = Name();
		if (!name)
			name = "no-name";
		printf("ERROR - you must Lock the application object before calling "
			   "Quit(), team=%ld, looper=%s\n", Team(), name);
		unlock = true;
		if (!Lock())
			return;
	}
	// Delete the object, if not running only.
	if (!fRunCalled) {
		delete this;
	} else if (find_thread(NULL) != fTaskID) {
		// We are not the looper thread.
		// We push a _QUIT_ into the queue.
		// TODO: When BLooper::AddMessage() is done, use that instead of
		// PostMessage()??? This would overtake messages that are still at
		// the port.
		// NOTE: We must not unlock here -- otherwise we had to re-lock, which
		// may not work. This is bad, since, if the port is full, it
		// won't get emptier, as the looper thread needs to lock the object
		// before dispatching messages.
		while (PostMessage(_QUIT_, this) == B_WOULD_BLOCK)
			snooze(10000);
	} else {
		// We are the looper thread.
		// Just set fTerminating to true which makes us fall through the
		// message dispatching loop and return from Run().
		fTerminating = true;
	}
	// If we had to lock the object, unlock now.
	if (unlock)
		Unlock();
}
//------------------------------------------------------------------------------
bool BApplication::QuitRequested()
{
	// No windows -- nothing to do.
	// TODO: Au contraire, we can have opened windows, and we have
	// to quit them.
	
	return BLooper::QuitRequested();
}
//------------------------------------------------------------------------------
void BApplication::Pulse()
{
}
//------------------------------------------------------------------------------
void BApplication::ReadyToRun()
{
}
//------------------------------------------------------------------------------
void BApplication::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		// TODO: Handle these
		
		// Bebook says: B_SILENT_RELAUNCH
		// Sent to a single-launch application when it's activated by being launched
		// (for example, if the user double-clicks its icon in Tracker).
		case B_SILENT_RELAUNCH:		
		case B_COUNT_PROPERTIES:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
			break;

		default:
			BLooper::MessageReceived(msg);
			break;
	}
	
}
//------------------------------------------------------------------------------
void BApplication::ArgvReceived(int32 argc, char** argv)
{
}
//------------------------------------------------------------------------------
void BApplication::AppActivated(bool active)
{
}
//------------------------------------------------------------------------------
void BApplication::RefsReceived(BMessage* a_message)
{
}
//------------------------------------------------------------------------------
void BApplication::AboutRequested()
{
	thread_info info;
	if (get_thread_info(Thread(), &info) == B_OK) {
		BAlert *alert = new BAlert("_about_", info.name, "OK");
		alert->Go(NULL);
	}
}
//------------------------------------------------------------------------------
BHandler* BApplication::ResolveSpecifier(BMessage* msg, int32 index,
										 BMessage* specifier, int32 form,
										 const char* property)
{
	return NULL; // TODO: implement? not implemented?
}
//------------------------------------------------------------------------------
void BApplication::ShowCursor()
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_SHOW_CURSOR);
	link.Flush();
}
//------------------------------------------------------------------------------
void BApplication::HideCursor()
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_HIDE_CURSOR);
	link.Flush();
}
//------------------------------------------------------------------------------
void BApplication::ObscureCursor()
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_OBSCURE_CURSOR);
	link.Flush();
}
//------------------------------------------------------------------------------
bool BApplication::IsCursorHidden() const
{
	BPrivate::BAppServerLink link;
	int32 code = SERVER_FALSE;
	link.StartMessage(AS_QUERY_CURSOR_HIDDEN);
	link.FlushWithReply(&code);
	return (code==SERVER_TRUE)?true:false;
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const void* cursor)
{
	// BeBook sez: If you want to call SetCursor() without forcing an immediate
	//				sync of the Application Server, you have to use a BCursor.
	// By deductive reasoning, this function forces a sync. =)
	BCursor Cursor(cursor);
	SetCursor(&Cursor, true);
}
//------------------------------------------------------------------------------
void BApplication::SetCursor(const BCursor* cursor, bool sync)
{
	BPrivate::BAppServerLink link;
	int32 code=SERVER_FALSE;
	
	link.StartMessage(AS_SET_CURSOR_BCURSOR);
	link.Attach<bool>(sync);
	link.Attach<int32>(cursor->m_serverToken);
	if(sync)
		link.FlushWithReply(&code);
	else
		link.Flush();
}
//------------------------------------------------------------------------------
int32 BApplication::CountWindows() const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return count_windows(false);
}
//------------------------------------------------------------------------------
BWindow* BApplication::WindowAt(int32 index) const
{
	// BeBook sez: The windows list includes all windows explicitely created by
	//				the app ... but excludes private windows create by Be
	//				classes.
	// I'm taking this to include private menu windows, thus the incl_menus
	// param is false.
	return window_at(index, false);
}
//------------------------------------------------------------------------------
int32 BApplication::CountLoopers() const
{
	using namespace BPrivate;
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		return gLooperList.CountLoopers();
	}

	return B_ERROR;	// Some bad, non-specific thing has happened
}
//------------------------------------------------------------------------------
BLooper* BApplication::LooperAt(int32 index) const
{
	using namespace BPrivate;
	BLooper* Looper = NULL;
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		Looper = gLooperList.LooperAt(index);
	}

	return Looper;
}
//------------------------------------------------------------------------------
bool BApplication::IsLaunching() const
{
	return !fReadyToRunCalled;
}
//------------------------------------------------------------------------------
status_t BApplication::GetAppInfo(app_info* info) const
{
	return be_roster->GetRunningAppInfo(be_app->Team(), info);
}
//------------------------------------------------------------------------------
BResources* BApplication::AppResources()
{
	if (!_app_resources_lock.Lock())
		return NULL;
	
	// BApplication caches its resources, so check
	// if it already happened.
	if (_app_resources != NULL) {
		_app_resources_lock.Unlock();
		return _app_resources;
	}
	
	entry_ref ref;
	bool found = false;
	
	// App is already running. Get its entry ref with
	// GetRunningAppInfo()	
	app_info appInfo;
	if (be_app && be_roster->GetRunningAppInfo(be_app->Team(), &appInfo) == B_OK) {
		ref = appInfo.ref;
		found = true;
	
	} else
		// Run() hasn't been called yet
		found = BPrivate::get_app_ref(&ref) == B_OK;
		
	if (found) {
		BFile file(&ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BResources *resources = new BResources();
			if (resources->SetTo(&file, false) < B_OK)
				delete resources;

			else
				_app_resources = resources;			
		}
	}
	
	_app_resources_lock.Unlock();
	
	return _app_resources;
}
//------------------------------------------------------------------------------
void BApplication::DispatchMessage(BMessage* message, BHandler* handler)
{
	switch (message->what) {
		case B_ARGV_RECEIVED:
		{
			// build the argv vector
			status_t error = B_OK;
			int32 argc;
			char **argv = NULL;
			if (message->FindInt32("argc", &argc) == B_OK && argc > 0) {
				argv = new char*[argc];
				for (int32 i = 0; error == B_OK && i < argc; i++)
					argv[i] = NULL;
				// copy the arguments
				for (int32 i = 0; error == B_OK && i < argc; i++) {
					const char *arg = NULL;
					error = message->FindString("argv", i, &arg);
					if (error == B_OK && arg) {
						argv[i] = new(std::nothrow) char[strlen(arg) + 1];
						if (argv[i])
							strcpy(argv[i], arg);
						else
							error = B_NO_MEMORY;
					}
				}
			}
			// call the hook
			if (error == B_OK)
				ArgvReceived(argc, argv);
			// cleanup
			if (argv) {
				for (int32 i = 0; i < argc; i++)
					delete[] argv[i];
				delete[] argv;
			}
			break;
		}

		case B_REFS_RECEIVED:
			RefsReceived(message);
			break;

		case B_READY_TO_RUN:
			ReadyToRun();
			fReadyToRunCalled = true;
			break;
		
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		
		// TODO: Handle these as well
		/*
		// These two are handled by BTextView, don't know if also
		// by other classes
		case _DISPOSE_DRAG_: 
		case _PING_:
		
		case _SHOW_DRAG_HANDLES_:
		case B_QUIT_REQUESTED:
			
			break;
		*/
		default:
			BLooper::DispatchMessage(message, handler);
			break;
	}
}
//------------------------------------------------------------------------------
void BApplication::SetPulseRate(bigtime_t rate)
{
	fPulseRate = rate;
}
//------------------------------------------------------------------------------
status_t BApplication::GetSupportedSuites(BMessage* data)
{
	status_t err = B_OK;
	if (!data)
	{
		err = B_BAD_VALUE;
	}

	if (!err)
	{
		err = data->AddString("Suites", "suite/vnd.Be-application");
		if (!err)
		{
			BPropertyInfo PropertyInfo(gApplicationPropInfo);
			err = data->AddFlat("message", &PropertyInfo);
			if (!err)
			{
				err = BHandler::GetSupportedSuites(data);
			}
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BApplication::Perform(perform_code d, void* arg)
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
BApplication::BApplication(uint32 signature)
{
}
//------------------------------------------------------------------------------
BApplication::BApplication(const BApplication& rhs)
{
}
//------------------------------------------------------------------------------
BApplication& BApplication::operator=(const BApplication& rhs)
{
	return *this;
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication1()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication2()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication3()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication4()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication5()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication6()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication7()
{
}
//------------------------------------------------------------------------------
void BApplication::_ReservedApplication8()
{
}
//------------------------------------------------------------------------------
bool BApplication::ScriptReceived(BMessage* msg, int32 index, BMessage* specifier, int32 form, const char* property)
{
	return false; // TODO: Implement? Not implemented?
}
//------------------------------------------------------------------------------
void BApplication::run_task()
{
	task_looper();
}
//------------------------------------------------------------------------------
void BApplication::InitData(const char* signature, status_t* error)
{
	// check whether there exists already an application
	if (be_app)
		debugger("2 BApplication objects were created. Only one is allowed.");
	// check signature
	fInitError = check_app_signature(signature);
	fAppName = signature;

#ifndef RUN_WITHOUT_REGISTRAR
	bool isRegistrar
		= (signature && !strcasecmp(signature, kRegistrarSignature));
	// get team and thread
	team_id team = Team();
	thread_id thread = BPrivate::main_thread_for(team);
#endif
	// get app executable ref
	entry_ref ref;
	if (fInitError == B_OK)
		fInitError = BPrivate::get_app_ref(&ref);
	// get the BAppFileInfo and extract the information we need
	uint32 appFlags = B_REG_DEFAULT_APP_FLAGS;
	if (fInitError == B_OK) {
		BAppFileInfo fileInfo;
		BFile file(&ref, B_READ_ONLY);
		fInitError = fileInfo.SetTo(&file);
		if (fInitError == B_OK) {
			fileInfo.GetAppFlags(&appFlags);
			char appFileSignature[B_MIME_TYPE_LENGTH + 1];
			// compare the file signature and the supplied signature
			if (fileInfo.GetSignature(appFileSignature) == B_OK) {
				if (strcasecmp(appFileSignature, signature) != 0) {
					printf("Signature in rsrc doesn't match constructor arg. "
						   "(%s,%s)\n", signature, appFileSignature);
				}
			}
		}
	}
#ifndef RUN_WITHOUT_REGISTRAR
	// check whether be_roster is valid
	if (fInitError == B_OK && !isRegistrar
		&& !BRoster::Private().IsMessengerValid(false)) {
		printf("FATAL: be_roster is not valid. Is the registrar running?\n");
		fInitError = B_NO_INIT;
	}

	// check whether or not we are pre-registered
	bool preRegistered = false;
	app_info appInfo;
	if (fInitError == B_OK && !isRegistrar) {
		preRegistered = BRoster::Private().IsAppPreRegistered(&ref, team,
															  &appInfo);
	}
	if (preRegistered) {
		// we are pre-registered => the app info has been filled in
		// Check whether we need to replace the looper port with a port
		// created by the roster.
		if (appInfo.port >= 0 && appInfo.port != fMsgPort) {
			delete_port(fMsgPort);
			fMsgPort = appInfo.port;
		} else
			appInfo.port = fMsgPort;
		// check the signature and correct it, if necessary
		if (strcasecmp(appInfo.signature, fAppName))
			BRoster::Private().SetSignature(team, fAppName);
		// complete the registration
		fInitError = BRoster::Private().CompleteRegistration(team, thread,
															 appInfo.port);
	} else if (fInitError == B_OK) {
		// not pre-registered -- try to register the application
		team_id otherTeam = -1;
		// the registrar must not register
		if (!isRegistrar) {
			fInitError = BRoster::Private().AddApplication(signature, &ref,
				appFlags, team, thread, fMsgPort, true, NULL, &otherTeam);
		}
		if (fInitError == B_ALREADY_RUNNING) {
			// An instance is already running and we asked for
			// single/exclusive launch. Send our argv to the running app.
			// Do that only, if the app is NOT B_ARGV_ONLY.
			if (otherTeam >= 0 && __libc_argc > 1) {
				app_info otherAppInfo;
				if (be_roster->GetRunningAppInfo(otherTeam, &otherAppInfo)
					== B_OK && !(otherAppInfo.flags & B_ARGV_ONLY)) {
					// create an B_ARGV_RECEIVED message
					BMessage argvMessage(B_ARGV_RECEIVED);
					do_argv(&argvMessage);
					// replace the first argv string with the path of the
					// other application
					BPath path;
					if (path.SetTo(&otherAppInfo.ref) == B_OK)
						argvMessage.ReplaceString("argv", 0, path.Path());
					// send the message
					BMessenger(NULL, otherTeam).SendMessage(&argvMessage);
				}
			}
		} else if (fInitError == B_OK) {
			// the registrations was successful
			// Create a B_ARGV_RECEIVED message and send it to ourselves.
			// Do that even, if we are B_ARGV_ONLY.
			// TODO: When BLooper::AddMessage() is done, use that instead of
			// PostMessage().
			
			DBG(OUT("info: BApplication sucessfully registered.\n"));
			
			if (__libc_argc > 1) {
				BMessage argvMessage(B_ARGV_RECEIVED);
				do_argv(&argvMessage);
				PostMessage(&argvMessage, this);
			}
			// send a B_READY_TO_RUN message as well
			PostMessage(B_READY_TO_RUN, this);
		} else if (fInitError > B_ERRORS_END) {
			// Registrar internal errors shouldn't fall into the user's hands.
			fInitError = B_ERROR;
		}
	}
#endif	// ifdef RUN_WITHOUT_REGISTRAR

	// TODO: Not completely sure about the order, but this should be close.
	
	// An app_server connection is necessary for a lot of stuff, so get that first.
	if (fInitError == B_OK)
		connect_to_app_server();
	if (fInitError == B_OK)
		setup_server_heaps();
	if (fInitError == B_OK)
		get_scs();

	
	// init be_app and be_app_messenger
	if (fInitError == B_OK) {
		be_app = this;
		be_app_messenger = BMessenger(NULL, this);
	}
	
	// Initialize the IK after we have set be_app because of a construction of a
	// BAppServerLink (which depends on be_app) nested inside the call to get_menu_info.
	if (fInitError == B_OK)
		fInitError = _init_interface_kit_();
	// set the BHandler's name
	if (fInitError == B_OK)
		SetName(ref.name);
	// create meta MIME
	if (fInitError == B_OK) {
		BPath path;
		if (path.SetTo(&ref) == B_OK)
			create_app_meta_mime(path.Path(), false, true, false);
	}

	// create global system cursors
	// ToDo: these could have a predefined server token to safe the communication!
	B_CURSOR_SYSTEM_DEFAULT = new BCursor(B_HAND_CURSOR);
	B_CURSOR_I_BEAM = new BCursor(B_I_BEAM_CURSOR);

	// Return the error or exit, if there was an error and no error variable
	// has been supplied.
	if (error)
		*error = fInitError;
	else if (fInitError != B_OK)
		exit(0);
}
//------------------------------------------------------------------------------
void BApplication::BeginRectTracking(BRect r, bool trackWhole)
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_BEGIN_RECT_TRACKING);
	link.Attach<BRect>(r);
	link.Attach<int32>(trackWhole);
	link.Flush();
}
//------------------------------------------------------------------------------
void BApplication::EndRectTracking()
{
	BPrivate::BAppServerLink link;
	link.StartMessage(AS_END_RECT_TRACKING);
	link.Flush();
}
//------------------------------------------------------------------------------
void BApplication::get_scs()
{
	gPrivateScreen = new BPrivateScreen();
}
//------------------------------------------------------------------------------
void BApplication::setup_server_heaps()
{
	// TODO: implement?

	// We may not need to implement this function or the XX_offs_to_ptr functions.
	// R5 sets up a couple of areas for various tasks having to do with the
	// app_server. Currently (7/29/04), the R1 app_server does not do this and
	// may never do this unless a significant need is found for it. --DW
}
//------------------------------------------------------------------------------
void* BApplication::rw_offs_to_ptr(uint32 offset)
{
	return NULL;	// TODO: implement
}
//------------------------------------------------------------------------------
void* BApplication::ro_offs_to_ptr(uint32 offset)
{
	return NULL;	// TODO: implement
}
//------------------------------------------------------------------------------
void* BApplication::global_ro_offs_to_ptr(uint32 offset)
{
	return NULL;	// TODO: implement
}
//------------------------------------------------------------------------------
void BApplication::connect_to_app_server()
{
	fServerFrom = find_port(SERVER_PORT_NAME);
	if (fServerFrom >= 0) {
		// Create the port so that the app_server knows where to send messages
		fServerTo = create_port(100, "a<fServerTo");
		if (fServerTo >= 0) {
			
			//We can't use BAppServerLink because be_app == NULL
			
			// AS_CREATE_APP:
	
			// Attach data:
			// 1) port_id - receiver port of a regular app
			// 2) port_id - looper port for this BApplication
			// 3) team_id - team identification field
			// 4) int32 - handler ID token of the app
			// 5) char * - signature of the regular app
			BPortLink link(fServerFrom,fServerTo);
			int32 code=SERVER_FALSE;
			
			link.StartMessage(AS_CREATE_APP);
			link.Attach<port_id>(fServerTo);
			link.Attach<port_id>(_get_looper_port_(this));
			link.Attach<team_id>(Team());
			link.Attach<int32>(_get_object_token_(this));
			link.AttachString(fAppName);
			link.Flush();
			link.GetNextReply(&code);

			// Reply code: AS_SET_SERVER_PORT
			// Reply data:
			//	1) port_id server-side application port (fServerFrom value)
			if(code==AS_SET_SERVER_PORT)
				link.Read<port_id>(&fServerFrom);
		
		} else
			fInitError = fServerTo;
	} else
		fInitError = fServerFrom;
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, BRect drag_rect, BHandler* reply_to)
{
	// TODO: implement
}
//------------------------------------------------------------------------------
void BApplication::send_drag(BMessage* msg, int32 vs_token, BPoint offset, int32 bitmap_token, drawing_mode dragMode, BHandler* reply_to)
{
	// TODO: implement
}
//------------------------------------------------------------------------------
void BApplication::write_drag(_BSession_* session, BMessage* a_message)
{
	// TODO: implement
}
//------------------------------------------------------------------------------
bool BApplication::quit_all_windows(bool force)
{
	return false;	// TODO: implement?
}
//------------------------------------------------------------------------------
bool BApplication::window_quit_loop(bool, bool)
{
	// TODO: Implement and use in BApplication::QuitRequested()
	return false;
}
//------------------------------------------------------------------------------
void BApplication::do_argv(BMessage* message)
{
	if (message) {
		int32 argc = __libc_argc;
		const char * const *argv = __libc_argv;
		// add argc
		message->AddInt32("argc", argc);
		// add argv
		for (int32 i = 0; i < argc; i++)
			message->AddString("argv", argv[i]);
		// add current working directory
		char cwd[B_PATH_NAME_LENGTH + 1];
		if (getcwd(cwd, B_PATH_NAME_LENGTH + 1))
			message->AddString("cwd", cwd);
	}
}
//------------------------------------------------------------------------------
uint32 BApplication::InitialWorkspace()
{
	return fInitialWorkspace;
}
//------------------------------------------------------------------------------
int32 BApplication::count_windows(bool incl_menus) const
{
	using namespace BPrivate;

	// Windows are BLoopers, so we can just check each BLooper to see if it's
	// a BWindow (or BMenuWindow)
	int32 count = 0;
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		BLooper* Looper = NULL;
		for (int32 i = 0; i < gLooperList.CountLoopers(); ++i)
		{
			Looper = gLooperList.LooperAt(i);
			if (dynamic_cast<BWindow*>(Looper))
			{
				if (incl_menus || dynamic_cast<BMenuWindow*>(Looper) == NULL)
				{
					++count;
				}
			}
		}
	}

	return count;
}
//------------------------------------------------------------------------------
BWindow* BApplication::window_at(uint32 index, bool incl_menus) const
{
	using namespace BPrivate;

	// Windows are BLoopers, so we can just check each BLooper to see if it's
	// a BWindow (or BMenuWindow)
	uint32 count = 0;
	BWindow* Window = NULL;
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		BLooper* Looper = NULL;
		for (int32 i = 0; i < gLooperList.CountLoopers() && !Window; ++i)
		{
			Looper = gLooperList.LooperAt(i);
			if (dynamic_cast<BWindow*>(Looper))
			{
				if (incl_menus || dynamic_cast<BMenuWindow*>(Looper) == NULL)
				{
					if (count == index)
					{
						Window = dynamic_cast<BWindow*>(Looper);
					}
					else
					{
						++count;
					}
				}
			}
		}
	}

	return Window;
}
//------------------------------------------------------------------------------
status_t BApplication::get_window_list(BList* list, bool incl_menus) const
{
	return NOT_IMPLEMENTED;
}
//------------------------------------------------------------------------------
int32 BApplication::async_quit_entry(void* data)
{
	return 0;	// TODO: implement? not implemented?
}
//------------------------------------------------------------------------------

// check_app_signature
/*!	\brief Checks whether the supplied string is a valid application signature.

	An error message is printed, if the string is no valid app signature.

	\param signature The string to be checked.
	\return
	- \c B_OK: \a signature is a valid app signature.
	- \c B_BAD_VALUE: \a signature is \c NULL or no valid app signature.
*/
static
status_t
check_app_signature(const char *signature)
{
	bool isValid = false;
	BMimeType type(signature);
	if (type.IsValid() && !type.IsSupertypeOnly()
		&& BMimeType("application").Contains(&type)) {
		isValid = true;
	}
	if (!isValid) {
		printf("bad signature (%s), must begin with \"application/\" and "
			   "can't conflict with existing registered mime types inside "
			   "the \"application\" media type.\n", signature);
	}
	return (isValid ? B_OK : B_BAD_VALUE);
}

// looper_name_for
/*!	\brief Returns the looper name for a given signature.

	Normally this is "AppLooperPort", but in case of the registrar a
	special name.

	\return The looper name.
*/
static
const char*
looper_name_for(const char *signature)
{
	if (signature && !strcasecmp(signature, kRegistrarSignature))
		return kRosterPortName;
	return "AppLooperPort";
}


/*
 * $Log $
 *
 * $Id  $
 *
 */

