/*
 * Copyright 2006, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "IconEditorApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <IconEditorProtocol.h>
#include <Locale.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <storage/FindDirectory.h>

#include "support_settings.h"

#include "AutoLocker.h"
#include "Defines.h"
#include "MainWindow.h"
#include "SavePanel.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Main"


using std::nothrow;

static const char* kAppSig = "application/x-vnd.haiku-icon_o_matic";

static const float kWindowOffset = 20;


IconEditorApp::IconEditorApp()
	:
	BApplication(kAppSig),
	fWindowCount(0),
	fLastWindowFrame(50, 50, 900, 750),

	fOpenPanel(NULL),
	fSavePanel(NULL),

	fLastOpenPath(""),
	fLastSavePath(""),
	fLastExportPath("")
{
	// create file panels
	BMessenger messenger(this, this);
	BMessage message(B_REFS_RECEIVED);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE,
		true, &message);

	message.what = MSG_SAVE_AS;
	fSavePanel = new SavePanel("save panel", &messenger, NULL, B_FILE_NODE
		| B_DIRECTORY_NODE | B_SYMLINK_NODE, false, &message);

	_RestoreSettings();
}


IconEditorApp::~IconEditorApp()
{
	delete fOpenPanel;
	delete fSavePanel;
}


// #pragma mark -


bool
IconEditorApp::QuitRequested()
{
	// Run the QuitRequested() hook in each window's own thread. Otherwise
	// the BAlert which a window shows when an icon is not saved will not
	// repaint the window. (BAlerts check which thread is running Go() and
	// will repaint windows when it's a BWindow.)
	bool quit = true;
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		if (!window->Lock())
			continue;
		// Try to cast the window while the pointer must be valid.
		MainWindow* mainWindow = dynamic_cast<MainWindow*>(window);
		window->Unlock();
		if (mainWindow == NULL)
			continue;
		BMessenger messenger(window, window);
		BMessage reply;
		if (messenger.SendMessage(B_QUIT_REQUESTED, &reply) != B_OK)
			continue;
		bool result;
		if (reply.FindBool("result", &result) == B_OK && !result)
			quit = false;
	}

	if (!quit)
		return false;

	_StoreSettings();

	return true;
}


void
IconEditorApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW:
			_NewWindow()->Show();
			break;
		case MSG_OPEN:
		{
			BMessage openMessage(B_REFS_RECEIVED);
			MainWindow* window;
			if (message->FindPointer("window", (void**)&window) == B_OK)
				openMessage.AddPointer("window", window);
			fOpenPanel->SetMessage(&openMessage);
			fOpenPanel->Show();
			break;
		}
		case MSG_APPEND:
		{
			MainWindow* window;
			if (message->FindPointer("window", (void**)&window) != B_OK)
				break;
			BMessage openMessage(B_REFS_RECEIVED);
			openMessage.AddBool("append", true);
			openMessage.AddPointer("window", window);
			fOpenPanel->SetMessage(&openMessage);
			fOpenPanel->Show();
			break;
		}
		case B_EDIT_ICON_DATA:
		{
			BMessenger messenger;
			if (message->FindMessenger("reply to", &messenger) < B_OK) {
				// required
				break;
			}
			const uint8* data;
			ssize_t size;
			if (message->FindData("icon data", B_VECTOR_ICON_TYPE,
				(const void**)&data, &size) < B_OK) {
				// optional (new icon will be created)
				data = NULL;
				size = 0;
			}
			MainWindow* window = _NewWindow();
			window->Open(messenger, data, size);
			window->Show();
			break;
		}
		case MSG_SAVE_AS:
		case MSG_EXPORT_AS:
		{
			BMessenger messenger;
			if (message->FindMessenger("target", &messenger) != B_OK)
				break;

			fSavePanel->SetExportMode(message->what == MSG_EXPORT_AS);
//			fSavePanel->Refresh();
			const char* saveText;
			if (message->FindString("save text", &saveText) == B_OK)
				fSavePanel->SetSaveText(saveText);
			fSavePanel->SetTarget(messenger);
			fSavePanel->Show();
			break;
		}

		case MSG_WINDOW_CLOSED:
		{
			fWindowCount--;
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			BMessage settings;
			if (message->FindMessage("settings", &settings) == B_OK)
				fLastWindowSettings = settings;
			BRect frame;
			if (message->FindRect("window frame", &frame) == B_OK) {
				fLastWindowFrame = frame;
				fLastWindowFrame.OffsetBy(-kWindowOffset, -kWindowOffset);
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
IconEditorApp::ReadyToRun()
{
	// create main window
	if (fWindowCount == 0)
		_NewWindow()->Show();

	_InstallDocumentMimeType();
}


void
IconEditorApp::RefsReceived(BMessage* message)
{
	bool append;
	if (message->FindBool("append", &append) != B_OK)
		append = false;
	MainWindow* window;
	if (message->FindPointer("window", (void**)&window) != B_OK)
		window = NULL;
	// When appending, we need to know a window.
	if (append && window == NULL)
		return;
	entry_ref ref;
	if (append) {
		if (!window->Lock())
			return;
		for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
			window->Open(ref, true);
		window->Unlock();
	} else {
		for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
			if (window != NULL && i == 0) {
				window->Lock();
				window->Open(ref, false);
				window->Unlock();
			} else {
				window = _NewWindow();
				window->Open(ref, false);
				window->Show();
			}
		}
	}

	if (fOpenPanel != NULL && fSavePanel != NULL)
		_SyncPanels(fOpenPanel, fSavePanel);
}


void
IconEditorApp::ArgvReceived(int32 argc, char** argv)
{
	if (argc < 2)
		return;

	entry_ref ref;

	for (int32 i = 1; i < argc; i++) {
		if (get_ref_for_path(argv[i], &ref) == B_OK) {
		 	MainWindow* window = _NewWindow();
			window->Open(ref);
			window->Show();
		}
	}
}


// #pragma mark -


MainWindow*
IconEditorApp::_NewWindow()
{
	fLastWindowFrame.OffsetBy(kWindowOffset, kWindowOffset);
	MainWindow* window = new MainWindow(fLastWindowFrame, this,
		&fLastWindowSettings);
	fWindowCount++;
	return window;
}


void
IconEditorApp::_SyncPanels(BFilePanel* from, BFilePanel* to)
{
	if (from->Window()->Lock()) {
		// location
		if (to->Window()->Lock()) {
			BRect frame = from->Window()->Frame();
			to->Window()->MoveTo(frame.left, frame.top);
			to->Window()->ResizeTo(frame.Width(), frame.Height());
			to->Window()->Unlock();
		}
		// current folder
		entry_ref panelDir;
		from->GetPanelDirectory(&panelDir);
		to->SetPanelDirectory(&panelDir);
		from->Window()->Unlock();
	}
}


const char*
IconEditorApp::_LastFilePath(path_kind which)
{
	const char* path = NULL;

	switch (which) {
		case LAST_PATH_OPEN:
			if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			else if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			break;
		case LAST_PATH_SAVE:
			if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			else if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			break;
		case LAST_PATH_EXPORT:
			if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			else if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			break;
	}
	if (!path) {

		BPath homePath;
		if (find_directory(B_USER_DIRECTORY, &homePath) == B_OK)
			path = homePath.Path();
		else 
			path = "/boot/home";
	}

	return path;
}


// #pragma mark -


void
IconEditorApp::_StoreSettings()
{
	BMessage settings('stns');

	settings.AddRect("window frame", fLastWindowFrame);
	settings.AddMessage("window settings", &fLastWindowSettings);
	settings.AddInt32("export mode", fSavePanel->ExportMode());

	save_settings(&settings, "Icon-O-Matic");
}


void
IconEditorApp::_RestoreSettings()
{
	BMessage settings('stns');
	load_settings(&settings, "Icon-O-Matic");

	BRect frame;
	if (settings.FindRect("window frame", &frame) == B_OK) {
		fLastWindowFrame = frame;
		// Compensate offset for next window...
		fLastWindowFrame.OffsetBy(-kWindowOffset, -kWindowOffset);
	}
	settings.FindMessage("window settings", &fLastWindowSettings);

	int32 mode;
	if (settings.FindInt32("export mode", &mode) >= B_OK)
		fSavePanel->SetExportMode(mode);
}


void
IconEditorApp::_InstallDocumentMimeType()
{
	// install mime type of documents
	BMimeType mime(kNativeIconMimeType);
	status_t ret = mime.InitCheck();
	if (ret < B_OK) {
		fprintf(stderr, "Could not init native document mime type (%s): %s.\n",
			kNativeIconMimeType, strerror(ret));
		return;
	}

	if (mime.IsInstalled() && !(modifiers() & B_SHIFT_KEY)) {
		// mime is already installed, and the user is not
		// pressing the shift key to force a re-install
		return;
	}

	ret = mime.Install();
	if (ret < B_OK) {
		fprintf(stderr, "Could not install native document mime type (%s): "
			"%s.\n", kNativeIconMimeType, strerror(ret));
		return;
	}
	// set preferred app
	ret = mime.SetPreferredApp(kAppSig);
	if (ret < B_OK)
		fprintf(stderr, "Could not set native document preferred app: %s\n",
			strerror(ret));

	// set descriptions
	ret = mime.SetShortDescription("Haiku Icon");
	if (ret < B_OK)
		fprintf(stderr, "Could not set short description of mime type: %s\n",
			strerror(ret));
	ret = mime.SetLongDescription("Native Haiku vector icon");
	if (ret < B_OK)
		fprintf(stderr, "Could not set long description of mime type: %s\n",
			strerror(ret));

	// set extensions
	BMessage message('extn');
	message.AddString("extensions", "icon");
	ret = mime.SetFileExtensions(&message);
	if (ret < B_OK)
		fprintf(stderr, "Could not set extensions of mime type: %s\n",
			strerror(ret));

	// set sniffer rule
	const char* snifferRule = "0.9 ('IMSG')";
	ret = mime.SetSnifferRule(snifferRule);
	if (ret < B_OK) {
		BString parseError;
		BMimeType::CheckSnifferRule(snifferRule, &parseError);
		fprintf(stderr, "Could not set sniffer rule of mime type: %s\n",
			parseError.String());
	}
}

