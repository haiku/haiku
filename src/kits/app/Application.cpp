/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 * 		Jerome Duval
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>

#include <new>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Cursor.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <Locker.h>
#include <MessageRunner.h>
#include <ObjectList.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <RegistrarDefs.h>
#include <Resources.h>
#include <Roster.h>
#include <Window.h>

#include <AppMisc.h>
#include <AppServerLink.h>
#include <AutoLocker.h>
#include <BitmapPrivate.h>
#include <DraggerPrivate.h>
#include <LooperList.h>
#include <MenuWindow.h>
#include <PicturePrivate.h>
#include <PortLink.h>
#include <RosterPrivate.h>
#include <ServerMemoryAllocator.h>
#include <ServerProtocol.h>


using namespace BPrivate;


BApplication *be_app = NULL;
BMessenger be_app_messenger;

pthread_once_t sAppResourcesInitOnce = PTHREAD_ONCE_INIT;
BResources *BApplication::sAppResources = NULL;


enum {
	kWindowByIndex,
	kWindowByName,
	kLooperByIndex,
	kLooperByID,
	kLooperByName,
	kApplication
};

static property_info sPropertyInfo[] = {
	{
		"Window",
		{},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		NULL, kWindowByIndex,
		{},
		{},
		{}
	},
	{
		"Window",
		{},
		{B_NAME_SPECIFIER},
		NULL, kWindowByName,
		{},
		{},
		{}
	},
	{
		"Looper",
		{},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		NULL, kLooperByIndex,
		{},
		{},
		{}
	},
	{
		"Looper",
		{},
		{B_ID_SPECIFIER},
		NULL, kLooperByID,
		{},
		{},
		{}
	},
	{
		"Looper",
		{},
		{B_NAME_SPECIFIER},
		NULL, kLooperByName,
		{},
		{},
		{}
	},
	{
		"Name",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_STRING_TYPE},
		{},
		{}
	},
	{
		"Window",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_INT32_TYPE},
		{},
		{}
	},
	{
		"Loopers",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_MESSENGER_TYPE},
		{},
		{}
	},
	{
		"Windows",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_MESSENGER_TYPE},
		{},
		{}
	},
	{
		"Looper",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_INT32_TYPE},
		{},
		{}
	},
	{}
};

// argc/argv
extern const int __libc_argc;
extern const char * const *__libc_argv;


// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf


// prototypes of helper functions
static const char* looper_name_for(const char *signature);
static status_t check_app_signature(const char *signature);
static void fill_argv_message(BMessage &message);


BApplication::BApplication(const char *signature)
	: BLooper(looper_name_for(signature))
{
	_InitData(signature, true, NULL);
}


BApplication::BApplication(const char *signature, status_t *_error)
	: BLooper(looper_name_for(signature))
{
	_InitData(signature, true, _error);
}


BApplication::BApplication(const char *signature, bool initGUI,
		status_t *_error)
	: BLooper(looper_name_for(signature))
{
	_InitData(signature, initGUI, _error);
}


BApplication::BApplication(BMessage *data)
	// Note: BeOS calls the private BLooper(int32, port_id, const char *)
	// constructor here, test if it's needed
	: BLooper(looper_name_for(NULL))
{
	const char *signature = NULL;
	data->FindString("mime_sig", &signature);

	_InitData(signature, true, NULL);

	bigtime_t pulseRate;
	if (data->FindInt64("_pulse", &pulseRate) == B_OK)
		SetPulseRate(pulseRate);
}


BApplication::BApplication(uint32 signature)
{
}


BApplication::BApplication(const BApplication &rhs)
{
}


BApplication::~BApplication()
{
	Lock();

	// tell all loopers(usually windows) to quit. Also, wait for them.
	_QuitAllWindows(true);

	// unregister from the roster
	BRoster::Private().RemoveApp(Team());

#ifndef RUN_WITHOUT_APP_SERVER
	// tell app_server we're quitting...
	if (be_app) {
		// be_app can be NULL here if the application fails to initialize
		// correctly. For example, if it's already running and it's set to
		// exclusive launch.
		BPrivate::AppServerLink link;
		link.StartMessage(B_QUIT_REQUESTED);
		link.Flush();
	}
	delete_port(fServerLink->SenderPort());
	delete_port(fServerLink->ReceiverPort());
	delete fServerLink;
#endif	// RUN_WITHOUT_APP_SERVER

	delete fServerAllocator;

	// uninitialize be_app, the be_app_messenger is invalidated automatically
	be_app = NULL;
}


BApplication &
BApplication::operator=(const BApplication &rhs)
{
	return *this;
}


void
BApplication::_InitData(const char *signature, bool initGUI, status_t *_error)
{
	DBG(OUT("BApplication::InitData(`%s', %p)\n", signature, _error));
	// check whether there exists already an application
	if (be_app)
		debugger("2 BApplication objects were created. Only one is allowed.");

	fServerLink = new BPrivate::PortLink(-1, -1);
	fServerAllocator = NULL;
	fInitialWorkspace = 0;
	//fDraggedMessage = NULL;
	fReadyToRunCalled = false;

	// initially, there is no pulse
	fPulseRunner = NULL;
	fPulseRate = 0;

	// check signature
	fInitError = check_app_signature(signature);
	fAppName = signature;

#ifndef RUN_WITHOUT_REGISTRAR
	bool isRegistrar = signature
		&& !strcasecmp(signature, kRegistrarSignature);
	// get team and thread
	team_id team = Team();
	thread_id thread = BPrivate::main_thread_for(team);
#endif

	// get app executable ref
	entry_ref ref;
	if (fInitError == B_OK) {
		fInitError = BPrivate::get_app_ref(&ref);
		if (fInitError != B_OK) {
			DBG(OUT("BApplication::InitData(): Failed to get app ref: %s\n",
				strerror(fInitError)));
		}
	}

	// get the BAppFileInfo and extract the information we need
	uint32 appFlags = B_REG_DEFAULT_APP_FLAGS;
	if (fInitError == B_OK) {
		BAppFileInfo fileInfo;
		BFile file(&ref, B_READ_ONLY);
		fInitError = fileInfo.SetTo(&file);
		if (fInitError == B_OK) {
			fileInfo.GetAppFlags(&appFlags);
			char appFileSignature[B_MIME_TYPE_LENGTH];
			// compare the file signature and the supplied signature
			if (fileInfo.GetSignature(appFileSignature) == B_OK
				&& strcasecmp(appFileSignature, signature) != 0) {
				printf("Signature in rsrc doesn't match constructor arg. (%s, %s)\n",
					signature, appFileSignature);
			}
		} else {
			DBG(OUT("BApplication::InitData(): Failed to get info from: "
				"BAppFileInfo: %s\n", strerror(fInitError)));
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
		if (BRoster::Private().IsAppRegistered(&ref, team, 0, &preRegistered,
				&appInfo) != B_OK) {
			preRegistered = false;
		}
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
		// check the signature and correct it, if necessary, also the case
		if (strcmp(appInfo.signature, fAppName))
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
			if (fInitError != B_OK) {
				DBG(OUT("BApplication::InitData(): Failed to add app: %s\n",
					strerror(fInitError)));
			}
		}
		if (fInitError == B_ALREADY_RUNNING) {
			// An instance is already running and we asked for
			// single/exclusive launch. Send our argv to the running app.
			// Do that only, if the app is NOT B_ARGV_ONLY.
			if (otherTeam >= 0) {
				BMessenger otherApp(NULL, otherTeam);
				app_info otherAppInfo;
				if (__libc_argc > 1
					&& be_roster->GetRunningAppInfo(otherTeam, &otherAppInfo) == B_OK
					&& !(otherAppInfo.flags & B_ARGV_ONLY)) {
					// create an B_ARGV_RECEIVED message
					BMessage argvMessage(B_ARGV_RECEIVED);
					fill_argv_message(argvMessage);

					// replace the first argv string with the path of the
					// other application
					BPath path;
					if (path.SetTo(&otherAppInfo.ref) == B_OK)
						argvMessage.ReplaceString("argv", 0, path.Path());

					// send the message
					otherApp.SendMessage(&argvMessage);
				} else
					otherApp.SendMessage(B_SILENT_RELAUNCH);
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
				fill_argv_message(argvMessage);
				PostMessage(&argvMessage, this);
			}
			// send a B_READY_TO_RUN message as well
			PostMessage(B_READY_TO_RUN, this);
		} else if (fInitError > B_ERRORS_END) {
			// Registrar internal errors shouldn't fall into the user's hands.
			fInitError = B_ERROR;
		}
	}
#else
	// We need to have ReadyToRun called even when we're not using the registrar
	PostMessage(B_READY_TO_RUN, this);
#endif	// ifndef RUN_WITHOUT_REGISTRAR

	if (fInitError == B_OK) {
		// TODO: Not completely sure about the order, but this should be close.

		// init be_app and be_app_messenger
		be_app = this;
		be_app_messenger = BMessenger(NULL, this);

		// set the BHandler's name
		SetName(ref.name);

		// create meta MIME
		BPath path;
		if (path.SetTo(&ref) == B_OK)
			create_app_meta_mime(path.Path(), false, true, false);

#ifndef RUN_WITHOUT_APP_SERVER
		// app server connection and IK initialization
		if (initGUI)
			fInitError = _InitGUIContext();
#endif	// RUN_WITHOUT_APP_SERVER
	}

	// Return the error or exit, if there was an error and no error variable
	// has been supplied.
	if (_error) {
		*_error = fInitError;
	} else if (fInitError != B_OK) {
		DBG(OUT("BApplication::InitData() failed: %s\n", strerror(fInitError)));
		exit(0);
	}
DBG(OUT("BApplication::InitData() done\n"));
}


BArchivable *
BApplication::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BApplication"))
		return new BApplication(data);

	return NULL;
}


status_t
BApplication::Archive(BMessage *data, bool deep) const
{
	status_t status = BLooper::Archive(data, deep);
	if (status < B_OK)
		return status;

	app_info info;
	status = GetAppInfo(&info);
	if (status < B_OK)
		return status;

	status = data->AddString("mime_sig", info.signature);
	if (status < B_OK)
		return status;

	return data->AddInt64("_pulse", fPulseRate);
}


status_t
BApplication::InitCheck() const
{
	return fInitError;
}


thread_id
BApplication::Run()
{
	if (fInitError != B_OK)
		return fInitError;

	AssertLocked();

	if (fRunCalled)
		debugger("BApplication::Run was already called. Can only be called once.");

	fThread = find_thread(NULL);
	fRunCalled = true;

	task_looper();

	delete fPulseRunner;
	return fThread;
}


void
BApplication::Quit()
{
	bool unlock = false;
	if (!IsLocked()) {
		const char *name = Name();
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
	} else if (find_thread(NULL) != fThread) {
// ToDo: why shouldn't we set fTerminating to true directly in this case?
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


bool
BApplication::QuitRequested()
{
	return _QuitAllWindows(false);
}


void
BApplication::Pulse()
{
	// supposed to be implemented by subclasses
}


void
BApplication::ReadyToRun()
{
	// supposed to be implemented by subclasses
}


void
BApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_COUNT_PROPERTIES:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			int32 index;
			BMessage specifier;
			int32 what;
			const char *property = NULL;
			if (message->GetCurrentSpecifier(&index, &specifier, &what, &property) < B_OK
				|| !ScriptReceived(message, index, &specifier, what, property))
				BLooper::MessageReceived(message);
			break;
		}

		case B_SILENT_RELAUNCH:
			// Sent to a B_SINGLE_LAUNCH application when it's launched again
			// (see _InitData())
			be_roster->ActivateApp(Team());
			break;

		case kMsgAppServerRestarted:
		{
			_ReconnectToServer();
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
BApplication::ArgvReceived(int32 argc, char **argv)
{
	// supposed to be implemented by subclasses
}


void
BApplication::AppActivated(bool active)
{
	// supposed to be implemented by subclasses
}


void
BApplication::RefsReceived(BMessage *message)
{
	// supposed to be implemented by subclasses
}


void
BApplication::AboutRequested()
{
	thread_info info;
	if (get_thread_info(Thread(), &info) == B_OK) {
		BAlert *alert = new BAlert("_about_", info.name, "OK");
		alert->Go(NULL);
	}
}


BHandler *
BApplication::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	BPropertyInfo propInfo(sPropertyInfo);
	status_t err = B_OK;
	uint32 data;

	if (propInfo.FindMatch(message, 0, specifier, what, property, &data) >= 0) {
		switch (data) {
			case kWindowByIndex:
			{
				int32 index;
				err = specifier->FindInt32("index", &index);
				if (err != B_OK)
					break;

				if (what == B_REVERSE_INDEX_SPECIFIER)
					index = CountWindows() - index;

				BWindow *window = WindowAt(index);
				if (window != NULL) {
					message->PopSpecifier();
					BMessenger(window).SendMessage(message);
				} else
					err = B_BAD_INDEX;
				break;
			}

			case kWindowByName:
			{
				const char *name;
				err = specifier->FindString("name", &name);
				if (err != B_OK)
					break;

				for (int32 i = 0;; i++) {
					BWindow *window = WindowAt(i);
					if (window == NULL) {
						err = B_NAME_NOT_FOUND;
						break;
					}
					if (window->Title() != NULL && !strcmp(window->Title(), name)) {
						message->PopSpecifier();
						BMessenger(window).SendMessage(message);
						break;
					}
				}
				break;
			}

			case kLooperByIndex:
			{
				int32 index;
				err = specifier->FindInt32("index", &index);
				if (err != B_OK)
					break;

				if (what == B_REVERSE_INDEX_SPECIFIER)
					index = CountLoopers() - index;

				BLooper *looper = LooperAt(index);
				if (looper != NULL) {
					message->PopSpecifier();
					BMessenger(looper).SendMessage(message);
				} else
					err = B_BAD_INDEX;
				break;
			}

			case kLooperByID:
				// TODO: implement getting looper by ID!
				break;

			case kLooperByName:
			{
				const char *name;
				err = specifier->FindString("name", &name);
				if (err != B_OK)
					break;

				for (int32 i = 0;; i++) {
					BLooper *looper = LooperAt(i);
					if (looper == NULL) {
						err = B_NAME_NOT_FOUND;
						break;
					}
					if (looper->Name() != NULL && !strcmp(looper->Name(), name)) {
						message->PopSpecifier();
						BMessenger(looper).SendMessage(message);
						break;
					}
				}
				break;
			}

			case kApplication:
				return this;
		}
	} else {
		return BLooper::ResolveSpecifier(message, index, specifier, what,
			property);
	}

	if (err != B_OK) {
		BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);
		reply.AddInt32("error", err);
		reply.AddString("message", strerror(err));
		message->SendReply(&reply);
	}

	return NULL;

}


void
BApplication::ShowCursor()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SHOW_CURSOR);
	link.Flush();
}


void
BApplication::HideCursor()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_HIDE_CURSOR);
	link.Flush();
}


void
BApplication::ObscureCursor()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_OBSCURE_CURSOR);
	link.Flush();
}


bool
BApplication::IsCursorHidden() const
{
	BPrivate::AppServerLink link;
	int32 status = B_ERROR;
	link.StartMessage(AS_QUERY_CURSOR_HIDDEN);
	link.FlushWithReply(status);

	return status == B_OK;
}


void
BApplication::SetCursor(const void *cursorData)
{
	BCursor cursor(cursorData);
	SetCursor(&cursor, true);
		// forces the cursor to be sync'ed
}


void
BApplication::SetCursor(const BCursor *cursor, bool sync)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_CURSOR);
	link.Attach<bool>(sync);
	link.Attach<int32>(cursor->fServerToken);

	if (sync) {
		int32 code;
		link.FlushWithReply(code);
	} else
		link.Flush();
}


int32
BApplication::CountWindows() const
{
	return _CountWindows(false);
		// we're ignoring menu windows
}


BWindow *
BApplication::WindowAt(int32 index) const
{
	return _WindowAt(index, false);
		// we're ignoring menu windows
}


int32
BApplication::CountLoopers() const
{
	AutoLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
		return gLooperList.CountLoopers();

	// Some bad, non-specific thing has happened
	return B_ERROR;
}


BLooper *
BApplication::LooperAt(int32 index) const
{
	BLooper *looper = NULL;
	AutoLocker<BLooperList> listLock(gLooperList);
	if (listLock.IsLocked())
		looper = gLooperList.LooperAt(index);

	return looper;
}


bool
BApplication::IsLaunching() const
{
	return !fReadyToRunCalled;
}


status_t
BApplication::GetAppInfo(app_info *info) const
{
	if (be_app == NULL || be_roster == NULL)
		return B_NO_INIT;
	return be_roster->GetRunningAppInfo(be_app->Team(), info);
}


BResources *
BApplication::AppResources()
{
	if (sAppResources == NULL)
		pthread_once(&sAppResourcesInitOnce, &_InitAppResources);

	return sAppResources;
}


void
BApplication::DispatchMessage(BMessage *message, BHandler *handler)
{
	if (handler != this) {
		// it's not ours to dispatch
		BLooper::DispatchMessage(message, handler);
		return;
	}

	switch (message->what) {
		case B_ARGV_RECEIVED:
			_ArgvReceived(message);
			break;

		case B_REFS_RECEIVED:
		{
			// this adds the refs that are part of this message to the recent
			// lists, but only folders and documents are handled here
			entry_ref ref;
			int32 i = 0;
			while (message->FindRef("refs", i++, &ref) == B_OK) {
				BEntry entry(&ref, true);
				if (entry.InitCheck() != B_OK)
					continue;

				if (entry.IsDirectory())
					BRoster().AddToRecentFolders(&ref);
				else {
					// filter out applications, we only want to have documents
					// in the recent files list
					BNode node(&entry);
					BNodeInfo info(&node);

					char mimeType[B_MIME_TYPE_LENGTH];
					if (info.GetType(mimeType) != B_OK
						|| strcasecmp(mimeType, B_APP_MIME_TYPE))
						BRoster().AddToRecentDocuments(&ref);
				}
			}

			RefsReceived(message);
			break;
		}

		case B_READY_TO_RUN:
			if (!fReadyToRunCalled) {
				ReadyToRun();
				fReadyToRunCalled = true;
			}
			break;

		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;

		case B_PULSE:
			Pulse();
			break;

		case B_APP_ACTIVATED:
		{
			bool active;
			if (message->FindBool("active", &active) == B_OK)
				AppActivated(active);
			break;
		}

		case _SHOW_DRAG_HANDLES_:
		{
			bool show;
			if (message->FindBool("show", &show) != B_OK)
				break;

			BDragger::Private::UpdateShowAllDraggers(show);
			break;
		}

		// TODO: Handle these as well
		case _DISPOSE_DRAG_:
		case _PING_:
			puts("not yet handled message:");
			DBG(message->PrintToStream());
			break;

		default:
			BLooper::DispatchMessage(message, handler);
			break;
	}
}


void
BApplication::SetPulseRate(bigtime_t rate)
{
	if (rate < 0)
		rate = 0;

	// BeBook states that we have only 100,000 microseconds granularity
	rate -= rate % 100000;

	if (!Lock())
		return;

	if (rate != 0) {
		// reset existing pulse runner, or create new one
		if (fPulseRunner == NULL) {
			BMessage pulse(B_PULSE);
			fPulseRunner = new BMessageRunner(be_app_messenger, &pulse, rate);
		} else
			fPulseRunner->SetInterval(rate);
	} else {
		// turn off pulse messages
		delete fPulseRunner;
		fPulseRunner = NULL;
	}

	fPulseRate = rate;
	Unlock();
}


status_t
BApplication::GetSupportedSuites(BMessage *data)
{
	if (!data)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-application");
	if (status == B_OK) {
		BPropertyInfo propertyInfo(sPropertyInfo);
		status = data->AddFlat("messages", &propertyInfo);
		if (status == B_OK)
			status = BLooper::GetSupportedSuites(data);
	}

	return status;
}


status_t
BApplication::Perform(perform_code d, void *arg)
{
	return BLooper::Perform(d, arg);
}


void BApplication::_ReservedApplication1() {}
void BApplication::_ReservedApplication2() {}
void BApplication::_ReservedApplication3() {}
void BApplication::_ReservedApplication4() {}
void BApplication::_ReservedApplication5() {}
void BApplication::_ReservedApplication6() {}
void BApplication::_ReservedApplication7() {}
void BApplication::_ReservedApplication8() {}


bool
BApplication::ScriptReceived(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	BMessage reply(B_REPLY);
	status_t err = B_BAD_SCRIPT_SYNTAX;

	switch (message->what) {
		case B_GET_PROPERTY:
			if (strcmp("Loopers", property) == 0) {
				int32 count = CountLoopers();
				err = B_OK;
				for (int32 i=0; err == B_OK && i<count; i++) {
					BMessenger messenger(LooperAt(i));
					err = reply.AddMessenger("result", messenger);
				}
			} else if (strcmp("Windows", property) == 0) {
				int32 count = CountWindows();
				err = B_OK;
				for (int32 i=0; err == B_OK && i<count; i++) {
					BMessenger messenger(WindowAt(i));
					err = reply.AddMessenger("result", messenger);
				}
			} else if (strcmp("Window", property) == 0) {
				switch (what) {
					case B_INDEX_SPECIFIER:
					case B_REVERSE_INDEX_SPECIFIER:
					{
						int32 index = -1;
						err = specifier->FindInt32("index", &index);
						if (err != B_OK)
							break;
						if (what == B_REVERSE_INDEX_SPECIFIER)
							index = CountWindows() - index;
						err = B_BAD_INDEX;
						BWindow *win = WindowAt(index);
						if (!win)
							break;
						BMessenger messenger(win);
						err = reply.AddMessenger("result", messenger);
						break;
					}
					case B_NAME_SPECIFIER:
					{
						const char *name;
						err = specifier->FindString("name", &name);
						if (err != B_OK)
							break;
						err = B_NAME_NOT_FOUND;
						for (int32 i = 0; i < CountWindows(); i++) {
							BWindow* window = WindowAt(i);
							if (window && window->Name() != NULL
								&& !strcmp(window->Name(), name)) {
								BMessenger messenger(window);
								err = reply.AddMessenger("result", messenger);
								break;
							}
						}
						break;
					}
				}
			} else if (strcmp("Looper", property) == 0) {
				switch (what) {
					case B_INDEX_SPECIFIER:
					case B_REVERSE_INDEX_SPECIFIER:
					{
						int32 index = -1;
						err = specifier->FindInt32("index", &index);
						if (err != B_OK)
							break;
						if (what == B_REVERSE_INDEX_SPECIFIER)
							index = CountLoopers() - index;
						err = B_BAD_INDEX;
						BLooper *looper = LooperAt(index);
						if (!looper)
							break;
						BMessenger messenger(looper);
						err = reply.AddMessenger("result", messenger);
						break;
					}
					case B_NAME_SPECIFIER:
					{
						const char *name;
						err = specifier->FindString("name", &name);
						if (err != B_OK)
							break;
						err = B_NAME_NOT_FOUND;
						for (int32 i = 0; i < CountLoopers(); i++) {
							BLooper *looper = LooperAt(i);
							if (looper && looper->Name()
								&& !strcmp(looper->Name(), name)) {
								BMessenger messenger(looper);
								err = reply.AddMessenger("result", messenger);
								break;
							}
						}
						break;
					}
					case B_ID_SPECIFIER:
					{
						// TODO
						debug_printf("Looper's ID specifier used but not implemented.\n");
						break;
					}
				}
			} else if (strcmp("Name", property) == 0) {
				err = reply.AddString("result", Name());
			}
			break;
		case B_COUNT_PROPERTIES:
			if (strcmp("Looper", property) == 0) {
				err = reply.AddInt32("result", CountLoopers());
			} else if (strcmp("Window", property) == 0) {
				err = reply.AddInt32("result", CountWindows());
			}
			break;
	}
	if (err == B_BAD_SCRIPT_SYNTAX)
		return false;

	if (err < B_OK) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", strerror(err));
	}
	reply.AddInt32("error", err);
	message->SendReply(&reply);
	return true;
}


void
BApplication::BeginRectTracking(BRect rect, bool trackWhole)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_BEGIN_RECT_TRACKING);
	link.Attach<BRect>(rect);
	link.Attach<int32>(trackWhole);
	link.Flush();
}


void
BApplication::EndRectTracking()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_END_RECT_TRACKING);
	link.Flush();
}


status_t
BApplication::_SetupServerAllocator()
{
	fServerAllocator = new (std::nothrow) BPrivate::ServerMemoryAllocator();
	if (fServerAllocator == NULL)
		return B_NO_MEMORY;

	return fServerAllocator->InitCheck();
}


status_t
BApplication::_InitGUIContext()
{
	// An app_server connection is necessary for a lot of stuff, so get that first.
	status_t error = _ConnectToServer();
	if (error != B_OK)
		return error;

	// Initialize the IK after we have set be_app because of a construction
	// of a AppServerLink (which depends on be_app) nested inside the call
	// to get_menu_info.
	error = _init_interface_kit_();
	if (error != B_OK)
		return error;

	// create global system cursors
	B_CURSOR_SYSTEM_DEFAULT = new BCursor(B_HAND_CURSOR);
	B_CURSOR_I_BEAM = new BCursor(B_I_BEAM_CURSOR);

	// TODO: would be nice to get the workspace at launch time from the registrar
	fInitialWorkspace = current_workspace();

	return B_OK;
}


status_t
BApplication::_ConnectToServer()
{
	status_t status
		= create_desktop_connection(fServerLink, "a<app_server", 100);
	if (status != B_OK)
		return status;

	// AS_CREATE_APP:
	//
	// Attach data:
	// 1) port_id - receiver port of a regular app
	// 2) port_id - looper port for this BApplication
	// 3) team_id - team identification field
	// 4) int32 - handler ID token of the app
	// 5) char * - signature of the regular app

	fServerLink->StartMessage(AS_CREATE_APP);
	fServerLink->Attach<port_id>(fServerLink->ReceiverPort());
	fServerLink->Attach<port_id>(_get_looper_port_(this));
	fServerLink->Attach<team_id>(Team());
	fServerLink->Attach<int32>(_get_object_token_(this));
	fServerLink->AttachString(fAppName);

	area_id sharedReadOnlyArea;
	team_id serverTeam;
	port_id serverPort;

	int32 code;
	if (fServerLink->FlushWithReply(code) == B_OK
		&& code == B_OK) {
		// We don't need to contact the main app_server anymore
		// directly; we now talk to our server alter ego only.
		fServerLink->Read<port_id>(&serverPort);
		fServerLink->Read<area_id>(&sharedReadOnlyArea);
		fServerLink->Read<team_id>(&serverTeam);
	} else {
		fServerLink->SetSenderPort(-1);
		debugger("BApplication: couldn't obtain new app_server comm port");
		return B_ERROR;
	}
	fServerLink->SetTargetTeam(serverTeam);
	fServerLink->SetSenderPort(serverPort);

	status = _SetupServerAllocator();
	if (status != B_OK)
		return status;

	area_id area;
	uint8* base;
	status = fServerAllocator->AddArea(sharedReadOnlyArea, area, base, true);
	if (status < B_OK)
		return status;

	fServerReadOnlyMemory = base;
	return B_OK;
}


void
BApplication::_ReconnectToServer()
{
	delete_port(fServerLink->SenderPort());
	delete_port(fServerLink->ReceiverPort());
	invalidate_server_port();

	if (_ConnectToServer() != B_OK)
		debugger("Can't reconnect to app server!");

	AutoLocker<BLooperList> listLock(gLooperList);
	if (!listLock.IsLocked())
		return;

	uint32 count = gLooperList.CountLoopers();
	for (uint32 i = 0; i < count ; i++) {
		BWindow* window = dynamic_cast<BWindow*>(gLooperList.LooperAt(i));
		if (window == NULL)
			continue;
		BMessenger windowMessenger(window);
		windowMessenger.SendMessage(kMsgAppServerRestarted);
	}

	reconnect_bitmaps_to_app_server();
	reconnect_pictures_to_app_server();
}


#if 0
void
BApplication::send_drag(BMessage *message, int32 vs_token, BPoint offset,
	BRect dragRect, BHandler *replyTo)
{
	// TODO: implement
}


void
BApplication::send_drag(BMessage *message, int32 vs_token, BPoint offset,
	int32 bitmapToken, drawing_mode dragMode, BHandler *replyTo)
{
	// TODO: implement
}


void
BApplication::write_drag(_BSession_ *session, BMessage *message)
{
	// TODO: implement
}
#endif

bool
BApplication::_WindowQuitLoop(bool quitFilePanels, bool force)
{
	int32 index = 0;
	while (true) {
		 BWindow *window = WindowAt(index);
		 if (window == NULL)
		 	break;

		// NOTE: the window pointer might be stale, in case the looper
		// was already quit by quitting an earlier looper... but fortunately,
		// we can still call Lock() on the invalid pointer, and it
		// will return false...
		if (!window->Lock())
			continue;

		// don't quit file panels if we haven't been asked for it
		if (!quitFilePanels && window->IsFilePanel()) {
			window->Unlock();
			index++;
			continue;
		}

		if (!force && !window->QuitRequested()
			&& !(quitFilePanels && window->IsFilePanel())) {
			// the window does not want to quit, so we don't either
			window->Unlock();
			return false;
		}

		// Re-lock, just to make sure that the user hasn't done nasty
		// things in QuitRequested(). Quit() unlocks fully, thus
		// double-locking is harmless.
		if (window->Lock())
			window->Quit();

		index = 0;
			// we need to continue at the start of the list again - it
			// might have changed
	}
	return true;
}


bool
BApplication::_QuitAllWindows(bool force)
{
	AssertLocked();

	// We need to unlock here because BWindow::QuitRequested() must be
	// allowed to lock the application - which would cause a deadlock
	Unlock();

	bool quit = _WindowQuitLoop(false, force);
	if (quit)
		quit = _WindowQuitLoop(true, force);

	Lock();

	return quit;
}


void
BApplication::_ArgvReceived(BMessage *message)
{
	ASSERT(message != NULL);

	// build the argv vector
	status_t error = B_OK;
	int32 argc = 0;
	char **argv = NULL;
	if (message->FindInt32("argc", &argc) == B_OK && argc > 0) {
		// allocate a NULL terminated array
		argv = new(std::nothrow) char*[argc + 1];
		if (argv == NULL)
			return;

		// copy the arguments
		for (int32 i = 0; error == B_OK && i < argc; i++) {
			const char *arg = NULL;
			error = message->FindString("argv", i, &arg);
			if (error == B_OK && arg) {
				argv[i] = strdup(arg);
				if (argv[i] == NULL)
					error = B_NO_MEMORY;
			} else
				argc = i;
		}

		argv[argc] = NULL;
	}

	// call the hook
	if (error == B_OK && argc > 0)
		ArgvReceived(argc, argv);

	if (error != B_OK) {
		printf("Error parsing B_ARGV_RECEIVED message. Message:\n");
		message->PrintToStream();
	}

	// cleanup
	if (argv) {
		for (int32 i = 0; i < argc; i++)
			free(argv[i]);
		delete[] argv;
	}
}


uint32
BApplication::InitialWorkspace()
{
	return fInitialWorkspace;
}


int32
BApplication::_CountWindows(bool includeMenus) const
{
	uint32 count = 0;
	for (int32 i = 0; i < gLooperList.CountLoopers(); i++) {
		BWindow* window = dynamic_cast<BWindow*>(gLooperList.LooperAt(i));
		if (window != NULL && !window->fOffscreen && (includeMenus
				|| dynamic_cast<BMenuWindow *>(window) == NULL)) {
			count++;
		}
	}

	return count;
}


BWindow *
BApplication::_WindowAt(uint32 index, bool includeMenus) const
{
	AutoLocker<BLooperList> listLock(gLooperList);
	if (!listLock.IsLocked())
		return NULL;

	uint32 count = gLooperList.CountLoopers();
	for (uint32 i = 0; i < count && index < count; i++) {
		BWindow* window = dynamic_cast<BWindow*>(gLooperList.LooperAt(i));
		if (window == NULL || (window != NULL && window->fOffscreen)
			|| (!includeMenus && dynamic_cast<BMenuWindow *>(window) != NULL)) {
			index++;
			continue;
		}

		if (i == index)
			return window;
	}

	return NULL;
}


/*static*/ void
BApplication::_InitAppResources()
{
	entry_ref ref;
	bool found = false;

	// App is already running. Get its entry ref with
	// GetAppInfo()
	app_info appInfo;
	if (be_app && be_app->GetAppInfo(&appInfo) == B_OK) {
		ref = appInfo.ref;
		found = true;
	} else {
		// Run() hasn't been called yet
		found = BPrivate::get_app_ref(&ref) == B_OK;
	}

	if (!found)
		return;

	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	BResources* resources = new (std::nothrow) BResources(&file, false);
	if (resources == NULL || resources->InitCheck() != B_OK) {
		delete resources;
		return;
	}

	sAppResources = resources;
}


//	#pragma mark -


/*!
	\brief Checks whether the supplied string is a valid application signature.

	An error message is printed, if the string is no valid app signature.

	\param signature The string to be checked.
	\return
	- \c B_OK: \a signature is a valid app signature.
	- \c B_BAD_VALUE: \a signature is \c NULL or no valid app signature.
*/
static status_t
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


/*!
	\brief Returns the looper name for a given signature.

	Normally this is "AppLooperPort", but in case of the registrar a
	special name.

	\return The looper name.
*/
static const char *
looper_name_for(const char *signature)
{
	if (signature && !strcasecmp(signature, kRegistrarSignature))
		return BPrivate::get_roster_port_name();
	return "AppLooperPort";
}


/*!
	\brief Fills the passed BMessage with B_ARGV_RECEIVED infos.
*/
static void
fill_argv_message(BMessage &message)
{
   	message.what = B_ARGV_RECEIVED;

	int32 argc = __libc_argc;
	const char * const *argv = __libc_argv;

	// add argc
	message.AddInt32("argc", argc);

	// add argv
	for (int32 i = 0; i < argc; i++) {
		if (argv[i] != NULL)
			message.AddString("argv", argv[i]);
	}

	// add current working directory
	char cwd[B_PATH_NAME_LENGTH];
	if (getcwd(cwd, B_PATH_NAME_LENGTH))
		message.AddString("cwd", cwd);
}

