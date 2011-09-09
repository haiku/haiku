/*
 * MainApp.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de> (MIT Ok)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#include "MainApp.h"

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Locale.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <MimeType.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "EventQueue.h"
#include "Playlist.h"
#include "Settings.h"
#include "SettingsWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaPlayer-Main"


static const char* kCurrentPlaylistFilename = "MediaPlayer Current Playlist";

const char* kAppSig = "application/x-vnd.Haiku-MediaPlayer";

MainApp* gMainApp;

static const char* kMediaServerSig = B_MEDIA_SERVER_SIGNATURE;
static const char* kMediaServerAddOnSig = "application/x-vnd.Be.addon-host";


MainApp::MainApp()
	:
	BApplication(kAppSig),
	fPlayerCount(0),
	fSettingsWindow(NULL),

	fOpenFilePanel(NULL),
	fSaveFilePanel(NULL),
	fLastFilePanelFolder(),

	fMediaServerRunning(false),
	fMediaAddOnServerRunning(false),

	fAudioWindowFrameSaved(false),
	fLastSavedAudioWindowCreationTime(0)
{
	fLastFilePanelFolder = Settings::Default()->FilePanelFolder();

	// Now tell the application roster, that we're interested
	// in getting notifications of apps being launched or quit.
	// In this way we are going to detect a media_server restart.
	be_roster->StartWatching(BMessenger(this, this),
		B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	// we will keep track of the status of media_server
	// and media_addon_server
	fMediaServerRunning = be_roster->IsRunning(kMediaServerSig);
	fMediaAddOnServerRunning = be_roster->IsRunning(kMediaServerAddOnSig);

	if (!fMediaServerRunning || !fMediaAddOnServerRunning) {
		BAlert* alert = new BAlert("start_media_server",
			B_TRANSLATE("It appears the media server is not running.\n"
			"Would you like to start it ?"), B_TRANSLATE("Quit"),
			B_TRANSLATE("Start media server"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert->Go() == 0) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}

		launch_media_server();

		fMediaServerRunning = be_roster->IsRunning(kMediaServerSig);
		fMediaAddOnServerRunning = be_roster->IsRunning(kMediaServerAddOnSig);
	}
}


MainApp::~MainApp()
{
	delete fOpenFilePanel;
	delete fSaveFilePanel;
}


bool
MainApp::QuitRequested()
{
	// Make sure we store the current playlist, if applicable.
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		MainWin* playerWindow = dynamic_cast<MainWin*>(window);
		if (playerWindow == NULL)
			continue;

		BAutolock _(playerWindow);

		BMessage quitMessage;
		playerWindow->GetQuitMessage(&quitMessage);

		// Store the playlist if there is one. If the user has multiple
		// instances playing audio at the this time, the first instance wins.
		BMessage playlistArchive;
		if (quitMessage.FindMessage("playlist", &playlistArchive) == B_OK) {
			_StoreCurrentPlaylist(&playlistArchive);
			break;
		}
	}

	// Note: This needs to be done here, SettingsWindow::QuitRequested()
	// returns "false" always. (Standard BApplication quit procedure will
	// hang otherwise.)
	if (fSettingsWindow && fSettingsWindow->Lock())
		fSettingsWindow->Quit();
	fSettingsWindow = NULL;

	// store the current file panel ref in the global settings
	Settings::Default()->SetFilePanelFolder(fLastFilePanelFolder);

	return BApplication::QuitRequested();
}


MainWin*
MainApp::NewWindow(BMessage* message)
{
	BAutolock _(this);
	fPlayerCount++;
	return new(std::nothrow) MainWin(fPlayerCount == 1, message);
}


int32
MainApp::PlayerCount() const
{
	BAutolock _(const_cast<MainApp*>(this));
	return fPlayerCount;
}


// #pragma mark -


void
MainApp::ReadyToRun()
{
	// make sure we have at least one window open
	if (fPlayerCount == 0) {
		MainWin* window = NewWindow();
		if (window == NULL) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		BMessage lastPlaylistArchive;
		if (_RestoreCurrentPlaylist(&lastPlaylistArchive) == B_OK) {
			lastPlaylistArchive.what = M_OPEN_PREVIOUS_PLAYLIST;
			window->PostMessage(&lastPlaylistArchive);
		} else
			window->Show();
	}

	// setup the settings window now, we need to have it
	fSettingsWindow = new SettingsWindow(BRect(150, 150, 450, 520));
	fSettingsWindow->Hide();
	fSettingsWindow->Show();

	_InstallPlaylistMimeType();
}


void
MainApp::RefsReceived(BMessage* message)
{
	// The user dropped a file (or files) on this app's icon,
	// or double clicked a file that's handled by this app.
	// Command line arguments are also redirected to here by
	// ArgvReceived() but without MIME type check.

	// If multiple refs are received in short succession we
	// combine them into a single window/playlist. Tracker
	// will send multiple messages when opening a multi-
	// selection for example and we don't want to spawn large
	// numbers of windows when someone just tries to open an
	// album. We use half a second time and prolong it for
	// each new ref received.
	static bigtime_t sLastRefsReceived = 0;
	static MainWin* sLastRefsWindow = NULL;

	if (system_time() - sLastRefsReceived < 500000) {
		// Find the last opened window
		for (int32 i = CountWindows() - 1; i >= 0; i--) {
			MainWin* playerWindow = dynamic_cast<MainWin*>(WindowAt(i));
			if (playerWindow == NULL)
				continue;

			if (playerWindow != sLastRefsWindow) {
				// The window has changed since the last refs
				sLastRefsReceived = 0;
				sLastRefsWindow = NULL;
				break;
			}

			message->AddBool("append to playlist", true);
			playerWindow->PostMessage(message);
			sLastRefsReceived = system_time();
			return;
		}
	}

	sLastRefsWindow = NewWindow(message);
	sLastRefsReceived = system_time();
}


void
MainApp::ArgvReceived(int32 argc, char** argv)
{
	char cwd[B_PATH_NAME_LENGTH];
	getcwd(cwd, sizeof(cwd));

	BMessage message(B_REFS_RECEIVED);

	for (int i = 1; i < argc; i++) {
		BPath path;
		if (argv[i][0] != '/')
			path.SetTo(cwd, argv[i]);
		else
			path.SetTo(argv[i]);
		BEntry entry(path.Path(), true);
		if (!entry.Exists() || !entry.IsFile())
			continue;

		entry_ref ref;
		if (entry.GetRef(&ref) == B_OK)
			message.AddRef("refs", &ref);
	}

	if (message.HasRef("refs"))
		RefsReceived(&message);
}


void
MainApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_NEW_PLAYER:
		{
			MainWin* window = NewWindow();
			if (window != NULL)
				window->Show();
			break;
		}
		case M_PLAYER_QUIT:
		{
			// store the window settings of this instance
			MainWin* window = NULL;
			bool audioOnly = false;
			BRect windowFrame;
			bigtime_t creationTime;
			if (message->FindPointer("instance", (void**)&window) == B_OK
				&& message->FindBool("audio only", &audioOnly) == B_OK
				&& message->FindRect("window frame", &windowFrame) == B_OK
				&& message->FindInt64("creation time", &creationTime) == B_OK) {
				if (audioOnly && (!fAudioWindowFrameSaved
						|| creationTime < fLastSavedAudioWindowCreationTime)) {
					fAudioWindowFrameSaved = true;
					fLastSavedAudioWindowCreationTime = creationTime;

					Settings::Default()->SetAudioPlayerWindowFrame(windowFrame);
				}
			}

			// Store the playlist if there is one. Since the app is doing
			// this, it is "atomic". If the user has multiple instances
			// playing audio at the same time, the last instance which is
			// quit wins.
			BMessage playlistArchive;
			if (message->FindMessage("playlist", &playlistArchive) == B_OK)
				_StoreCurrentPlaylist(&playlistArchive);

			// quit if this was the last player window
			fPlayerCount--;
			if (fPlayerCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char* mimeSig;
			if (message->FindString("be:signature", &mimeSig) < B_OK)
				break;

			bool isMediaServer = strcmp(mimeSig, kMediaServerSig) == 0;
			bool isAddonServer = strcmp(mimeSig, kMediaServerAddOnSig) == 0;
			if (!isMediaServer && !isAddonServer)
				break;

			bool running = (message->what == B_SOME_APP_LAUNCHED);
			if (isMediaServer)
				fMediaServerRunning = running;
			if (isAddonServer)
				fMediaAddOnServerRunning = running;

			if (!fMediaServerRunning && !fMediaAddOnServerRunning) {
				fprintf(stderr, "media server has quit.\n");
				// trigger closing of media nodes
				BMessage broadcast(M_MEDIA_SERVER_QUIT);
				_BroadcastMessage(broadcast);
			} else if (fMediaServerRunning && fMediaAddOnServerRunning) {
				fprintf(stderr, "media server has launched.\n");
				// HACK!
				// quit our now invalid instance of the media roster
				// so that before new nodes are created,
				// we get a new roster (it is a normal looper)
				// TODO: This functionality could become part of
				// BMediaRoster. It could detect the start/quit of
				// the servers like it is done here, and either quit
				// itself, or re-establish the connection, and send some
				// notification to the app... something along those lines.
				BMediaRoster* roster = BMediaRoster::CurrentRoster();
				if (roster) {
					roster->Lock();
					roster->Quit();
				}
				// give the servers some time to init...
				snooze(3000000);
				// trigger re-init of media nodes
				BMessage broadcast(M_MEDIA_SERVER_STARTED);
				_BroadcastMessage(broadcast);
			}
			break;
		}
		case M_SETTINGS:
			_ShowSettingsWindow();
			break;

		case M_SHOW_OPEN_PANEL:
			_ShowOpenFilePanel(message);
			break;
		case M_SHOW_SAVE_PANEL:
			_ShowSaveFilePanel(message);
			break;

		case M_OPEN_PANEL_RESULT:
			_HandleOpenPanelResult(message);
			break;
		case M_SAVE_PANEL_RESULT:
			_HandleSavePanelResult(message);
			break;
		case B_CANCEL:
		{
			// The user canceled a file panel, but store at least the current
			// file panel folder.
			uint32 oldWhat;
			if (message->FindInt32("old_what", (int32*)&oldWhat) != B_OK)
				break;
			if (oldWhat == M_OPEN_PANEL_RESULT && fOpenFilePanel != NULL)
				fOpenFilePanel->GetPanelDirectory(&fLastFilePanelFolder);
			else if (oldWhat == M_SAVE_PANEL_RESULT && fSaveFilePanel != NULL)
				fSaveFilePanel->GetPanelDirectory(&fLastFilePanelFolder);
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
MainApp::_BroadcastMessage(const BMessage& _message)
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		BMessage message(_message);
		window->PostMessage(&message);
	}
}


void
MainApp::_ShowSettingsWindow()
{
	BAutolock lock(fSettingsWindow);
	if (!lock.IsLocked())
		return;

	// If the window is already showing, don't jerk the workspaces around,
	// just pull it to the current one.
	uint32 workspace = 1UL << (uint32)current_workspace();
	uint32 windowWorkspaces = fSettingsWindow->Workspaces();
	if ((windowWorkspaces & workspace) == 0) {
		// window in a different workspace, reopen in current
		fSettingsWindow->SetWorkspaces(workspace);
	}

	if (fSettingsWindow->IsHidden())
		fSettingsWindow->Show();
	else
		fSettingsWindow->Activate();
}


// #pragma mark - file panels


void
MainApp::_ShowOpenFilePanel(const BMessage* message)
{
	if (fOpenFilePanel == NULL) {
		BMessenger target(this);
		fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, &target);
	}

	_ShowFilePanel(fOpenFilePanel, M_OPEN_PANEL_RESULT, message,
		B_TRANSLATE("Open"), B_TRANSLATE("Open"));
}


void
MainApp::_ShowSaveFilePanel(const BMessage* message)
{
	if (fSaveFilePanel == NULL) {
		BMessenger target(this);
		fSaveFilePanel = new BFilePanel(B_SAVE_PANEL, &target);
	}

	_ShowFilePanel(fSaveFilePanel, M_SAVE_PANEL_RESULT, message,
		B_TRANSLATE("Save"), B_TRANSLATE("Save"));
}


void
MainApp::_ShowFilePanel(BFilePanel* panel, uint32 command,
	const BMessage* message, const char* defaultTitle,
	const char* defaultLabel)
{
//	printf("_ShowFilePanel()\n");
//	message->PrintToStream();

	BMessage panelMessage(command);

	if (message != NULL) {
		BMessage targetMessage;
		if (message->FindMessage("message", &targetMessage) == B_OK)
			panelMessage.AddMessage("message", &targetMessage);

		BMessenger target;
		if (message->FindMessenger("target", &target) == B_OK)
			panelMessage.AddMessenger("target", target);

		const char* panelTitle;
		if (message->FindString("title", &panelTitle) != B_OK)
			panelTitle = defaultTitle;
		{
			BString finalPanelTitle = "MediaPlayer: ";
			finalPanelTitle << panelTitle;
			BAutolock lock(panel->Window());
			panel->Window()->SetTitle(finalPanelTitle.String());
		}
		const char* buttonLabel;
		if (message->FindString("label", &buttonLabel) != B_OK)
			buttonLabel = defaultLabel;
		panel->SetButtonLabel(B_DEFAULT_BUTTON, buttonLabel);
	}

//	panelMessage.PrintToStream();
	panel->SetMessage(&panelMessage);

	if (fLastFilePanelFolder != entry_ref()) {
		panel->SetPanelDirectory(&fLastFilePanelFolder);
	}

	panel->Show();
}


void
MainApp::_HandleOpenPanelResult(const BMessage* message)
{
	_HandleFilePanelResult(fOpenFilePanel, message);
}


void
MainApp::_HandleSavePanelResult(const BMessage* message)
{
	_HandleFilePanelResult(fSaveFilePanel, message);
}


void
MainApp::_HandleFilePanelResult(BFilePanel* panel, const BMessage* message)
{
//	printf("_HandleFilePanelResult()\n");
//	message->PrintToStream();

	panel->GetPanelDirectory(&fLastFilePanelFolder);

	BMessage targetMessage;
	if (message->FindMessage("message", &targetMessage) != B_OK)
		targetMessage.what = message->what;

	BMessenger target;
	if (message->FindMessenger("target", &target) != B_OK) {
		if (targetMessage.what == M_OPEN_PANEL_RESULT
			|| targetMessage.what == M_SAVE_PANEL_RESULT) {
			// prevent endless message cycle
			return;
		}
		// send result message to ourselves
		target = BMessenger(this);
	}

	// copy the important contents of the message
	// save panel
	entry_ref directory;
	if (message->FindRef("directory", &directory) == B_OK)
		targetMessage.AddRef("directory", &directory);
	const char* name;
	if (message->FindString("name", &name) == B_OK)
		targetMessage.AddString("name", name);
	// open panel
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
		targetMessage.AddRef("refs", &ref);

	target.SendMessage(&targetMessage);
}


void
MainApp::_StoreCurrentPlaylist(const BMessage* message) const
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kCurrentPlaylistFilename) != B_OK) {
		return;
	}

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return;

	message->Flatten(&file);
}


status_t
MainApp::_RestoreCurrentPlaylist(BMessage* message) const
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kCurrentPlaylistFilename) != B_OK) {
		return B_ERROR;
	}

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return B_ERROR;

	return message->Unflatten(&file);
}


void
MainApp::_InstallPlaylistMimeType()
{
	// install mime type of documents
	BMimeType mime(kBinaryPlaylistMimeString);
	status_t ret = mime.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Could not init native document mime type (%s): %s.\n",
			kBinaryPlaylistMimeString, strerror(ret));
		return;
	}

	if (mime.IsInstalled() && !(modifiers() & B_SHIFT_KEY)) {
		// mime is already installed, and the user is not
		// pressing the shift key to force a re-install
		return;
	}

	ret = mime.Install();
	if (ret != B_OK && ret != B_FILE_EXISTS) {
		fprintf(stderr, "Could not install native document mime type (%s): %s.\n",
			kBinaryPlaylistMimeString, strerror(ret));
		return;
	}
	// set preferred app
	ret = mime.SetPreferredApp(kAppSig);
	if (ret != B_OK) {
		fprintf(stderr, "Could not set native document preferred app: %s\n",
			strerror(ret));
	}

	// set descriptions
	ret = mime.SetShortDescription("MediaPlayer playlist");
	if (ret != B_OK) {
		fprintf(stderr, "Could not set short description of mime type: %s\n",
			strerror(ret));
	}
	ret = mime.SetLongDescription("MediaPlayer binary playlist file");
	if (ret != B_OK) {
		fprintf(stderr, "Could not set long description of mime type: %s\n",
			strerror(ret));
	}

	// set extensions
	BMessage message('extn');
	message.AddString("extensions", "playlist");
	ret = mime.SetFileExtensions(&message);
	if (ret != B_OK) {
		fprintf(stderr, "Could not set extensions of mime type: %s\n",
			strerror(ret));
	}

	// set sniffer rule
	char snifferRule[32];
	uint32 bigEndianMagic = B_HOST_TO_BENDIAN_INT32(kPlaylistMagicBytes);
	sprintf(snifferRule, "0.9 ('%4s')", (const char*)&bigEndianMagic);
	ret = mime.SetSnifferRule(snifferRule);
	if (ret != B_OK) {
		BString parseError;
		BMimeType::CheckSnifferRule(snifferRule, &parseError);
		fprintf(stderr, "Could not set sniffer rule of mime type: %s\n",
			parseError.String());
	}

	// set playlist icon
	BResources* resources = AppResources();
		// does not need to be freed (belongs to BApplication base)
	if (resources != NULL) {
		size_t size;
		const void* iconData = resources->LoadResource('VICN', "PlaylistIcon",
			&size);
		if (iconData != NULL && size > 0) {
			if (mime.SetIcon(reinterpret_cast<const uint8*>(iconData), size)
				!= B_OK) {
				fprintf(stderr, "Could not set vector icon of mime type.\n");
			}
		} else {
			fprintf(stderr, "Could not find icon in app resources "
				"(data: %p, size: %ld).\n", iconData, size);
		}
	} else
		fprintf(stderr, "Could not find app resources.\n");
}


// #pragma mark - main


int
main()
{
	EventQueue::CreateDefault();

	srand(system_time());

	gMainApp = new MainApp;
	gMainApp->Run();
	delete gMainApp;

	EventQueue::DeleteDefault();

	return 0;
}
