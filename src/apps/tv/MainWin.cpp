/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "MainWin.h"
#include "MainApp.h"
#include "Controller.h"
#include "config.h"
#include "DeviceRoster.h"

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Alert.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <String.h>
#include <View.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWin"

B_TRANSLATE_MARK_VOID("TV");
B_TRANSLATE_MARK_VOID("unknown");
B_TRANSLATE_MARK_VOID("DVB - Digital Video Broadcasting TV");

enum
{
	M_DUMMY = 0x100,
	M_FILE_QUIT,
	M_SCALE_TO_NATIVE_SIZE,
	M_TOGGLE_FULLSCREEN,
	M_TOGGLE_NO_BORDER,
	M_TOGGLE_NO_MENU,
	M_TOGGLE_NO_BORDER_NO_MENU,
	M_TOGGLE_ALWAYS_ON_TOP,
	M_TOGGLE_KEEP_ASPECT_RATIO,
	M_PREFERENCES,
	M_CHANNEL_NEXT,
	M_CHANNEL_PREV,
	M_VOLUME_UP,
	M_VOLUME_DOWN,
	M_ASPECT_100000_1,
	M_ASPECT_106666_1,
	M_ASPECT_109091_1,
	M_ASPECT_141176_1,
	M_ASPECT_720_576,
	M_ASPECT_704_576,
	M_ASPECT_544_576,
	M_SELECT_INTERFACE		= 0x00000800,
	M_SELECT_INTERFACE_END	= 0x00000fff,
	M_SELECT_CHANNEL		= 0x00010000,
	M_SELECT_CHANNEL_END	= 0x000fffff,
		// this limits possible channel count to 0xeffff = 983039
};

//#define printf(a...)


MainWin::MainWin(BRect frame_rect)
	:
	BWindow(frame_rect, B_TRANSLATE_SYSTEM_NAME(NAME), B_TITLED_WINDOW,
 	B_ASYNCHRONOUS_CONTROLS /* | B_WILL_ACCEPT_FIRST_CLICK */)
 ,	fController(new Controller)
 ,	fIsFullscreen(false)
 ,	fKeepAspectRatio(true)
 ,	fAlwaysOnTop(false)
 ,	fNoMenu(false)
 ,	fNoBorder(false)
 ,	fSourceWidth(720)
 ,	fSourceHeight(576)
 ,	fWidthScale(1.0)
 ,	fHeightScale(1.0)
 ,	fMouseDownTracking(false)
 ,	fFrameResizedTriggeredAutomatically(false)
 ,	fIgnoreFrameResized(false)
 ,	fFrameResizedCalled(true)
{
	BRect rect = Bounds();

	// background
	fBackground = new BView(rect, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	fBackground->SetViewColor(0,0,0);
	AddChild(fBackground);

	// menu
	fMenuBar = new BMenuBar(fBackground->Bounds(), "menu");
	CreateMenu();
	fBackground->AddChild(fMenuBar);
	fMenuBar->ResizeToPreferred();
	fMenuBarHeight = (int)fMenuBar->Frame().Height() + 1;
	fMenuBar->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);

	// video view
	BRect video_rect = BRect(0, fMenuBarHeight, rect.right, rect.bottom);
	fVideoView = new VideoView(video_rect, "video display", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	fBackground->AddChild(fVideoView);

	fVideoView->MakeFocus();

//	SetSizeLimits(fControlViewMinWidth - 1, 32767,
//		fMenuBarHeight + fControlViewHeight - 1, fMenuBarHeight
//		+ fControlViewHeight - 1);

//	SetSizeLimits(320 - 1, 32767, 240 + fMenuBarHeight - 1, 32767);

	SetSizeLimits(0, 32767, fMenuBarHeight - 1, 32767);

	fController->SetVideoView(fVideoView);
	fController->SetVideoNode(fVideoView->Node());

	fVideoView->IsOverlaySupported();

	SetupInterfaceMenu();
	SelectInitialInterface();
	SetInterfaceMenuMarker();
	SetupChannelMenu();
	SetChannelMenuMarker();

	VideoFormatChange(fSourceWidth, fSourceHeight, fWidthScale, fHeightScale);

	CenterOnScreen();
}


MainWin::~MainWin()
{
	printf("MainWin::~MainWin\n");
	fController->DisconnectInterface();
	delete fController;
}


void
MainWin::CreateMenu()
{
	fFileMenu = new BMenu(B_TRANSLATE(NAME));
	fChannelMenu = new BMenu(B_TRANSLATE("Channel"));
	fInterfaceMenu = new BMenu(B_TRANSLATE("Interface"));
	fSettingsMenu = new BMenu(B_TRANSLATE("Settings"));
	fDebugMenu = new BMenu(B_TRANSLATE("Debug"));

	fMenuBar->AddItem(fFileMenu);
	fMenuBar->AddItem(fChannelMenu);
	fMenuBar->AddItem(fInterfaceMenu);
	fMenuBar->AddItem(fSettingsMenu);
	fMenuBar->AddItem(fDebugMenu);

	fFileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(M_FILE_QUIT), 'Q', B_COMMAND_KEY));

/*
	fChannelMenu->AddItem(new BMenuItem(B_TRANSLATE("Next channel"),
		new BMessage(M_CHANNEL_NEXT), '+', B_COMMAND_KEY));
	fChannelMenu->AddItem(new BMenuItem(B_TRANSLATE("Previous channel"),
		new BMessage(M_CHANNEL_PREV), '-', B_COMMAND_KEY));
	fChannelMenu->AddSeparatorItem();
	fChannelMenu->AddItem(new BMenuItem("RTL", new BMessage(M_DUMMY), '0',
		B_COMMAND_KEY));
	fChannelMenu->AddItem(new BMenuItem("Pro7", new BMessage(M_DUMMY), '1',
		B_COMMAND_KEY));

	fInterfaceMenu->AddItem(new BMenuItem(B_TRANSLATE("none"),
		new BMessage(M_DUMMY)));
	fInterfaceMenu->AddItem(new BMenuItem(B_TRANSLATE("none 1"),
		new BMessage(M_DUMMY)));
	fInterfaceMenu->AddItem(new BMenuItem(B_TRANSLATE("none 2"),
		new BMessage(M_DUMMY)));
*/

	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Scale to native size"),
		new BMessage(M_SCALE_TO_NATIVE_SIZE), 'N', B_COMMAND_KEY));
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Full screen"),
		new BMessage(M_TOGGLE_FULLSCREEN), 'F', B_COMMAND_KEY));
	fSettingsMenu->AddSeparatorItem();
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("No menu"),
		new BMessage(M_TOGGLE_NO_MENU), 'M', B_COMMAND_KEY));
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("No border"),
		new BMessage(M_TOGGLE_NO_BORDER), 'B', B_COMMAND_KEY));
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Always on top"),
		new BMessage(M_TOGGLE_ALWAYS_ON_TOP), 'T', B_COMMAND_KEY));
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Keep aspect ratio"),
		new BMessage(M_TOGGLE_KEEP_ASPECT_RATIO), 'K', B_COMMAND_KEY));
	fSettingsMenu->AddSeparatorItem();
	fSettingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Settings"B_UTF8_ELLIPSIS)
		, new BMessage(M_PREFERENCES), 'P', B_COMMAND_KEY));

	const char* pixel_ratio = B_TRANSLATE("pixel aspect ratio");
	BString str1 = pixel_ratio;
	str1 << " 1.00000:1";
	fDebugMenu->AddItem(new BMenuItem(str1.String(),
		new BMessage(M_ASPECT_100000_1)));
	BString str2 = pixel_ratio;
	str2 << " 1.06666:1";
	fDebugMenu->AddItem(new BMenuItem(str2.String(),
		new BMessage(M_ASPECT_106666_1)));
	BString str3 = pixel_ratio;
	str3 << " 1.09091:1";
	fDebugMenu->AddItem(new BMenuItem(str3.String(),
		new BMessage(M_ASPECT_109091_1)));
	BString str4 = pixel_ratio;
	str4 << " 1.41176:1";
	fDebugMenu->AddItem(new BMenuItem(str4.String(),
		new BMessage(M_ASPECT_141176_1)));
	fDebugMenu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 720 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_720_576)));
	fDebugMenu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 704 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_704_576)));
	fDebugMenu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 544 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_544_576)));

	fSettingsMenu->ItemAt(1)->SetMarked(fIsFullscreen);
	fSettingsMenu->ItemAt(3)->SetMarked(fNoMenu);
	fSettingsMenu->ItemAt(4)->SetMarked(fNoBorder);
	fSettingsMenu->ItemAt(5)->SetMarked(fAlwaysOnTop);
	fSettingsMenu->ItemAt(6)->SetMarked(fKeepAspectRatio);
	fSettingsMenu->ItemAt(8)->SetEnabled(false);
		// XXX disable unused preference menu
}


void
MainWin::SetupInterfaceMenu()
{
	fInterfaceMenu->RemoveItems(0, fInterfaceMenu->CountItems(), true);

	fInterfaceMenu->AddItem(new BMenuItem(B_TRANSLATE("None"),
		new BMessage(M_SELECT_INTERFACE)));

	int count = gDeviceRoster->DeviceCount();

	if (count > 0)
		fInterfaceMenu->AddSeparatorItem();

	for (int i = 0; i < count; i++) {
		// 1 gets subtracted in MessageReceived, so -1 is Interface None,
		// and 0 == Interface 0 in SelectInterface()
		fInterfaceMenu->AddItem(new BMenuItem(gDeviceRoster->DeviceName(i),
			new BMessage(M_SELECT_INTERFACE + i + 1)));
	}
}


void
MainWin::SetupChannelMenu()
{
	fChannelMenu->RemoveItems(0, fChannelMenu->CountItems(), true);

	int interface = fController->CurrentInterface();
	printf("MainWin::SetupChannelMenu: interface %d\n", interface);

	int channels = fController->ChannelCount();

	if (channels == 0) {
		fChannelMenu->AddItem(new BMenuItem(B_TRANSLATE("None"),
			new BMessage(M_DUMMY)));
	} else {
		fChannelMenu->AddItem(new BMenuItem(B_TRANSLATE("Next channel"),
			new BMessage(M_CHANNEL_NEXT), '+', B_COMMAND_KEY));
		fChannelMenu->AddItem(new BMenuItem(B_TRANSLATE("Previous channel"),
			new BMessage(M_CHANNEL_PREV), '-', B_COMMAND_KEY));
		fChannelMenu->AddSeparatorItem();
	}

	for (int i = 0; i < channels; i++) {
		BString string;
		string.SetToFormat("%s%d %s", (i < 9) ? "  " : "", i + 1,
			fController->ChannelName(i));
		fChannelMenu->AddItem(new BMenuItem(string,
			new BMessage(M_SELECT_CHANNEL + i)));
	}
}


void
MainWin::SetInterfaceMenuMarker()
{
	BMenuItem *item;

	int interface = fController->CurrentInterface();
	printf("MainWin::SetInterfaceMenuMarker: interface %d\n", interface);

	// remove old marker
	item = fInterfaceMenu->FindMarked();
	if (item)
		item->SetMarked(false);

	// set new marker
	int index = (interface < 0) ? 0 : interface + 2;
	item = fInterfaceMenu->ItemAt(index);
	if (item)
		item->SetMarked(true);
}


void
MainWin::SetChannelMenuMarker()
{
	BMenuItem *item;

	int channel = fController->CurrentChannel();
	printf("MainWin::SetChannelMenuMarker: channel %d\n", channel);

	// remove old marker
	item = fChannelMenu->FindMarked();
	if (item)
		item->SetMarked(false);

	// set new marker
	int index = (channel < 0) ? 0 : channel + 3;
	item = fChannelMenu->ItemAt(index);
	if (item)
		item->SetMarked(true);
}


void
MainWin::SelectChannel(int i)
{
	printf("MainWin::SelectChannel %d\n", i);

	if (B_OK != fController->SelectChannel(i))
		return;

	SetChannelMenuMarker();
}


void
MainWin::SelectInterface(int i)
{
	printf("MainWin::SelectInterface %d\n", i);
	printf("  CurrentInterface %d\n", fController->CurrentInterface());
	printf("  CurrentChannel %d\n", fController->CurrentChannel());

	// i = -1 means "None"

	if (i < 0) {
		fController->DisconnectInterface();
		goto done;
	}

	if (!fController->IsInterfaceAvailable(i)) {
		BString s;
		s << B_TRANSLATE("Error, interface is busy:\n\n");
		s << gDeviceRoster->DeviceName(i);
		(new BAlert("error", s.String(), B_TRANSLATE("OK")))->Go();
		return;
	}

	fController->DisconnectInterface();
	if (fController->ConnectInterface(i) != B_OK) {
		BString s;
		s << B_TRANSLATE("Error, connecting to interface failed:\n\n");
		s << gDeviceRoster->DeviceName(i);
		(new BAlert("error", s.String(), B_TRANSLATE("OK")))->Go();
	}

done:
	printf("MainWin::SelectInterface done:\n");
	printf("  CurrentInterface %d\n", fController->CurrentInterface());
	printf("  CurrentChannel %d\n", fController->CurrentChannel());

	SetInterfaceMenuMarker();
	SetupChannelMenu();
	SetChannelMenuMarker();
}


void
MainWin::SelectInitialInterface()
{
	printf("MainWin::SelectInitialInterface enter\n");

	int count = gDeviceRoster->DeviceCount();
	for (int i = 0; i < count; i++) {
		if (fController->IsInterfaceAvailable(i)
			&& B_OK == fController->ConnectInterface(i)) {
			printf("MainWin::SelectInitialInterface connected to interface "
				"%d\n", i);
			break;
		}
	}

	printf("MainWin::SelectInitialInterface leave\n");
}


bool
MainWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWin::MouseDown(BMessage *msg)
{
	BPoint screen_where;
	uint32 buttons = msg->FindInt32("buttons");

	// On Zeta, only "screen_where" is relyable, "where" and "be:view_where"
	// seem to be broken
	if (B_OK != msg->FindPoint("screen_where", &screen_where)) {
		// Workaround for BeOS R5, it has no "screen_where"
		fVideoView->GetMouse(&screen_where, &buttons, false);
		fVideoView->ConvertToScreen(&screen_where);
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
			PostMessage(M_TOGGLE_NO_BORDER_NO_MENU);
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
		ShowContextMenu(screen_where);
}


void
MainWin::MouseMoved(BMessage *msg)
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
		// On Zeta, only "screen_where" is relyable, "where" and
		// "be:view_where" seem to be broken
		if (B_OK != msg->FindPoint("screen_where", &mousePos)) {
			// Workaround for BeOS R5, it has no "screen_where"
			fVideoView->GetMouse(&mousePos, &buttons, false);
			fVideoView->ConvertToScreen(&mousePos);
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
MainWin::MouseUp(BMessage *msg)
{
//	msg->PrintToStream();
	fMouseDownTracking = false;
}


void
MainWin::ShowContextMenu(const BPoint &screen_point)
{
	printf("Show context menu\n");
	BPopUpMenu *menu = new BPopUpMenu("context menu", false, false);
	BMenuItem *item;
	menu->AddItem(new BMenuItem(B_TRANSLATE("Scale to native size"),
		new BMessage(M_SCALE_TO_NATIVE_SIZE), 'N', B_COMMAND_KEY));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Full screen"),
		new BMessage(M_TOGGLE_FULLSCREEN), 'F', B_COMMAND_KEY));
	item->SetMarked(fIsFullscreen);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("No menu"),
		new BMessage(M_TOGGLE_NO_MENU), 'M', B_COMMAND_KEY));
	item->SetMarked(fNoMenu);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("No border"),
		new BMessage(M_TOGGLE_NO_BORDER), 'B', B_COMMAND_KEY));
	item->SetMarked(fNoBorder);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Always on top"),
		new BMessage(M_TOGGLE_ALWAYS_ON_TOP), 'T', B_COMMAND_KEY));
	item->SetMarked(fAlwaysOnTop);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Keep aspect ratio"),
		new BMessage(M_TOGGLE_KEEP_ASPECT_RATIO), 'K', B_COMMAND_KEY));
	item->SetMarked(fKeepAspectRatio);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(M_FILE_QUIT), 'Q', B_COMMAND_KEY));

	menu->AddSeparatorItem();
	const char* pixel_aspect = "pixel aspect ratio";
	BString str1 = pixel_aspect;
	str1 << " 1.00000:1";
	menu->AddItem(new BMenuItem(str1.String(),
		new BMessage(M_ASPECT_100000_1)));
	BString str2 = pixel_aspect;
	str2 << " 1.06666:1";
	menu->AddItem(new BMenuItem(str2.String(),
		new BMessage(M_ASPECT_106666_1)));
	BString str3 = pixel_aspect;
	str3 << " 1.09091:1";
	menu->AddItem(new BMenuItem(str3.String(),
		new BMessage(M_ASPECT_109091_1)));
	BString str4 = pixel_aspect;
	str4 << " 1.41176:1";
	menu->AddItem(new BMenuItem(str4.String(),
		new BMessage(M_ASPECT_141176_1)));
	menu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 720 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_720_576)));
	menu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 704 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_704_576)));
	menu->AddItem(new BMenuItem(B_TRANSLATE(
		"force 544 x 576, display aspect 4:3"),
		new BMessage(M_ASPECT_544_576)));

	menu->SetTargetForItems(this);
	BRect r(screen_point.x - 5, screen_point.y - 5, screen_point.x + 5,
		screen_point.y + 5);
	menu->Go(screen_point, true, true, r, true);
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

//	ResizeWindow(Bounds().Width() + 1, Bounds().Height() + 1, true);

	if (fIsFullscreen) {
		AdjustFullscreenRenderer();
	} else {
		AdjustWindowedRenderer(false);
	}

	printf("VideoFormatChange leave\n");

}


void
MainWin::Zoom(BPoint rec_position, float rec_width, float rec_height)
{
	PostMessage(M_TOGGLE_FULLSCREEN);
}


void
MainWin::FrameResized(float new_width, float new_height)
{
	// called when the window got resized
	fFrameResizedCalled = true;

	if (new_width != Bounds().Width() || new_height != Bounds().Height()) {
		debugger("size wrong\n");
	}


	printf("FrameResized enter: new_width %.0f, new_height %.0f, bounds width "
		"%.0f, bounds height %.0f\n", new_width, new_height, Bounds().Width(),
		Bounds().Height());

	if (fIsFullscreen) {

		printf("FrameResized in fullscreen mode\n");

		fIgnoreFrameResized = false;
		AdjustFullscreenRenderer();

	} else {

		if (fFrameResizedTriggeredAutomatically) {
			fFrameResizedTriggeredAutomatically = false;
			printf("FrameResized triggered automatically\n");

			fIgnoreFrameResized = false;

			AdjustWindowedRenderer(false);
		} else {
			printf("FrameResized by user in window mode\n");

			if (fIgnoreFrameResized) {
				fIgnoreFrameResized = false;
				printf("FrameResized ignored\n");
				return;
			}

			AdjustWindowedRenderer(true);
		}

	}

	printf("FrameResized leave\n");
}



void
MainWin::UpdateWindowTitle()
{
	BString title;
	title.SetToFormat("%s - %d x %d, %.3f:%.3f => %.0f x %.0f",
		B_TRANSLATE_SYSTEM_NAME(NAME),
		fSourceWidth, fSourceHeight, fWidthScale, fHeightScale,
		fVideoView->Bounds().Width() + 1, fVideoView->Bounds().Height() + 1);
	SetTitle(title);
}


void
MainWin::AdjustFullscreenRenderer()
{
	// n.b. we don't have a menu in fullscreen mode!

	if (fKeepAspectRatio) {

		// Keep aspect ratio, place render inside
		// the background area (may create black bars).
		float max_width  = fBackground->Bounds().Width() + 1.0f;
		float max_height = fBackground->Bounds().Height() + 1.0f;
		float scaled_width  = fSourceWidth * fWidthScale;
		float scaled_height = fSourceHeight * fHeightScale;
		float factor = min_c(max_width / scaled_width, max_height
			/ scaled_height);
		int render_width = int(scaled_width * factor);
		int render_height = int(scaled_height * factor);
		int x_ofs = (int(max_width) - render_width) / 2;
		int y_ofs = (int(max_height) - render_height) / 2;

		printf("AdjustFullscreenRenderer: background %.1f x %.1f, src video "
			"%d x %d, scaled video %.3f x %.3f, factor %.3f, render %d x %d, "
			"x-ofs %d, y-ofs %d\n", max_width, max_height, fSourceWidth,
			fSourceHeight, scaled_width, scaled_height, factor, render_width,
			render_height, x_ofs, y_ofs);

		fVideoView->MoveTo(x_ofs, y_ofs);
		fVideoView->ResizeTo(render_width - 1, render_height - 1);

	} else {

		printf("AdjustFullscreenRenderer: using whole background area\n");

		// no need to keep aspect ratio, make
		// render cover the whole background
		fVideoView->MoveTo(0, 0);
		fVideoView->ResizeTo(fBackground->Bounds().Width(),
			fBackground->Bounds().Height());

	}
}


void
MainWin::AdjustWindowedRenderer(bool user_resized)
{
	printf("AdjustWindowedRenderer enter - user_resized %d\n", user_resized);

	// In windowed mode, the renderer always covers the
	// whole background, accounting for the menu
	fVideoView->MoveTo(0, fNoMenu ? 0 : fMenuBarHeight);
	fVideoView->ResizeTo(fBackground->Bounds().Width(),
		fBackground->Bounds().Height() - (fNoMenu ? 0 : fMenuBarHeight));

	if (fKeepAspectRatio) {
		// To keep the aspect ratio correct, we
		// do resize the window as required

		float max_width  = Bounds().Width() + 1.0f;
		float max_height = Bounds().Height() + 1.0f - (fNoMenu ? 0
			: fMenuBarHeight);
		float scaled_width  = fSourceWidth * fWidthScale;
		float scaled_height = fSourceHeight * fHeightScale;

		if (!user_resized && (scaled_width > max_width
			|| scaled_height > max_height)) {
			// A format switch occured, and the window was
			// smaller then the video source. As it was not
			// initiated by the user resizing the window, we
			// enlarge the window to fit the video.
			fIgnoreFrameResized = true;
			ResizeTo(scaled_width - 1, scaled_height - 1
				+ (fNoMenu ? 0 : fMenuBarHeight));
//			Sync();
			return;
		}

		float display_aspect_ratio = scaled_width / scaled_height;
		int new_width  = int(max_width);
		int new_height = int(max_width / display_aspect_ratio + 0.5);

		printf("AdjustWindowedRenderer: old display %d x %d, src video "
			"%d x %d, scaled video %.3f x %.3f, aspect ratio %.3f, new "
			"display %d x %d\n", int(max_width), int(max_height),
			fSourceWidth, fSourceHeight, scaled_width, scaled_height,
			display_aspect_ratio, new_width, new_height);

		fIgnoreFrameResized = true;
		ResizeTo(new_width - 1, new_height - 1 + (fNoMenu ? 0
			: fMenuBarHeight));
//		Sync();
	}

	printf("AdjustWindowedRenderer leave\n");
}


void
MainWin::ToggleNoBorderNoMenu()
{
	if (!fNoMenu && fNoBorder) {
		// if no border, switch of menu, too
		PostMessage(M_TOGGLE_NO_MENU);
	} else
	if (fNoMenu && !fNoBorder) {
		// if no menu, switch of border, too
		PostMessage(M_TOGGLE_NO_BORDER);
	} else {
		// both are either on or off, toggle both
		PostMessage(M_TOGGLE_NO_MENU);
		PostMessage(M_TOGGLE_NO_BORDER);
	}
}


void
MainWin::ToggleFullscreen()
{
	printf("ToggleFullscreen enter\n");

	if (!fFrameResizedCalled) {
		printf("ToggleFullscreen - ignoring, as FrameResized wasn't called "
			"since last switch\n");
		return;
	}
	fFrameResizedCalled = false;


	fIsFullscreen = !fIsFullscreen;

	if (fIsFullscreen) {
		// switch to fullscreen

		// Sync here is probably not required
//		Sync();

		fSavedFrame = Frame();
		printf("saving current frame: %d %d %d %d\n", int(fSavedFrame.left),
			int(fSavedFrame.top), int(fSavedFrame.right),
			int(fSavedFrame.bottom));
		BScreen screen(this);
		BRect rect(screen.Frame());

		Hide();
		if (!fNoMenu) {
			// if we have a menu, remove it now
			fMenuBar->Hide();
		}
		fFrameResizedTriggeredAutomatically = true;
		MoveTo(rect.left, rect.top);
		ResizeTo(rect.Width(), rect.Height());
		Show();

//		Sync();

	} else {
		// switch back from full screen mode

		Hide();
		// if we need a menu, show it now
		if (!fNoMenu) {
			fMenuBar->Show();
		}
		fFrameResizedTriggeredAutomatically = true;
		MoveTo(fSavedFrame.left, fSavedFrame.top);
		ResizeTo(fSavedFrame.Width(), fSavedFrame.Height());
		Show();

		// We *must* make sure that the window is at
		// the correct position before continuing, or
		// rapid fullscreen switching by holding down
		// the TAB key will expose strange bugs.
		// Never remove this Sync!
//		Sync();
	}

	// FrameResized() will do the required adjustments

	printf("ToggleFullscreen leave\n");
}


void
MainWin::ToggleNoMenu()
{
	printf("ToggleNoMenu enter\n");

	fNoMenu = !fNoMenu;

	if (fIsFullscreen) {
		// fullscreen is always without menu
		printf("ToggleNoMenu leave, doing nothing, we are fullscreen\n");
		return;
	}

//	fFrameResizedTriggeredAutomatically = true;
	fIgnoreFrameResized = true;

	if (fNoMenu) {
		fMenuBar->Hide();
		fVideoView->MoveTo(0, 0);
		fVideoView->ResizeBy(0, fMenuBarHeight);
		MoveBy(0, fMenuBarHeight);
		ResizeBy(0, - fMenuBarHeight);
//		Sync();
	} else {
		fMenuBar->Show();
		fVideoView->MoveTo(0, fMenuBarHeight);
		fVideoView->ResizeBy(0, -fMenuBarHeight);
		MoveBy(0, - fMenuBarHeight);
		ResizeBy(0, fMenuBarHeight);
//		Sync();
	}

	printf("ToggleNoMenu leave\n");
}


void
MainWin::ToggleNoBorder()
{
	printf("ToggleNoBorder enter\n");
	fNoBorder = !fNoBorder;
//	SetLook(fNoBorder ? B_NO_BORDER_WINDOW_LOOK : B_TITLED_WINDOW_LOOK);
	SetLook(fNoBorder ? B_BORDERED_WINDOW_LOOK : B_TITLED_WINDOW_LOOK);
	printf("ToggleNoBorder leave\n");
}


void
MainWin::ToggleAlwaysOnTop()
{
	printf("ToggleAlwaysOnTop enter\n");
	fAlwaysOnTop = !fAlwaysOnTop;
	SetFeel(fAlwaysOnTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
	printf("ToggleAlwaysOnTop leave\n");
}


void
MainWin::ToggleKeepAspectRatio()
{
	printf("ToggleKeepAspectRatio enter\n");
	fKeepAspectRatio = !fKeepAspectRatio;

	fFrameResizedTriggeredAutomatically = true;
	FrameResized(Bounds().Width(), Bounds().Height());
//	if (fIsFullscreen) {
//		AdjustFullscreenRenderer();
//	} else {
//		AdjustWindowedRenderer(false);
//	}
	printf("ToggleKeepAspectRatio leave\n");
}


/* Trap keys that are about to be send to background or renderer view.
 * Return B_OK if it shouldn't be passed to the view
 */
status_t
MainWin::KeyDown(BMessage *msg)
{
//	msg->PrintToStream();

	uint32 key		 = msg->FindInt32("key");
	uint32 raw_char  = msg->FindInt32("raw_char");
	uint32 modifiers = msg->FindInt32("modifiers");

	printf("key 0x%lx, raw_char 0x%lx, modifiers 0x%lx\n", key, raw_char,
		modifiers);

	switch (raw_char) {
		case B_SPACE:
			PostMessage(M_TOGGLE_NO_BORDER_NO_MENU);
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
				PostMessage(M_CHANNEL_NEXT);
			} else {
				PostMessage(M_VOLUME_UP);
			}
			return B_OK;

		case B_DOWN_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_CHANNEL_PREV);
			} else {
				PostMessage(M_VOLUME_DOWN);
			}
			return B_OK;

		case B_RIGHT_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_VOLUME_UP);
			} else {
				PostMessage(M_CHANNEL_NEXT);
			}
			return B_OK;

		case B_LEFT_ARROW:
			if (modifiers & B_COMMAND_KEY) {
				PostMessage(M_VOLUME_DOWN);
			} else {
				PostMessage(M_CHANNEL_PREV);
			}
			return B_OK;

		case B_PAGE_UP:
			PostMessage(M_CHANNEL_NEXT);
			return B_OK;

		case B_PAGE_DOWN:
			PostMessage(M_CHANNEL_PREV);
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
			PostMessage(M_CHANNEL_NEXT);
			return B_OK;

		case 0x5a:			// numeric keypad page down
		case 0x48:			// numeric keypad left arrow
			PostMessage(M_CHANNEL_PREV);
			return B_OK;
	}

	return B_ERROR;
}


void
MainWin::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if ((msg->what == B_MOUSE_DOWN) && (handler == fBackground
		|| handler == fVideoView))
		MouseDown(msg);
	if ((msg->what == B_MOUSE_MOVED) && (handler == fBackground
		|| handler == fVideoView))
		MouseMoved(msg);
	if ((msg->what == B_MOUSE_UP) && (handler == fBackground
		|| handler == fVideoView))
		MouseUp(msg);

	if ((msg->what == B_KEY_DOWN) && (handler == fBackground
		|| handler == fVideoView)) {

		// special case for PrintScreen key
		if (msg->FindInt32("key") == B_PRINT_KEY) {
			fVideoView->OverlayScreenshotPrepare();
			BWindow::DispatchMessage(msg, handler);
			fVideoView->OverlayScreenshotCleanup();
			return;
		}

		// every other key gets dispatched to our KeyDown first
		if (KeyDown(msg) == B_OK) {
			// it got handled, don't pass it on
			return;
		}
	}

	BWindow::DispatchMessage(msg, handler);
}


void
MainWin::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
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
			if (dx > 0.1)	PostMessage(inv ? M_VOLUME_DOWN : M_CHANNEL_PREV);
			if (dx < -0.1)	PostMessage(inv ? M_VOLUME_UP : M_CHANNEL_NEXT);
			if (dy > 0.1)	PostMessage(inv ? M_CHANNEL_PREV : M_VOLUME_DOWN);
			if (dy < -0.1)	PostMessage(inv ? M_CHANNEL_NEXT : M_VOLUME_UP);
			break;
		}

		case M_CHANNEL_NEXT:
		{
			printf("M_CHANNEL_NEXT\n");
			int chan = fController->CurrentChannel();
			if (chan != -1) {
				chan++;
				if (chan < fController->ChannelCount())
					SelectChannel(chan);
			}
			break;
		}

		case M_CHANNEL_PREV:
		{
			printf("M_CHANNEL_PREV\n");
			int chan = fController->CurrentChannel();
			if (chan != -1) {
				chan--;
				if (chan >= 0)
					SelectChannel(chan);
			}
			break;
		}

		case M_VOLUME_UP:
			printf("M_VOLUME_UP\n");
			fController->VolumeUp();
			break;

		case M_VOLUME_DOWN:
			printf("M_VOLUME_DOWN\n");
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

		case B_REFS_RECEIVED:
			printf("MainWin::MessageReceived: B_REFS_RECEIVED\n");
//			RefsReceived(msg);
			break;

		case B_SIMPLE_DATA:
			printf("MainWin::MessageReceived: B_SIMPLE_DATA\n");
//			if (msg->HasRef("refs"))
//				RefsReceived(msg);
			break;

		case M_FILE_QUIT:
//			be_app->PostMessage(B_QUIT_REQUESTED);
			PostMessage(B_QUIT_REQUESTED);
			break;

		case M_SCALE_TO_NATIVE_SIZE:
			printf("M_SCALE_TO_NATIVE_SIZE\n");
			if (fIsFullscreen) {
				ToggleFullscreen();
			}
			ResizeTo(int(fSourceWidth * fWidthScale),
					 int(fSourceHeight * fHeightScale) + (fNoMenu ? 0
					 	: fMenuBarHeight));
//			Sync();
			break;

		case M_TOGGLE_FULLSCREEN:
			ToggleFullscreen();
			fSettingsMenu->ItemAt(1)->SetMarked(fIsFullscreen);
			break;

		case M_TOGGLE_NO_MENU:
			ToggleNoMenu();
			fSettingsMenu->ItemAt(3)->SetMarked(fNoMenu);
			break;

		case M_TOGGLE_NO_BORDER:
			ToggleNoBorder();
			fSettingsMenu->ItemAt(4)->SetMarked(fNoBorder);
			break;

		case M_TOGGLE_ALWAYS_ON_TOP:
			ToggleAlwaysOnTop();
			fSettingsMenu->ItemAt(5)->SetMarked(fAlwaysOnTop);
			break;

		case M_TOGGLE_KEEP_ASPECT_RATIO:
			ToggleKeepAspectRatio();
			fSettingsMenu->ItemAt(6)->SetMarked(fKeepAspectRatio);
			break;

		case M_TOGGLE_NO_BORDER_NO_MENU:
			ToggleNoBorderNoMenu();
			break;

		case M_PREFERENCES:
			break;

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
	}
}

