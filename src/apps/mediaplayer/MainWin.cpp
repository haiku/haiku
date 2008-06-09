/*
 * MainWin.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2008 Stephan Aßmus <superstippi@gmx.de> (GPL->MIT ok)
 * Copyright (C) 2007-2008 Fredrik Modéen <fredrik@modeen.se>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 */
#include "MainWin.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Debug.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <RecentItems.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <View.h>

#include "ControllerObserver.h"
#include "MainApp.h"
#include "PlaylistObserver.h"
#include "PlaylistWindow.h"
#include "SettingsWindow.h"

#define NAME "MediaPlayer"
#define MIN_WIDTH 250


// XXX TODO: why is lround not defined?
#define lround(a) ((int)(0.99999 + (a)))

enum {
	M_DUMMY = 0x100,
	M_FILE_OPEN = 0x1000,
	M_FILE_NEWPLAYER,
	M_FILE_INFO,
	M_FILE_PLAYLIST,
	M_FILE_CLOSE,
	M_FILE_QUIT,	
	M_VIEW_50,
	M_VIEW_100,
	M_VIEW_200,
	M_VIEW_300,
	M_VIEW_400,
	M_TOGGLE_FULLSCREEN,
	M_TOGGLE_NO_BORDER,
	M_TOGGLE_NO_MENU,
	M_TOGGLE_NO_CONTROLS,
	M_TOGGLE_NO_BORDER_NO_MENU_NO_CONTROLS,
	M_TOGGLE_ALWAYS_ON_TOP,
	M_TOGGLE_KEEP_ASPECT_RATIO,
	M_SETTINGS,
	M_VOLUME_UP,
	M_VOLUME_DOWN,
	M_SKIP_NEXT,
	M_SKIP_PREV,
	M_ASPECT_100000_1,
	M_ASPECT_106666_1,
	M_ASPECT_109091_1,
	M_ASPECT_141176_1,
	M_ASPECT_720_576,
	M_ASPECT_704_576,
	M_ASPECT_544_576,
	M_SELECT_AUDIO_TRACK		= 0x00000800,
	M_SELECT_AUDIO_TRACK_END	= 0x00000fff,
	M_SELECT_VIDEO_TRACK		= 0x00010000,
	M_SELECT_VIDEO_TRACK_END	= 0x000fffff,

	M_SET_PLAYLIST_POSITION
};

//#define printf(a...)


MainWin::MainWin()
 :	BWindow(BRect(100,100,400,300), NAME, B_TITLED_WINDOW,
 		B_ASYNCHRONOUS_CONTROLS /* | B_WILL_ACCEPT_FIRST_CLICK */)
 ,  fFilePanel(NULL)
 ,	fInfoWin(NULL)
 ,	fPlaylistWindow(NULL)
 ,	fSettingsWindow(NULL)
 ,	fHasFile(false)
 ,	fHasVideo(false)
 ,	fHasAudio(false)
 ,	fPlaylist(new Playlist)
 ,	fPlaylistObserver(new PlaylistObserver(this))
 ,	fController(new Controller)
 ,	fControllerObserver(new ControllerObserver(this,
 		OBSERVE_FILE_CHANGES | OBSERVE_TRACK_CHANGES
 			| OBSERVE_PLAYBACK_STATE_CHANGES | OBSERVE_POSITION_CHANGES
 			| OBSERVE_VOLUME_CHANGES))
 ,	fIsFullscreen(false)
 ,	fKeepAspectRatio(true)
 ,	fAlwaysOnTop(false)
 ,	fNoMenu(false)
 ,	fNoBorder(false)
 ,	fNoControls(false)
 ,	fSourceWidth(0)
 ,	fSourceHeight(0)
 ,	fWidthScale(1.0)
 ,	fHeightScale(1.0)
 ,	fMouseDownTracking(false)
{
	static int pos = 0;
	MoveBy(pos * 25, pos * 25);
	pos = (pos + 1) % 15;
	
	BRect rect = Bounds();

	// background
	fBackground = new BView(rect, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	fBackground->SetViewColor(0,0,0);
	AddChild(fBackground);

	// menu
	fMenuBar = new BMenuBar(fBackground->Bounds(), "menu");
	_CreateMenu();
	fBackground->AddChild(fMenuBar);
	fMenuBar->ResizeToPreferred();
	fMenuBarWidth = (int)fMenuBar->Frame().Width() + 1;
	fMenuBarHeight = (int)fMenuBar->Frame().Height() + 1;
	fMenuBar->SetResizingMode(B_FOLLOW_NONE);

	// video view
	rect = BRect(0, fMenuBarHeight, fBackground->Bounds().right,
		fMenuBarHeight + 10);
	fVideoView = new VideoView(rect, "video display", B_FOLLOW_NONE);
	fBackground->AddChild(fVideoView);
	
	// controls
	rect = BRect(0, fMenuBarHeight + 11, fBackground->Bounds().right,
		fBackground->Bounds().bottom);
	fControls = new ControllerView(rect, fController, fPlaylist);
	fBackground->AddChild(fControls);
	fControls->ResizeToPreferred();
	fControlsHeight = (int)fControls->Frame().Height() + 1;
	fControlsWidth = (int)fControls->Frame().Width() + 1;
	fControls->SetResizingMode(B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
//	fControls->MoveTo(0, fBackground->Bounds().bottom - fControlsHeight + 1);
	
//	fVideoView->ResizeTo(fBackground->Bounds().Width(),
//		fBackground->Bounds().Height() - fMenuBarHeight - fControlsHeight);

	fPlaylist->AddListener(fPlaylistObserver);
	fController->SetVideoView(fVideoView);
	fController->AddListener(fControllerObserver);
	
//	printf("fMenuBarHeight %d\n", fMenuBarHeight);
//	printf("fControlsHeight %d\n", fControlsHeight);
//	printf("fControlsWidth %d\n", fControlsWidth);

	_SetupWindow();

	// setup the settings window now, we need to have it 
	fSettingsWindow = new SettingsWindow(BRect(150, 150, 450, 520));
	fSettingsWindow->Hide();
	fSettingsWindow->Show();

	// setup the playlist window now, we need to have it 
	// running for the undo/redo playlist editing
	fPlaylistWindow = new PlaylistWindow(BRect(150, 150, 500, 600), fPlaylist,
		fController);
	fPlaylistWindow->Hide();
	fPlaylistWindow->Show();
		// this makes sure the window thread is running without
		// showing the window just yet

	Show();
}


MainWin::~MainWin()
{
	printf("MainWin::~MainWin\n");

	fPlaylist->RemoveListener(fPlaylistObserver);
	fController->RemoveListener(fControllerObserver);

	// give the views a chance to detach from any notifiers
	// before we delete them
	fBackground->RemoveSelf();
	delete fBackground;

	if (fInfoWin) {
		fInfoWin->Lock();
		fInfoWin->Quit();
	}
	if (fPlaylistWindow) {
		fPlaylistWindow->Lock();
		fPlaylistWindow->Quit();
	}
	
	if (fSettingsWindow) {
		fSettingsWindow->Lock();
		fSettingsWindow->Quit();
	}

	delete fPlaylist;
	delete fFilePanel;

	// quit the Controller looper thread
	thread_id controllerThread = fController->Thread();
	fController->PostMessage(B_QUIT_REQUESTED);
	status_t exitValue;
	wait_for_thread(controllerThread, &exitValue);
}


// #pragma mark -


void
MainWin::FrameResized(float new_width, float new_height)
{
	if (new_width != Bounds().Width() || new_height != Bounds().Height()) {
		debugger("size wrong\n");
	}
	
	bool no_menu = fNoMenu || fIsFullscreen;
	bool no_controls = fNoControls || fIsFullscreen;
	
	printf("FrameResized enter: new_width %.0f, new_height %.0f\n",
		new_width, new_height);
	
	int max_video_width  = int(new_width) + 1;
	int max_video_height = int(new_height) + 1
		- (no_menu  ? 0 : fMenuBarHeight)
		- (no_controls ? 0 : fControlsHeight);

	ASSERT(max_video_height >= 0);
	
	int y = 0;
	
	if (no_menu) {
		if (!fMenuBar->IsHidden())
			fMenuBar->Hide();
	} else {
//		fMenuBar->MoveTo(0, y);
		fMenuBar->ResizeTo(new_width, fMenuBarHeight - 1);
		if (fMenuBar->IsHidden())
			fMenuBar->Show();
		y += fMenuBarHeight;
	}
	
	if (max_video_height == 0) {
		if (!fVideoView->IsHidden())
			fVideoView->Hide();
	} else {
//		fVideoView->MoveTo(0, y);
//		fVideoView->ResizeTo(max_video_width - 1, max_video_height - 1);
		_ResizeVideoView(0, y, max_video_width, max_video_height);
		if (fVideoView->IsHidden())
			fVideoView->Show();
		y += max_video_height;
	}
	
	if (no_controls) {
		if (!fControls->IsHidden())
			fControls->Hide();
	} else {
		fControls->MoveTo(0, y);
		fControls->ResizeTo(new_width, fControlsHeight - 1);
		if (fControls->IsHidden())
			fControls->Show();
//		y += fControlsHeight;
	}

	printf("FrameResized leave\n");
}


void
MainWin::Zoom(BPoint rec_position, float rec_width, float rec_height)
{
	PostMessage(M_TOGGLE_FULLSCREEN);
}


void
MainWin::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if ((msg->what == B_MOUSE_DOWN)
		&& (handler == fBackground || handler == fVideoView
				|| handler == fControls))
		_MouseDown(msg, dynamic_cast<BView*>(handler));

	if ((msg->what == B_MOUSE_MOVED)
		&& (handler == fBackground || handler == fVideoView
				|| handler == fControls))
		_MouseMoved(msg, dynamic_cast<BView*>(handler));

	if ((msg->what == B_MOUSE_UP)
		&& (handler == fBackground || handler == fVideoView))
		_MouseUp(msg);

	if ((msg->what == B_KEY_DOWN)
		&& (handler == fBackground || handler == fVideoView)) {

		// special case for PrintScreen key
		if (msg->FindInt32("key") == B_PRINT_KEY) {
			fVideoView->OverlayScreenshotPrepare();
			BWindow::DispatchMessage(msg, handler);
			fVideoView->OverlayScreenshotCleanup();
			return;
		}
		
		// every other key gets dispatched to our _KeyDown first
		if (_KeyDown(msg) == B_OK) {
			// it got handled, don't pass it on
			return;
		}
	}
	
	BWindow::DispatchMessage(msg, handler);
}


void
MainWin::MessageReceived(BMessage *msg)
{
//	msg->PrintToStream();
	switch (msg->what) {
		case B_REFS_RECEIVED:
			printf("MainWin::MessageReceived: B_REFS_RECEIVED\n");
			_RefsReceived(msg);
			break;
		case B_SIMPLE_DATA:
			printf("MainWin::MessageReceived: B_SIMPLE_DATA\n");
			if (msg->HasRef("refs")) {
				// add to recent documents as it's not done with drag-n-drop
				entry_ref ref;
				for (int32 i = 0; msg->FindRef("refs", i, &ref) == B_OK; i++) {
					be_roster->AddToRecentDocuments(&ref, kAppSig);
				}
				_RefsReceived(msg);
			}
			break;

		case M_MEDIA_SERVER_STARTED:
			// fController->...
			break;

		case M_MEDIA_SERVER_QUIT:
			// fController->...
			break;

		// PlaylistObserver messages
		case MSG_PLAYLIST_REF_ADDED: {
			entry_ref ref;
			int32 index;
			if (msg->FindRef("refs", &ref) == B_OK
				&& msg->FindInt32("index", &index) == B_OK) {
				_AddPlaylistItem(ref, index);
			}
			break;
		}
		case MSG_PLAYLIST_REF_REMOVED: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK) {
				_RemovePlaylistItem(index);
			}
			break;
		}
		case MSG_PLAYLIST_CURRENT_REF_CHANGED: {
			BAutolock _(fPlaylist);

			int32 index;
			if (msg->FindInt32("index", &index) < B_OK
				|| index != fPlaylist->CurrentRefIndex())
				break;
			entry_ref ref;
			if (fPlaylist->GetRefAt(index, &ref) == B_OK) {
				printf("open ref: %s\n", ref.name);
				OpenFile(ref);
				_MarkPlaylistItem(index);
			}
			break;
		}

		// ControllerObserver messages
		case MSG_CONTROLLER_FILE_FINISHED:
			fPlaylist->SetCurrentRefIndex(fPlaylist->CurrentRefIndex() + 1);
			break;
		case MSG_CONTROLLER_FILE_CHANGED:
			// TODO: move all other GUI changes as a reaction to this
			// notification
//			_UpdatePlaylistMenu();
			break;
		case MSG_CONTROLLER_VIDEO_TRACK_CHANGED: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK) {
				BMenuItem* item = fVideoTrackMenu->ItemAt(index);
				if (item)
					item->SetMarked(true);
			}
			break;
		}
		case MSG_CONTROLLER_AUDIO_TRACK_CHANGED: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK) {
				BMenuItem* item = fAudioTrackMenu->ItemAt(index);
				if (item)
					item->SetMarked(true);
			}
			break;
		}
		case MSG_CONTROLLER_PLAYBACK_STATE_CHANGED: {
			uint32 state;
			if (msg->FindInt32("state", (int32*)&state) == B_OK)
				fControls->SetPlaybackState(state);
			break;
		}
		case MSG_CONTROLLER_POSITION_CHANGED: {
			float position;
			if (msg->FindFloat("position", &position) == B_OK)
				fControls->SetPosition(position);
			break;
		}
		case MSG_CONTROLLER_VOLUME_CHANGED: {
			float volume;
			if (msg->FindFloat("volume", &volume) == B_OK)
				fControls->SetVolume(volume);
			break;
		}
		case MSG_CONTROLLER_MUTED_CHANGED: {
			bool muted;
			if (msg->FindBool("muted", &muted) == B_OK)
				fControls->SetMuted(muted);
			break;
		}

		// menu item messages
		case M_FILE_NEWPLAYER:
			gMainApp->NewWindow();
			break;
		case M_FILE_OPEN:
			if (!fFilePanel) {
				fFilePanel = new BFilePanel();
				fFilePanel->SetTarget(BMessenger(0, this));
				fFilePanel->SetPanelDirectory("/boot/home/");
			}
			fFilePanel->Show();
			break;
		case M_FILE_INFO:
			ShowFileInfo();
			break;
		case M_FILE_PLAYLIST:
			ShowPlaylistWindow();
			break;
		case B_ABOUT_REQUESTED:
			BAlert *alert;
			alert = new BAlert("about", NAME"\n\n Written by Marcus Overhagen "
				", Stephan Aßmus and Frederik Modéen", "Thanks");
			if (fAlwaysOnTop) {
				_ToggleAlwaysOnTop();
				alert->Go(NULL);  // Asynchronous mode
				_ToggleAlwaysOnTop();
			} else {
				alert->Go(NULL); // Asynchronous mode
			}
			break;
		case M_FILE_CLOSE:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case M_FILE_QUIT:	
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case M_TOGGLE_FULLSCREEN:
			_ToggleFullscreen();
			break;

		case M_TOGGLE_NO_MENU:
			_ToggleNoMenu();
			break;
			
		case M_TOGGLE_NO_CONTROLS:
			_ToggleNoControls();
			break;
		
		case M_TOGGLE_NO_BORDER:
			_ToggleNoBorder();
			break;
			
		case M_TOGGLE_ALWAYS_ON_TOP:
			_ToggleAlwaysOnTop();
			break;
	
		case M_TOGGLE_KEEP_ASPECT_RATIO:
			_ToggleKeepAspectRatio();
			break;

		case M_TOGGLE_NO_BORDER_NO_MENU_NO_CONTROLS:
			_ToggleNoBorderNoMenu();
			break;

		case M_VIEW_50:
			if (!fHasVideo)
				break;
			if (fIsFullscreen)
				_ToggleFullscreen();
			_ResizeWindow(50);
			break;
			
		case M_VIEW_100:
			if (!fHasVideo)
				break;
			if (fIsFullscreen)
				_ToggleFullscreen();
			_ResizeWindow(100);
			break;

		case M_VIEW_200:
			if (!fHasVideo)
				break;
			if (fIsFullscreen)
				_ToggleFullscreen();
			_ResizeWindow(200);
			break;

		case M_VIEW_300:
			if (!fHasVideo)
				break;
			if (fIsFullscreen)
				_ToggleFullscreen();
			_ResizeWindow(300);
			break;

		case M_VIEW_400:
			if (!fHasVideo)
				break;
			if (fIsFullscreen)
				_ToggleFullscreen();
			_ResizeWindow(400);
			break;

/*
		
		case B_ACQUIRE_OVERLAY_LOCK:
			printf("B_ACQUIRE_OVERLAY_LOCK\n");
			fVideoView->OverlayLockAcquire();
			break;
			
		case B_RELEASE_OVERLAY_LOCK:
			printf("B_RELEASE_OVERLAY_LOCK\n");
			fVideoView->OverlayLockRelease();
			break;
	
		case B_MOUSE_WHEEL_CHANGED:
		{
			printf("B_MOUSE_WHEEL_CHANGED\n");
			float dx = msg->FindFloat("be:wheel_delta_x");
			float dy = msg->FindFloat("be:wheel_delta_y");
			bool inv = modifiers() & B_COMMAND_KEY;
			if (dx > 0.1)	PostMessage(inv ? M_VOLUME_DOWN : M_SKIP_PREV);
			if (dx < -0.1)	PostMessage(inv ? M_VOLUME_UP : M_SKIP_NEXT);
			if (dy > 0.1)	PostMessage(inv ? M_SKIP_PREV : M_VOLUME_DOWN);
			if (dy < -0.1)	PostMessage(inv ? M_SKIP_NEXT : M_VOLUME_UP);
			break;
		}
*/
		case M_SKIP_NEXT:
			fControls->SkipForward();
			break;

		case M_SKIP_PREV:
			fControls->SkipBackward();
			break;

		case M_VOLUME_UP:
			fController->VolumeUp();
			break;

		case M_VOLUME_DOWN:
			fController->VolumeDown();
			break;

		case M_ASPECT_100000_1:
			VideoFormatChange(fSourceWidth, fSourceHeight, 1.0, 1.0);
			break;

		case M_ASPECT_106666_1:
			VideoFormatChange(fSourceWidth, fSourceHeight, 1.06666, 1.0);
			break;

		case M_ASPECT_109091_1:
			VideoFormatChange(fSourceWidth, fSourceHeight, 1.09091, 1.0);
			break;

		case M_ASPECT_141176_1:
			VideoFormatChange(fSourceWidth, fSourceHeight, 1.41176, 1.0);
			break;

		case M_ASPECT_720_576:
			VideoFormatChange(720, 576, 1.06666, 1.0);
			break;

		case M_ASPECT_704_576:
			VideoFormatChange(704, 576, 1.09091, 1.0);
			break;

		case M_ASPECT_544_576:
			VideoFormatChange(544, 576, 1.41176, 1.0);
			break;

		case M_SETTINGS:
			ShowSettingsWindow();
			break;
/*			
		default:
			if (msg->what >= M_SELECT_CHANNEL
				&& msg->what <= M_SELECT_CHANNEL_END) {
				SelectChannel(msg->what - M_SELECT_CHANNEL);
				break;
			}
			if (msg->what >= M_SELECT_INTERFACE
				&& msg->what <= M_SELECT_INTERFACE_END) {
				SelectInterface(msg->what - M_SELECT_INTERFACE - 1);
				break;
			}
*/
		case M_SET_PLAYLIST_POSITION: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK)
				fPlaylist->SetCurrentRefIndex(index);
			break;
		}

		default:
			// let BWindow handle the rest
			BWindow::MessageReceived(msg);
	}
}


void
MainWin::WindowActivated(bool active)
{
	if (active) {
		BScreen screen(this);
		BRect screenFrame = screen.Frame();
		BRect frame = Frame();
		float diffX = 0.0;
		float diffY = 0.0;

		// If the frame if off the edge of the screen at all
		// we will move it so all the window is on the screen.
		if (frame.left < screenFrame.left)
			// Move right
			diffX = screenFrame.left - frame.left; 
		if (frame.top < screenFrame.top)
			// Move down
			diffY = screenFrame.top - frame.top; 
		if (frame.right > screenFrame.right)
			// Move left
			diffX = screenFrame.right - frame.right; 
		if (frame.bottom > screenFrame.bottom)
			// Move up
			diffY = screenFrame.bottom - frame.bottom; 

		MoveBy(diffX, diffY);
	}
}


bool
MainWin::QuitRequested()
{
	be_app->PostMessage(M_PLAYER_QUIT);
	return true;
}


// #pragma mark -


void
MainWin::OpenFile(const entry_ref &ref)
{
	printf("MainWin::OpenFile\n");

	status_t err = fController->SetTo(ref);
	if (err != B_OK) {
		if (fPlaylist->CountItems() == 1) {
			// display error if this is the only file we're supposed to play
			char s[300];
			sprintf(s, "Can't open file\n\n%s\n\nError 0x%08lx\n(%s)\n",
				ref.name, err, strerror(err));
			(new BAlert("error", s, "OK"))->Go();
		} else {
			// just go to the next file and don't bother user
			fPlaylist->SetCurrentRefIndex(fPlaylist->CurrentRefIndex() + 1);
		}
		fHasFile = false;
		fHasVideo = false;
		fHasAudio = false;
		SetTitle(NAME);
	} else {
		fHasFile = true;
		fHasVideo = fController->VideoTrackCount() != 0;
		fHasAudio = fController->AudioTrackCount() != 0;
		SetTitle(ref.name);
	}
	_SetupWindow();
}


void
MainWin::ShowFileInfo()
{
	if (!fInfoWin)
		fInfoWin = new InfoWin(Frame().LeftTop(), fController);

	if (fInfoWin->Lock()) {
		if (fInfoWin->IsHidden())
			fInfoWin->Show();
		else
			fInfoWin->Activate();
		fInfoWin->Unlock();
	}
}


void
MainWin::ShowPlaylistWindow()
{
	if (fPlaylistWindow->Lock()) {
		if (fPlaylistWindow->IsHidden())
			fPlaylistWindow->Show();
		else
			fPlaylistWindow->Activate();
		fPlaylistWindow->Unlock();
	}
}


void
MainWin::ShowSettingsWindow()
{
	if (fSettingsWindow->Lock()) {
		if (fSettingsWindow->IsHidden())
			fSettingsWindow->Show();
		else
			fSettingsWindow->Activate();
		fSettingsWindow->Unlock();
	}
}


void
MainWin::VideoFormatChange(int width, int height, float width_scale,
	float height_scale)
{
	// called when video format or aspect ratio changes
	
	printf("VideoFormatChange enter: width %d, height %d, width_scale %.6f, "
		"height_scale %.6f\n", width, height, width_scale, height_scale);

	if (width_scale < 1.0 && height_scale >= 1.0) {
		width_scale  = 1.0 / width_scale;
		height_scale = 1.0 / height_scale;
		printf("inverting! new values: width_scale %.6f, height_scale %.6f\n",
			width_scale, height_scale);
	}
	
 	fSourceWidth  = width;
 	fSourceHeight = height;
 	fWidthScale   = width_scale;
 	fHeightScale  = height_scale;
 	
 	FrameResized(Bounds().Width(), Bounds().Height());

	printf("VideoFormatChange leave\n");
}


// #pragma mark -


void
MainWin::_RefsReceived(BMessage* msg)
{
	// the playlist ist replaced by dropped files
	// or the dropped files are appended to the end
	// of the existing playlist if <shift> is pressed
	int32 appendIndex = modifiers() & B_SHIFT_KEY ?
		fPlaylist->CountItems() : -1;
	msg->AddInt32("append_index", appendIndex);

	// forward the message to the playlist window,
	// so that undo/redo is used for modifying the playlist
	fPlaylistWindow->PostMessage(msg);
}


void
MainWin::_SetupWindow()
{
	printf("MainWin::_SetupWindow\n");	
	// Populate the track menus
	_SetupTrackMenus();
	// Enable both if a file was loaded
	fAudioTrackMenu->SetEnabled(fHasFile);
	fVideoTrackMenu->SetEnabled(fHasFile);
	// Select first track (might be "none") in both
	fAudioTrackMenu->ItemAt(0)->SetMarked(true);
	fVideoTrackMenu->ItemAt(0)->SetMarked(true);

	fVideoMenu->SetEnabled(fHasVideo);
	fAudioMenu->SetEnabled(fHasAudio);
//	fDebugMenu->SetEnabled(fHasVideo);
	if (fHasVideo) {
		fController->GetSize(&fSourceWidth, &fSourceHeight);
		fWidthScale = 1.0;
		fHeightScale = 1.0;
	} else {
		fSourceWidth = 0;
		fSourceHeight = 0;
		fWidthScale = 1.0;
		fHeightScale = 1.0;
	}
	_UpdateControlsEnabledStatus();

	_ResizeWindow(100);

	fVideoView->MakeFocus();
}


void
MainWin::_CreateMenu()
{
	fFileMenu = new BMenu(NAME);
	fPlaylistMenu = new BMenu("Playlist"B_UTF8_ELLIPSIS);
	fAudioMenu = new BMenu("Audio");
	fVideoMenu = new BMenu("Video");
	fSettingsMenu = new BMenu("Settings");
	fAudioTrackMenu = new BMenu("Track");
	fVideoTrackMenu = new BMenu("Track");
//	fDebugMenu = new BMenu("Debug");

	fMenuBar->AddItem(fFileMenu);
	fMenuBar->AddItem(fAudioMenu);
	fMenuBar->AddItem(fVideoMenu);
	fMenuBar->AddItem(fSettingsMenu);
//	fMenuBar->AddItem(fDebugMenu);

	fFileMenu->AddItem(new BMenuItem("New Player"B_UTF8_ELLIPSIS,
		new BMessage(M_FILE_NEWPLAYER), 'N'));
	fFileMenu->AddSeparatorItem();

//	fFileMenu->AddItem(new BMenuItem("Open File"B_UTF8_ELLIPSIS,
//		new BMessage(M_FILE_OPEN), 'O'));
	// Add recent files
	BRecentFilesList recentFiles(10, false, NULL, kAppSig);
	BMenuItem *item = new BMenuItem(recentFiles.NewFileListMenu(
		"Open File"B_UTF8_ELLIPSIS, new BMessage(B_REFS_RECEIVED), 
		NULL, this, 10, false, NULL, 0, kAppSig), new BMessage(M_FILE_OPEN));
	item->SetShortcut('O', 0);
	fFileMenu->AddItem(item);

	fFileMenu->AddItem(new BMenuItem("File Info"B_UTF8_ELLIPSIS,
		new BMessage(M_FILE_INFO), 'I'));
	fFileMenu->AddItem(fPlaylistMenu);
	fPlaylistMenu->Superitem()->SetShortcut('P', B_COMMAND_KEY);
	fPlaylistMenu->Superitem()->SetMessage(new BMessage(M_FILE_PLAYLIST));

	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("About " NAME B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(new BMenuItem("Close", new BMessage(M_FILE_CLOSE), 'W'));
	fFileMenu->AddItem(new BMenuItem("Quit", new BMessage(M_FILE_QUIT), 'Q'));

	fPlaylistMenu->SetRadioMode(true);

	fAudioMenu->AddItem(fAudioTrackMenu);

	fVideoMenu->AddItem(fVideoTrackMenu);
	fVideoMenu->AddSeparatorItem();
	fVideoMenu->AddItem(new BMenuItem("50% scale",
		new BMessage(M_VIEW_50), '0'));
	fVideoMenu->AddItem(new BMenuItem("100% scale",
		new BMessage(M_VIEW_100), '1'));
	fVideoMenu->AddItem(new BMenuItem("200% scale",
		new BMessage(M_VIEW_200), '2'));
	fVideoMenu->AddItem(new BMenuItem("300% scale",
		new BMessage(M_VIEW_300), '3'));
	fVideoMenu->AddItem(new BMenuItem("400% scale",
		new BMessage(M_VIEW_400), '4'));
	fVideoMenu->AddSeparatorItem();
	fVideoMenu->AddItem(new BMenuItem("Full Screen",
		new BMessage(M_TOGGLE_FULLSCREEN), 'F'));
	fVideoMenu->AddItem(new BMenuItem("Keep Aspect Ratio",
		new BMessage(M_TOGGLE_KEEP_ASPECT_RATIO), 'K'));

	fSettingsMenu->AddItem(new BMenuItem("No Menu",
		new BMessage(M_TOGGLE_NO_MENU), 'M'));
	fSettingsMenu->AddItem(new BMenuItem("No Border",
		new BMessage(M_TOGGLE_NO_BORDER), 'B'));
	fSettingsMenu->AddItem(new BMenuItem("No Controls",
		new BMessage(M_TOGGLE_NO_CONTROLS), 'C'));
	fSettingsMenu->AddItem(new BMenuItem("Always on Top",
		new BMessage(M_TOGGLE_ALWAYS_ON_TOP), 'T'));
//	fSettingsMenu->AddSeparatorItem();
//	fSettingsMenu->AddItem(new BMenuItem("Settings"B_UTF8_ELLIPSIS,
//		new BMessage(M_SETTINGS), 'S'));

//	fDebugMenu->AddItem(new BMenuItem("pixel aspect ratio 1.00000:1",
//		new BMessage(M_ASPECT_100000_1)));
//	fDebugMenu->AddItem(new BMenuItem("pixel aspect ratio 1.06666:1",
//		new BMessage(M_ASPECT_106666_1)));
//	fDebugMenu->AddItem(new BMenuItem("pixel aspect ratio 1.09091:1",
//		new BMessage(M_ASPECT_109091_1)));
//	fDebugMenu->AddItem(new BMenuItem("pixel aspect ratio 1.41176:1",
//		new BMessage(M_ASPECT_141176_1)));
//	fDebugMenu->AddItem(new BMenuItem("force 720 x 576, display aspect 4:3",
//		new BMessage(M_ASPECT_720_576)));
//	fDebugMenu->AddItem(new BMenuItem("force 704 x 576, display aspect 4:3",
//		new BMessage(M_ASPECT_704_576)));
//	fDebugMenu->AddItem(new BMenuItem("force 544 x 576, display aspect 4:3",
//		new BMessage(M_ASPECT_544_576)));

	fAudioTrackMenu->SetRadioMode(true);
	fVideoTrackMenu->SetRadioMode(true);
}


void
MainWin::_SetupTrackMenus()
{
	fAudioTrackMenu->RemoveItems(0, fAudioTrackMenu->CountItems(), true);
	fVideoTrackMenu->RemoveItems(0, fVideoTrackMenu->CountItems(), true);
	
	int c, i;
	char s[100];
	
	c = fController->AudioTrackCount();
	for (i = 0; i < c; i++) {
		sprintf(s, "Track %d", i + 1);
		fAudioTrackMenu->AddItem(new BMenuItem(s,
			new BMessage(M_SELECT_AUDIO_TRACK + i)));
	}
	if (!c)
		fAudioTrackMenu->AddItem(new BMenuItem("none", new BMessage(M_DUMMY)));

	c = fController->VideoTrackCount();
	for (i = 0; i < c; i++) {
		sprintf(s, "Track %d", i + 1);
		fVideoTrackMenu->AddItem(new BMenuItem(s,
			new BMessage(M_SELECT_VIDEO_TRACK + i)));
	}
	if (!c)
		fVideoTrackMenu->AddItem(new BMenuItem("none", new BMessage(M_DUMMY)));
}


void
MainWin::_SetWindowSizeLimits()
{
	int minWidth = fNoControls  ? MIN_WIDTH : fControlsWidth;
	if (!fNoMenu)
		minWidth = max_c(minWidth, fMenuBarWidth);
	int minHeight = (fNoMenu ? 0 : fMenuBarHeight)
		+ (fNoControls ? 0 : fControlsHeight);
	
	SetSizeLimits(minWidth - 1, 32000, minHeight - 1, fHasVideo ?
		32000 : minHeight - 1);
}


void
MainWin::_ResizeWindow(int percent)
{
	int video_width;
	int video_height;

	// Get required window size
	video_width = lround(fSourceWidth * fWidthScale);
	video_height = lround(fSourceHeight * fHeightScale);
	
	video_width = (video_width * percent) / 100;
	video_height = (video_height * percent) / 100;
	
	// Calculate and set the initial window size
	int width = max_c(fControlsWidth, video_width);
	int height = (fNoControls ? 0 : fControlsHeight) + video_height;
	if (!fNoMenu) {
		width = max_c(width, fMenuBarWidth);
		height += fMenuBarHeight;
	}
	_SetWindowSizeLimits();
	ResizeTo(width - 1, height - 1);
}


void
MainWin::_ResizeVideoView(int x, int y, int width, int height)
{
	printf("_ResizeVideoView: %d,%d, width %d, height %d\n", x, y,
		width, height);
	
	if (fKeepAspectRatio) {
		// Keep aspect ratio, place video view inside
		// the background area (may create black bars).
		float scaled_width  = fSourceWidth * fWidthScale;
		float scaled_height = fSourceHeight * fHeightScale;
		float factor = min_c(width / scaled_width, height / scaled_height);
		int render_width = lround(scaled_width * factor);
		int render_height = lround(scaled_height * factor);
		if (render_width > width)
			render_width = width;
		if (render_height > height)
			render_height = height;

		int x_ofs = x + (width - render_width) / 2;
		int y_ofs = y + (height - render_height) / 2;

		fVideoView->MoveTo(x_ofs, y_ofs);
		fVideoView->ResizeTo(render_width - 1, render_height - 1);

	} else {
		fVideoView->MoveTo(x, y);
		fVideoView->ResizeTo(width - 1, height - 1);
	}
}


// #pragma mark -


void
MainWin::_MouseDown(BMessage *msg, BView* originalHandler)
{
	BPoint screen_where;
	uint32 buttons = msg->FindInt32("buttons");

	// On Zeta, only "screen_where" is relyable, "where" and "be:view_where"
	// seem to be broken
	if (B_OK != msg->FindPoint("screen_where", &screen_where)) {
		// Workaround for BeOS R5, it has no "screen_where"
		if (!originalHandler || msg->FindPoint("where", &screen_where) < B_OK)
			return;
		originalHandler->ConvertToScreen(&screen_where);
	}

//	msg->PrintToStream();

//	if (1 == msg->FindInt32("buttons") && msg->FindInt32("clicks") == 1) {

	if (1 == buttons && msg->FindInt32("clicks") % 2 == 0) {
		BRect r(screen_where.x - 1, screen_where.y - 1, screen_where.x + 1,
			screen_where.y + 1);
		if (r.Contains(fMouseDownMousePos)) {
			PostMessage(M_TOGGLE_FULLSCREEN);
			return;
		}
	}
	
	if (2 == buttons && msg->FindInt32("clicks") % 2 == 0) {
		BRect r(screen_where.x - 1, screen_where.y - 1, screen_where.x + 1,
			screen_where.y + 1);
		if (r.Contains(fMouseDownMousePos)) {
			PostMessage(M_TOGGLE_NO_BORDER_NO_MENU_NO_CONTROLS);
			return;
		}
	}

/*
		// very broken in Zeta:
		fMouseDownMousePos = fVideoView->ConvertToScreen(
			msg->FindPoint("where"));
*/
	fMouseDownMousePos = screen_where;
	fMouseDownWindowPos = Frame().LeftTop();

	if (buttons == 1 && !fIsFullscreen) {
		// start mouse tracking
		fVideoView->SetMouseEventMask(B_POINTER_EVENTS | B_NO_POINTER_HISTORY
			/* | B_LOCK_WINDOW_FOCUS */);
		fMouseDownTracking = true;
	}

	// pop up a context menu if right mouse button is down for 200 ms

	if ((buttons & 2) == 0)
		return;
	bigtime_t start = system_time();
	bigtime_t delay = 200000;
	BPoint location;
	do { 
		fVideoView->GetMouse(&location, &buttons);
		if ((buttons & 2) == 0)
			break;
		snooze(1000);
	} while (system_time() - start < delay);

	if (buttons & 2)
		_ShowContextMenu(screen_where);
}


void
MainWin::_MouseMoved(BMessage *msg, BView* originalHandler)
{
//	msg->PrintToStream();

	BPoint mousePos;
	uint32 buttons = msg->FindInt32("buttons");
	
	if (1 == buttons && fMouseDownTracking && !fIsFullscreen) {
/*		
		// very broken in Zeta:
		BPoint mousePos = msg->FindPoint("where");
		printf("view where: %.0f, %.0f => ", mousePos.x, mousePos.y);
		fVideoView->ConvertToScreen(&mousePos);
*/
		// On Zeta, only "screen_where" is relyable, "where"
		// and "be:view_where" seem to be broken
		if (B_OK != msg->FindPoint("screen_where", &mousePos)) {
			// Workaround for BeOS R5, it has no "screen_where"
			if (!originalHandler || msg->FindPoint("where", &mousePos) < B_OK)
				return;
			originalHandler->ConvertToScreen(&mousePos);
		}
//		printf("screen where: %.0f, %.0f => ", mousePos.x, mousePos.y);
		float delta_x = mousePos.x - fMouseDownMousePos.x;
		float delta_y = mousePos.y - fMouseDownMousePos.y;
		float x = fMouseDownWindowPos.x + delta_x;
		float y = fMouseDownWindowPos.y + delta_y;
//		printf("move window to %.0f, %.0f\n", x, y);
		MoveTo(x, y);
	}
}


void
MainWin::_MouseUp(BMessage *msg)
{
//	msg->PrintToStream();
	fMouseDownTracking = false;
}


void
MainWin::_ShowContextMenu(const BPoint &screen_point)
{
	printf("Show context menu\n");
	BPopUpMenu *menu = new BPopUpMenu("context menu", false, false);
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem("Full Screen",
		new BMessage(M_TOGGLE_FULLSCREEN), 'F'));
	item->SetMarked(fIsFullscreen);
	item->SetEnabled(fHasVideo);
	menu->AddItem(item = new BMenuItem("Keep Aspect Ratio",
		new BMessage(M_TOGGLE_KEEP_ASPECT_RATIO), 'K'));
	item->SetMarked(fKeepAspectRatio);
	item->SetEnabled(fHasVideo);

	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("No Menu",
		new BMessage(M_TOGGLE_NO_MENU), 'M'));
	item->SetMarked(fNoMenu);
	menu->AddItem(item = new BMenuItem("No Border",
		new BMessage(M_TOGGLE_NO_BORDER), 'B'));
	item->SetMarked(fNoBorder);
	menu->AddItem(item = new BMenuItem("Always on Top",
		new BMessage(M_TOGGLE_ALWAYS_ON_TOP), 'T'));
	item->SetMarked(fAlwaysOnTop);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("About " NAME B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(M_FILE_QUIT), 'Q'));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("pixel aspect ratio 1.00000:1",
		new BMessage(M_ASPECT_100000_1)));
	menu->AddItem(new BMenuItem("pixel aspect ratio 1.06666:1",
		new BMessage(M_ASPECT_106666_1)));
	menu->AddItem(new BMenuItem("pixel aspect ratio 1.09091:1",
		new BMessage(M_ASPECT_109091_1)));
	menu->AddItem(new BMenuItem("pixel aspect ratio 1.41176:1",
		new BMessage(M_ASPECT_141176_1)));
	menu->AddItem(new BMenuItem("force 720 x 576, display aspect 4:3",
		new BMessage(M_ASPECT_720_576)));
	menu->AddItem(new BMenuItem("force 704 x 576, display aspect 4:3",
		new BMessage(M_ASPECT_704_576)));
	menu->AddItem(new BMenuItem("force 544 x 576, display aspect 4:3",
		new BMessage(M_ASPECT_544_576)));

	menu->SetTargetForItems(this);
	BRect r(screen_point.x - 5, screen_point.y - 5, screen_point.x + 5,
		screen_point.y + 5);
	menu->Go(screen_point, true, true, r, true);
}


/* Trap keys that are about to be send to background or renderer view.
 * Return B_OK if it shouldn't be passed to the view
 */
status_t
MainWin::_KeyDown(BMessage *msg)
{
//	msg->PrintToStream();
	
	uint32 key		 = msg->FindInt32("key");
	uint32 raw_char  = msg->FindInt32("raw_char");
	uint32 modifiers = msg->FindInt32("modifiers");
	
	printf("key 0x%lx, raw_char 0x%lx, modifiers 0x%lx\n", key, raw_char,
		modifiers);
	
	switch (raw_char) {
		case B_SPACE:
			fController->TogglePlaying();
			return B_OK;

		case B_ESCAPE:
			if (fIsFullscreen) {
				PostMessage(M_TOGGLE_FULLSCREEN);
				return B_OK;
			} else
				break;

		case B_ENTER:		// Enter / Return
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_TOGGLE_FULLSCREEN);
				return B_OK;
			} else
				break;
				
		case B_TAB:
			if ((modifiers & (B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY
					| B_MENU_KEY)) == 0) {
				PostMessage(M_TOGGLE_FULLSCREEN);
				return B_OK;
			} else
				break;
		
		case B_UP_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_SKIP_NEXT);
			} else {
				PostMessage(M_VOLUME_UP);
			}
			return B_OK;

		case B_DOWN_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_SKIP_PREV);
			} else {
				PostMessage(M_VOLUME_DOWN);
			}
			return B_OK;
			
		case B_RIGHT_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_VOLUME_UP);
			} else {
				PostMessage(M_SKIP_NEXT);
			}
			return B_OK;

		case B_LEFT_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_VOLUME_DOWN);
			} else {
				PostMessage(M_SKIP_PREV);
			}
			return B_OK;

		case B_PAGE_UP:
			PostMessage(M_SKIP_NEXT);
			return B_OK;
			
		case B_PAGE_DOWN:
			PostMessage(M_SKIP_PREV);
			return B_OK;
	}

	switch (key) {
		case 0x3a:  		// numeric keypad +
			if ((modifiers & B_COMMAND_KEY) == 0) {
				printf("if\n");
				PostMessage(M_VOLUME_UP);
				return B_OK;
			} else {
				printf("else\n");
				break;
			}

		case 0x25:  		// numeric keypad -
			if ((modifiers & B_COMMAND_KEY) == 0) {
				PostMessage(M_VOLUME_DOWN);
				return B_OK;
			} else {
				break;
			}

		case 0x38:			// numeric keypad up arrow
			PostMessage(M_VOLUME_UP);
			return B_OK;

		case 0x59:			// numeric keypad down arrow
			PostMessage(M_VOLUME_DOWN);
			return B_OK;
			
		case 0x39:			// numeric keypad page up
		case 0x4a:			// numeric keypad right arrow
			PostMessage(M_SKIP_NEXT);
			return B_OK;
			
		case 0x5a:			// numeric keypad page down
		case 0x48:			// numeric keypad left arrow
			PostMessage(M_SKIP_PREV);
			return B_OK;
	}
	
	return B_ERROR;
}


// #pragma mark -


void
MainWin::_ToggleNoBorderNoMenu()
{
	if (!fNoMenu && !fNoBorder && !fNoControls) {
		PostMessage(M_TOGGLE_NO_MENU);
		PostMessage(M_TOGGLE_NO_BORDER);
		PostMessage(M_TOGGLE_NO_CONTROLS);
	} else {
		if (!fNoMenu)
			PostMessage(M_TOGGLE_NO_MENU);
		if (!fNoBorder)
			PostMessage(M_TOGGLE_NO_BORDER);
		if (!fNoControls)
			PostMessage(M_TOGGLE_NO_CONTROLS);
	}
}


void
MainWin::_ToggleFullscreen()
{
	printf("_ToggleFullscreen enter\n");

	if (!fHasVideo) {
		printf("_ToggleFullscreen - ignoring, as we don't have a video\n");
		return;
	}

	fIsFullscreen = !fIsFullscreen;
	
	if (fIsFullscreen) {
		// switch to fullscreen
		
		fSavedFrame = Frame();
		printf("saving current frame: %d %d %d %d\n", int(fSavedFrame.left),
			int(fSavedFrame.top), int(fSavedFrame.right),
			int(fSavedFrame.bottom));
		BScreen screen(this);
		BRect rect(screen.Frame());

		Hide();
		MoveTo(rect.left, rect.top);
		ResizeTo(rect.Width(), rect.Height());
		Show();

	} else {
		// switch back from full screen mode

		Hide();
		MoveTo(fSavedFrame.left, fSavedFrame.top);
		ResizeTo(fSavedFrame.Width(), fSavedFrame.Height());
		Show();
	}

	_MarkSettingsItem(M_TOGGLE_FULLSCREEN, fIsFullscreen);

	printf("_ToggleFullscreen leave\n");
}

void
MainWin::_ToggleNoControls()
{
	printf("_ToggleNoControls enter\n");

	if (fIsFullscreen) {
		// fullscreen is always without menu	
		printf("_ToggleNoControls leave, doing nothing, we are fullscreen\n");
		return;
	}

	fNoControls = !fNoControls;
	_SetWindowSizeLimits();

	if (fNoControls) {
		ResizeBy(0, - fControlsHeight);
	} else {
		ResizeBy(0, fControlsHeight);
	}

	_MarkSettingsItem(M_TOGGLE_NO_CONTROLS, fNoControls);

	printf("_ToggleNoControls leave\n");
}

void
MainWin::_ToggleNoMenu()
{
	printf("_ToggleNoMenu enter\n");

	if (fIsFullscreen) {
		// fullscreen is always without menu	
		printf("_ToggleNoMenu leave, doing nothing, we are fullscreen\n");
		return;
	}

	fNoMenu = !fNoMenu;
	_SetWindowSizeLimits();

	if (fNoMenu) {
		MoveBy(0, fMenuBarHeight);
		ResizeBy(0, - fMenuBarHeight);
	} else {
		MoveBy(0, - fMenuBarHeight);
		ResizeBy(0, fMenuBarHeight);
	}

	_MarkSettingsItem(M_TOGGLE_NO_MENU, fNoMenu);

	printf("_ToggleNoMenu leave\n");
}


void
MainWin::_ToggleNoBorder()
{
	fNoBorder = !fNoBorder;
	SetLook(fNoBorder ? B_BORDERED_WINDOW_LOOK : B_TITLED_WINDOW_LOOK);

	_MarkSettingsItem(M_TOGGLE_NO_BORDER, fNoBorder);
}


void
MainWin::_ToggleAlwaysOnTop()
{
	fAlwaysOnTop = !fAlwaysOnTop;
	SetFeel(fAlwaysOnTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);

	_MarkSettingsItem(M_TOGGLE_ALWAYS_ON_TOP, fAlwaysOnTop);
}


void
MainWin::_ToggleKeepAspectRatio()
{
	fKeepAspectRatio = !fKeepAspectRatio;
	FrameResized(Bounds().Width(), Bounds().Height());

	_MarkSettingsItem(M_TOGGLE_KEEP_ASPECT_RATIO, fKeepAspectRatio);
}


// #pragma mark -


void
MainWin::_UpdateControlsEnabledStatus()
{
	uint32 enabledButtons = 0;
	if (fHasVideo || fHasAudio) {
		enabledButtons |= PLAYBACK_ENABLED | SEEK_ENABLED
			| SEEK_BACK_ENABLED | SEEK_FORWARD_ENABLED;
	}
	if (fHasAudio)
		enabledButtons |= VOLUME_ENABLED;

	bool canSkipPrevious, canSkipNext;
	fPlaylist->GetSkipInfo(&canSkipPrevious, &canSkipNext);
	if (canSkipPrevious)
		enabledButtons |= SKIP_BACK_ENABLED;
	if (canSkipNext)
		enabledButtons |= SKIP_FORWARD_ENABLED;

	fControls->SetEnabled(enabledButtons);
}


void
MainWin::_UpdatePlaylistMenu()
{
	if (!fPlaylist->Lock())
		return;

	fPlaylistMenu->RemoveItems(0, fPlaylistMenu->CountItems(), true);

	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		entry_ref ref;
		if (fPlaylist->GetRefAt(i, &ref) < B_OK)
			continue;
		_AddPlaylistItem(ref, i);
	}
	fPlaylistMenu->SetTargetForItems(this);

	_MarkPlaylistItem(fPlaylist->CurrentRefIndex());

	fPlaylist->Unlock();
}


void
MainWin::_AddPlaylistItem(const entry_ref& ref, int32 index)
{
	BMessage* message = new BMessage(M_SET_PLAYLIST_POSITION);
	message->AddInt32("index", index);
	BMenuItem* item = new BMenuItem(ref.name, message);
	fPlaylistMenu->AddItem(item, index);
}


void
MainWin::_RemovePlaylistItem(int32 index)
{
	delete fPlaylistMenu->RemoveItem(index);
}


void
MainWin::_MarkPlaylistItem(int32 index)
{
	if (BMenuItem* item = fPlaylistMenu->ItemAt(index)) {
		item->SetMarked(true);
		// ... and in case the menu is currently on screen:
		if (fPlaylistMenu->LockLooper()) {
			fPlaylistMenu->Invalidate();
			fPlaylistMenu->UnlockLooper();
		}
	}
}


void
MainWin::_MarkSettingsItem(uint32 command, bool mark)
{
	if (BMenuItem* item = fSettingsMenu->FindItem(command))
		item->SetMarked(mark);
}


