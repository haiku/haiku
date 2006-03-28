/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// ***************************************************************************************
//	BeMenu configuration
//
//	add temp item on dnd with new group
//	save 	window location
//			selection information
//			last group showing
//			recents
//	auto scroll of contentsmenu
//	default settings
//	off screen drawing
//	return/cancel in editingtext for name invokes name change
//	after New Folder, scroll to selection
//	move of items to folders
//	change parent folder btn to be a NavMenu
//	
// ***************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <SymLink.h>
#include <TextControl.h>
#include <TextView.h>
#include <Volume.h>

//	Tracker
#include "Commands.h"
#include "IconCache.h"
#include "IconMenuItem.h"
#include "Model.h"
#include "ObjectList.h"
#include "tracker_private.h"
#include "Utilities.h"

#include "FavoritesConfig.h"


enum {
	kRemove = 'rmve',
	kAdd,
	kRecentDocs,
	kRecentFolders,
	kRecentApps,
	kNewGroup,
	kItemSelected,
	kOpenItem,
	kEditItem,
	kDoubleClick,
	kRecentDocsCount,
	kRecentFoldersCount,
	kRecentAppsCount,
	kScrollUp,
	kScrollDown,
	kShowGroup,
	kTraverseUp,
	kNameChange
};

const rgb_color kAlmostWhite = {232, 232, 232, 255};
const rgb_color kLightGray = {136, 136, 136, 255};
const rgb_color kMediumGray = {96, 96, 96, 255};

const float kMinBtnWidth = 75.0f;
const float kMenuFieldPad = 30.0f;
const float kMenuFieldGap = 5.0f;
const float kCheckBoxPad = 20.0f;
const float kTextControlPad = 15.0f;

const char *const kOpenStr = "Open";
const char *const kRemoveStr = "Remove";
const char *const kShowGroupStr = "Show Group";
const char *const kSortByStr = "Sort By";

const char *const kNewGroupStr = "New Group";
const char *const kRecentDocsStr = "Recent Documents";
const char *const kRecentFoldersStr = "Recent Folders";
const char *const kRecentAppsStr = "Recent Applications";
const char *const kShowStr = "Show:";

const unsigned char kLargeNewGroupIcon [] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xfa, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xf8, 0xfa, 0xfa, 0x00, 0x00, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xfa, 0xf8, 0xf8, 0xf8, 0xfa, 0xfa, 0x00,
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xfa, 0x00, 0x00, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xfa,
	0xfa, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xf8, 0xfa, 0xfa, 0x00, 0x00, 0xf8, 0xf8, 0xf8, 0xf8,
	0xf8, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xf8, 0xf8, 0xf8, 0xfa, 0xfa, 0x00, 0x00, 0xf8, 0xf8,
	0xf8, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xfa, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xfa, 0xfa, 0x00, 0xf8,
	0xf8, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x0e,
	0x0f, 0x1c, 0x1c, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x0f, 0x0f, 0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x3f,
	0x3f, 0x0e, 0x0f, 0x1c, 0x1c, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x0e, 0x0f, 0xf8, 0xf8, 0x00, 0x00,
	0x3f, 0x3f, 0x3f, 0x0e, 0x0f, 0x1c, 0x1c, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x0e, 0x0f, 0x1c, 0x1c,
	0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x0e, 0x0f, 0x1a, 0x19, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x0e, 0x0f,
	0x1c, 0x1c, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x0e, 0x1a, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x0e, 0x0f, 0x1c, 0x1c, 0x00, 0x00, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x0e, 0x0f, 0x1a, 0x19, 0x00, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x0e, 0x1a, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x0f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x18, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1b, 0x1c, 0x17, 0x18, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x1b, 0x1c, 0x17, 0x18, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x19, 0x1a, 0x17, 0x17, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x19, 0x1a, 0x17, 0x17,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x17, 0x1a, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x19, 0x1a,
	0x17, 0x17, 0x3f, 0x3f, 0x3f, 0x17, 0x19, 0x00, 0x1a, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
	0x19, 0x1a, 0x17, 0x17, 0x3f, 0x17, 0x19, 0x00, 0x00, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x19, 0x1a, 0x17, 0x17, 0x1a, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x19, 0x1a, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x19, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned char kSmallNewGroupIcon [] = {
	0xff, 0xff, 0xff, 0xff, 0x0e, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x0e, 0xfa, 0xfa, 0x0e, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x0e, 0xf8, 0xf8, 0xfa, 0xfa, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0xfa, 0xfa, 0x0e, 0xf8, 0xf8, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0xf8, 0xf8, 0xfa, 0xfa, 0x0e, 0x0e, 0x0e, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x18, 0x18, 0xf8, 0xf8, 0x0e, 0x3f, 0x3f, 0x0e, 0x0e, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x18, 0x3f, 0x18, 0x18, 0x0f, 0x0f, 0x3f, 0x3f, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x18, 0x3f, 0x3f, 0x3f, 0x18, 0x18, 0x0f, 0x0f, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x18, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x18, 0x18, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x18, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1c, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x0e, 0x0e, 0x18, 0x1c, 0x3f, 0x3f, 0x3f, 0x3f, 0x1c, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0x0e, 0x0e, 0x17, 0x1c, 0x3f, 0x3f, 0x1c, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0e, 0x0e, 0x17, 0x1c, 0x1c, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0e, 0x0e, 0x17, 0x0e, 0x12, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0e, 0x0e, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


void
CenterWindowOnScreen(BWindow *window)
{
	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint point;
	point.x = screenFrame.Width() / 2 - window->Bounds().Width() / 2;
	point.y = screenFrame.Height() / 2 - window->Bounds().Height() / 2;

	if (screenFrame.Contains(point))
		window->MoveTo(point);
}


float
FontHeight(const BFont *font, bool full)
{
	font_height finfo;		
	font->GetHeight(&finfo);
	float height = finfo.ascent + finfo.descent;

	if (full)
		height += finfo.leading;
	
	return height;
}


//	#pragma mark -


TFavoritesConfigWindow::TFavoritesConfigWindow(BRect frame, const char *title,
	bool modal, uint32 filePanelNodeFlavors, BMessenger parent, const entry_ref *startRef,
	int32 maxApps, int32 maxDocs, int32 maxFolders)
	:	BWindow(frame, title, B_TITLED_WINDOW_LOOK,
			modal ? B_MODAL_APP_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
		fFilePanelNodeFlavors(filePanelNodeFlavors),
		fParent(parent),
		fCurrentRef(*startRef),
		fAddPanel(NULL)
{
	Lock();

	//	show the window offscreen
	//	so that the placement calculations actually work
	MoveTo(-1024, -1024);
	Show();
	AddParts(maxApps, maxDocs, maxFolders);
	CenterWindowOnScreen(this);
	Unlock();

	AddShortcut('R', B_COMMAND_KEY, new BMessage(kRemove));
	AddShortcut('A', B_COMMAND_KEY, new BMessage(kAdd));
	AddShortcut('E', B_COMMAND_KEY, new BMessage(kEditItem));
	AddShortcut('O', B_COMMAND_KEY, new BMessage(kOpenItem));
	AddShortcut('N', B_COMMAND_KEY, new BMessage(kNewGroup));

	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(kTraverseUp));

	BMessenger tracker(kTrackerSignature);
	StartWatching(tracker, kFavoriteCountChanged);
}


TFavoritesConfigWindow::~TFavoritesConfigWindow()
{
	//	node monitoring the be menu directory
	stop_watching(this);
	
	if (fAddPanel && fAddPanel->IsShowing()) 
		//	kill the filepanel if its still showing
		fAddPanel->Hide();		

	BMessenger tracker(kTrackerSignature);
	StopWatching(tracker, kFavoriteCountChanged);
}


void 
TFavoritesConfigWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		//	from menuthing selection
		case kItemSelected:
			UpdateButtons();
			break;

		//	double click on item in menuthing
		case kDoubleClick:
			{				
				entry_ref ref;
				if (message->FindRef("current", &ref) == B_OK)
					OpenGroup(&ref);
			}
			break;

		//	open button
		case kOpenItem:
			if (fMenuThing->Value() >= 0 && fMenuThing->Value() < fMenuThing->ItemCount())
				OpenGroup(fMenuThing->ItemAt(fMenuThing->Value())->EntryRef());
			break;
			
		case kEditItem:
			{
				int32 selection = fMenuThing->Value();
				if (selection < 0)
					break;
				const Model *item = fMenuThing->ItemAt(selection);
				if (!item)
					break;
		
				new NameItemPanel(this, item->Name());
				//	shows itself, kills itself
			}
			break;
			
		case 'canc':
			break;

		case kNameChange:
			{
				const char *name;
				if (message->FindString("name", &name) == B_OK) {
					int32 selection = fMenuThing->Value();
					if (selection < 0)
						break;
					const Model *item = fMenuThing->ItemAt(selection);
					if (!item)
						break;

					const entry_ref *ref = item->EntryRef();
					if (strcmp(ref->name, name) != 0) {							
						BEntry entry(ref);
						if (entry.InitCheck() == B_OK && entry.Exists())
							entry.Rename(name);
					}
				}
			}
			break;
			
		//	change of selection in directory menu
		case kShowGroup:
			{				
				entry_ref ref;
				if (message->FindRef("current", &ref) == B_OK)
					ShowGroup(&ref);
			}
			break;

		//	alt-up arrow
		case kTraverseUp:
			{
				BMenuItem *item = fGroupMenu->FindMarked();
				if (item) {
					int32 index = fGroupMenu->IndexOf(item) - 1;
					if (index < 0)
						break;

					item = fGroupMenu->ItemAt(index);
					if (item) {
						BMessage *message = item->Message();
						if (message) {
							entry_ref ref;
							if (message->FindRef("current", &ref) == B_OK) {							
								ShowGroup(&ref);
							}
						}						
					}
				}
			}
			break;

		//	new group icon
		case kNewGroup:
			AddNewGroup();			
			break;

		//	Add button
		case kAdd:
			PromptForAdd();
			break;

		//	from Add btn - FilePanel
		case B_REFS_RECEIVED:
			AddRefs(message);
			// fall through
		case B_CANCEL:
			fAddPanel = NULL;
			break;

		//	remove btn
		case kRemove:
			fMenuThing->RemoveItem(fMenuThing->Value());
			break;

		//	recents items
		case kRecentFolders:
			fRecentFoldersFld->SetEnabled(fRecentFoldersBtn->Value() != 0);
			if (fRecentFoldersBtn->Value())
				UpdateFoldersCount();
			else
				UpdateFoldersCount(0);
			break;

		case kRecentDocs:
			fRecentDocsFld->SetEnabled(fRecentDocsBtn->Value() != 0);
			if (fRecentDocsBtn->Value())
				UpdateDocsCount();
			else
				UpdateDocsCount(0);
			break;

		case kRecentApps:
			fRecentAppsFld->SetEnabled(fRecentAppsBtn->Value() != 0);
			if (fRecentAppsBtn->Value())
				UpdateAppsCount();
			else
				UpdateAppsCount(0);
			break;
			
		case kRecentFoldersCount:
			UpdateFoldersCount();
			break;

		case kRecentDocsCount:
			UpdateDocsCount();
			break;

		case kRecentAppsCount:
			UpdateAppsCount();
			break;

		case B_OBSERVER_NOTICE_CHANGE:
			{
				int32 observerWhat;
				if (message->FindInt32("be:observe_change_what", &observerWhat) == B_OK) {
					switch (observerWhat) {
						case kFavoriteCountChanged:
							{
								int32 appCount = 10;
								int32 documentCount = 10;
								int32 folderCount = 10;
								
								if (message->FindInt32("RecentApplications", &appCount) == B_OK) {
									BString appStr;
									appStr << appCount;
									fRecentAppsFld->SetText(appStr.String());
									UpdateAppsCount(appCount, false);
										// false -> do not tell tracker
								}
								
								if (message->FindInt32("RecentDocuments", &documentCount) == B_OK) {
									BString docStr;
									docStr << documentCount;
									fRecentDocsFld->SetText(docStr.String());
									UpdateDocsCount(documentCount, false);
										// false -> do not tell tracker
								}
								
								if (message->FindInt32("RecentFolders", &folderCount) == B_OK) {
									BString folderStr;
									folderStr << folderCount;
									fRecentFoldersFld->SetText(folderStr.String());
									UpdateFoldersCount(folderCount, false);
										// false -> do not tell tracker
								}
							}
							break;
					}
				}		
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool 
TFavoritesConfigWindow::QuitRequested()
{
	//	tell the app the config panel is closing
	BMessage message(kConfigClose);
	int32 count;
	if (fRecentAppsFld) {
		if (fRecentAppsFld->IsEnabled())
			count = atoi(fRecentAppsFld->Text());
		else
			count = 0;
		message.AddInt32("applications", count);
	}
	if (fRecentFoldersFld) {
		if (fRecentFoldersFld->IsEnabled())
			count = atoi(fRecentFoldersFld->Text());
		else
			count = 0;
		message.AddInt32("folders", count);
	}
	if (fRecentDocsFld) {
		if (fRecentDocsFld->IsEnabled())
			count = atoi(fRecentDocsFld->Text());
		else
			count = 0;
		message.AddInt32("documents", count);
	}
	
	fParent.SendMessage(&message);
	return true;
}


void
TFavoritesConfigWindow::AddParts(int32 maxApps, int32 maxDocs, int32 maxFolders)
{
	// build the interface
	AddBeMenuPane(maxApps, maxDocs, maxFolders);

	// find the bg with the largest dimensions
	ResizeTo(fBeMenuPaneBG->Frame().Width(), fBeMenuPaneBG->Frame().Height());	

	// enable node watching
	// fill the pseudo menu
	OpenGroup(&fCurrentRef);
}


void
TFavoritesConfigWindow::BuildCommon(BRect *frame, int32 count, const char *string,
	uint32 btnWhat, uint32 fldWhat, BCheckBox **button, BTextControl **field, BBox *parent)
{
	frame->right = frame->left + be_plain_font->StringWidth(string)
		+ kCheckBoxPad;		

	BCheckBox *newButton = new BCheckBox(*frame, "recents btn", string,
		new BMessage(btnWhat), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT,
		B_WILL_DRAW | B_NAVIGABLE);
	parent->AddChild(newButton);
	newButton->SetValue(count > 0);
	
	float width = be_plain_font->StringWidth(kShowStr);
	frame->right = frame->left + width
		+ (be_plain_font->StringWidth("0") * 4) + kTextControlPad;		
	BTextControl *newFld = new BTextControl(*frame, "recents fld", kShowStr, "",
		new BMessage(fldWhat), B_FOLLOW_BOTTOM | B_FOLLOW_LEFT,
		B_WILL_DRAW | B_NAVIGABLE);
	parent->AddChild(newFld);
	newFld->SetDivider(width + 5);
	newFld->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_CENTER);
	newFld->SetEnabled(count>0);
	
	char str[32];
	sprintf(str, "%ld", count);
	newFld->SetText(str);

	BTextView *textView = newFld->TextView();
	textView->SetMaxBytes(2);

	for (uint32 index = 0; index < 256; index++)
		textView->DisallowChar(index);
	for (uint32 index = '0'; index <= '9'; index++)
		textView->AllowChar(index);
	textView->AllowChar(B_BACKSPACE);
	
	*button = newButton;
	*field = newFld;
}


void
TFavoritesConfigWindow::AddBeMenuPane(int32 maxApps, int32 maxDocs, int32 maxFolders)
{
	fBeMenuPaneBG = new BBox(Bounds(), "bg", B_FOLLOW_NONE,
		B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(fBeMenuPaneBG);
	
	BRect frame;
	float width = 0;
	
	//	New Group
	fNewGroupBtn = new TDraggableIconButton(BRect(10, 0, 41, 31), kNewGroupStr,
		new BMessage(kNewGroup), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE);
	fBeMenuPaneBG->AddChild(fNewGroupBtn);

	//	Recent Documents
	frame.left = 10;
	if (maxDocs > -1) {
		BuildCommon(&frame, maxDocs, kRecentDocsStr,
			kRecentDocs, kRecentDocsCount, &fRecentDocsBtn, &fRecentDocsFld,
			fBeMenuPaneBG);
	} else {
		fRecentDocsBtn = NULL;
		fRecentDocsFld = NULL;
	}

	//	Recent Applications
	if (maxApps > -1) {
		BuildCommon(&frame, maxApps, kRecentAppsStr,
			kRecentApps, kRecentAppsCount, &fRecentAppsBtn, &fRecentAppsFld,
			fBeMenuPaneBG);
	} else {
		fRecentAppsBtn = NULL;
		fRecentAppsFld = NULL;
	}
	
	//	Recent Folders
	if (maxFolders > -1) {
		BuildCommon(&frame, maxFolders, kRecentFoldersStr,
			kRecentFolders, kRecentFoldersCount, &fRecentFoldersBtn, &fRecentFoldersFld,
			fBeMenuPaneBG);
	} else {
		fRecentFoldersBtn = NULL;
		fRecentFoldersFld = NULL;
	}

	//	will place this items left edge relative to the contents list
	fGroupMenu = new BPopUpMenu("Show Group");	
	width = be_plain_font->StringWidth("Some Long Name For a Group")
		+ kMenuFieldPad;
	frame.Set(0, 10, width, 11);
	fGroupBtn = new BMenuField(frame, "group btn", kShowGroupStr, fGroupMenu);
	fBeMenuPaneBG->AddChild(fGroupBtn);
	fGroupBtn->HidePopUpMarker();
	fGroupBtn->SetDivider(0);
	
#ifdef IS_SORTABLE
	//	not sortable now, ever (?)
	fSortMenu = new BPopUpMenu("Sort By");
	fSortMenu->AddItem(new BMenuItem("Sort One", NULL));
	fSortMenu->AddItem(new BMenuItem("Sort Two", NULL));
	
	frame.OffsetBy(0, fGroupBtn->Frame().Height() + kMenuFieldGap);
	fSortBtn = new BMenuField(frame, "sort btn", kSortByStr, fSortMenu);
	fBeMenuPaneBG->AddChild(fSortBtn);
	fSortBtn->SetDivider(be_plain_font->StringWidth(kSortByStr) + 5);	
#endif

	//	Contents List
	//	placement is relative to controls in place
	//	may or may not have recents items
	//	always will have New Group button
#ifdef IS_SORTABLE
	frame.top = fSortBtn->Frame().bottom + 10;
#else
	frame.top = fGroupBtn->Frame().bottom + 10;
#endif
	frame.bottom = frame.top + 1;
	if (maxApps > -1 && fRecentAppsBtn)
		frame.left = fRecentAppsBtn->Frame().right + 10;
	else if (maxDocs > -1 && fRecentDocsBtn)
		frame.left = fRecentDocsBtn->Frame().right + 10;
	else if (maxFolders > -1 && fRecentFoldersBtn)
		frame.left = fRecentFoldersBtn->Frame().right + 10;		
	else
		frame.left = fGroupBtn->Frame().right + 10;
		
	frame.right = frame.left + 1;
	fMenuThing = new TContentsMenu(frame, new BMessage(kItemSelected),
		new BMessage(kDoubleClick), 10, &fCurrentRef);
	fBeMenuPaneBG->AddChild(fMenuThing);

	width = be_plain_font->StringWidth("Edit"B_UTF8_ELLIPSIS);
	frame.Set(0, 0, (kMinBtnWidth >= width) ? kMinBtnWidth : width, 1);
	fEditBtn = new BButton(frame, "edit", "Edit"B_UTF8_ELLIPSIS, new BMessage(kEditItem),
		B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE);
	fBeMenuPaneBG->AddChild(fEditBtn);

	width = be_plain_font->StringWidth(kOpenStr);
	frame.Set(0, 0, (kMinBtnWidth >= width) ? kMinBtnWidth : width, 1);
	fOpenBtn = new BButton(frame, "open", kOpenStr, new BMessage(kOpenItem),
		B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE);
	fBeMenuPaneBG->AddChild(fOpenBtn);

	width = be_plain_font->StringWidth("Add"B_UTF8_ELLIPSIS);
	frame.Set(0, 0, (kMinBtnWidth >= width) ? kMinBtnWidth : width, 1);
	fAddBtn = new BButton(frame, "add", "Add"B_UTF8_ELLIPSIS, new BMessage(kAdd),
		B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE);		
	fBeMenuPaneBG->AddChild(fAddBtn);

	width = be_plain_font->StringWidth(kRemoveStr);
	frame.Set(0, 0, (kMinBtnWidth >= width) ? kMinBtnWidth : width, 1);
	fRemoveBtn = new BButton(frame, "remove", kRemoveStr, new BMessage(kRemove),
		B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE);
	fBeMenuPaneBG->AddChild(fRemoveBtn);

	// initially disable buttons
	fRemoveBtn->SetEnabled(false);
	fOpenBtn->SetEnabled(false);
	fEditBtn->SetEnabled(false);

	// contents list will auto resize
	// resizing of parent pane will cause
	// contents list and buttons to flow
	fOpenBtn->MoveTo(fMenuThing->Frame().right - fOpenBtn->Frame().Width(),
		fMenuThing->Frame().bottom + 10);
	fEditBtn->MoveTo(fOpenBtn->Frame().left - 10 - fEditBtn->Frame().Width(),
		fOpenBtn->Frame().top);

	fRemoveBtn->MoveTo(fOpenBtn->Frame().left, fOpenBtn->Frame().bottom + 5);		
	fAddBtn->MoveTo(fEditBtn->Frame().left, fRemoveBtn->Frame().top);		

	// resize the pane so that everything is visible
	fBeMenuPaneBG->ResizeTo(fMenuThing->Frame().right + 10,
		fMenuThing->Frame().bottom + 10 + fOpenBtn->Frame().Height() + 5
			+ fRemoveBtn->Frame().Height() + 10);	

	//	place the remaining controls relative to the menu thing
	float bottom;
	float left = 10;
	BRect menuthingframe = fMenuThing->Frame();
	if (maxFolders > -1 && fRecentFoldersFld && fRecentFoldersBtn) {
		bottom = menuthingframe.bottom;
		fRecentFoldersFld->MoveTo(
			menuthingframe.left - fRecentFoldersFld->Frame().Width() - 10,
			bottom - fRecentFoldersFld->Frame().Height());
		fRecentFoldersBtn->MoveTo(10,
			fRecentFoldersFld->Frame().top - fRecentFoldersBtn->Frame().Height());
			
		left = fRecentFoldersBtn->Frame().right
			- (fRecentFoldersBtn->Frame().Width() / 2) - (fNewGroupBtn->Frame().Width() / 2);
	}
	
	if (maxApps > -1 && fRecentAppsFld && fRecentAppsBtn) {
		if (maxFolders > -1 && fRecentFoldersFld && fRecentFoldersBtn)
			bottom = fRecentFoldersBtn->Frame().top - 20;
		else {
			bottom = menuthingframe.bottom;			
			left = fRecentAppsBtn->Frame().right
				- (fRecentAppsBtn->Frame().Width() / 2) - (fNewGroupBtn->Frame().Width() / 2);
		}
  
		fRecentAppsFld->MoveTo(
			menuthingframe.left - fRecentAppsFld->Frame().Width() - 10,
			bottom - fRecentAppsFld->Frame().Height());
		fRecentAppsBtn->MoveTo(10,
			fRecentAppsFld->Frame().top - fRecentAppsBtn->Frame().Height());
	}

	if (maxDocs > -1 && fRecentDocsFld && fRecentDocsBtn) {
		if (maxApps > -1 && fRecentAppsFld && fRecentAppsBtn)
			bottom = fRecentAppsBtn->Frame().top - 20;
		else if (maxFolders > -1 && fRecentFoldersFld && fRecentFoldersBtn)
			bottom = fRecentFoldersBtn->Frame().top - 20;
		else {
			bottom = menuthingframe.bottom;			
			left = fRecentDocsBtn->Frame().right
				- (fRecentDocsBtn->Frame().Width() / 2) - (fNewGroupBtn->Frame().Width() / 2);
		}
		
		fRecentDocsFld->MoveTo(
			menuthingframe.left - fRecentDocsFld->Frame().Width() - 10,
			bottom - fRecentDocsFld->Frame().Height());
		fRecentDocsBtn->MoveTo(10,
			fRecentDocsFld->Frame().top - fRecentDocsBtn->Frame().Height());
	}

	//	place this button relative (centered) to
	//	any recents items and content list
	fNewGroupBtn->MoveTo(left, menuthingframe.top);
		
#ifdef IS_SORTABLE
	fSortBtn->MoveTo(menuthingframe.left - fSortBtn->Divider(),
		fSortBtn->Frame().top);
#endif
	fGroupBtn->MoveTo(menuthingframe.left - fGroupBtn->Divider(),
		fGroupBtn->Frame().top);
}


static void
GetNextGroupName(BDirectory *dir, char *directoryName)
{
	int32 index = 1;
	for (;;) {
		sprintf(directoryName, "Untitled Group %li", index);
		if (!dir->Contains(directoryName))
			 break;
		else
			index++;
	}
}


void
TFavoritesConfigWindow::AddNewGroup(entry_ref *dirRef, entry_ref *newGroup)
{
	BDirectory dir(dirRef);
	
	char directoryName[B_FILE_NAME_LENGTH];
	GetNextGroupName(&dir, directoryName);
		
	BDirectory subdir;
	dir.CreateDirectory(directoryName, &subdir);

	BEntry entry;
	subdir.GetEntry(&entry);
	entry.GetRef(newGroup);
}


void
TFavoritesConfigWindow::AddSymLink(const entry_ref *dirRef, const entry_ref *target)
{
	BDirectory dir(dirRef);
	BEntry entry(target);
	BPath path;
	entry.GetPath(&path);
	BSymLink symlink;
	dir.CreateSymLink(target->name, path.Path(), &symlink);
}


void
TFavoritesConfigWindow::AddNewGroup()
{
	//	from New Group/Folder button
	//	add a new group/folder to this directory
	entry_ref newGroup;
	AddNewGroup(&fCurrentRef, &newGroup);
}


void
TFavoritesConfigWindow::PromptForAdd()
{
	if (fAddPanel)
		fAddPanel->Show();
			//	does an activate on the window
	else {
		//  determine a starting point for where apps are added from
		char appPath[B_PATH_NAME_LENGTH];
		
		//  search for an apps directory
		if (find_directory(B_APPS_DIRECTORY, 0, false, appPath, B_PATH_NAME_LENGTH) == B_OK) {
			entry_ref ref;

			// get reference to application directory
		    get_ref_for_path(appPath, &ref);

			fAddPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this, this),
				&ref, fFilePanelNodeFlavors, true);
		} else
			fAddPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this, this),
				NULL, fFilePanelNodeFlavors, true);
		
		fAddPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Add");
		fAddPanel->Show();
	}
}


void
TFavoritesConfigWindow::AddRefs(BMessage *message)
{
	//	add any refs in the message to the current directory
	int32 index = 0;
	entry_ref ref;
	while (message->FindRef("refs", index, &ref) == B_OK) {
		AddSymLink(&fCurrentRef, &ref);
		index++;
	}
}


//	open a new ref to show, tunnelling down

void
TFavoritesConfigWindow::OpenGroup(const entry_ref *ref)
{
	if (!ref)
		return;
		
	fCurrentRef = *ref;
	fMenuThing->SetStartRef(&fCurrentRef);
	fMenuThing->SetValue(0);
	
	BMessage *message = new BMessage(kShowGroup);
	message->AddRef("current", &fCurrentRef);
	
	ModelMenuItem *item = new ModelMenuItem(new Model(&fCurrentRef), fCurrentRef.name, message);
	fGroupMenu->AddItem(item);
	item->SetMarked(true);
	
	UpdateButtons();
}


//	specify a new ref to show, tunnelling up

void
TFavoritesConfigWindow::ShowGroup(const entry_ref *groupRef)
{
	if (!groupRef)
		return;
	
	entry_ref lastRef = fCurrentRef;
		
	fCurrentRef = *groupRef;
	fMenuThing->SetStartRef(&fCurrentRef);
	fMenuThing->Select(&lastRef);

	//	find the item that was selected
	int32 count = fGroupMenu->CountItems() - 1;
	for (int32 index = count ; index >= 0 ; index--) {
		BMenuItem *item = fGroupMenu->ItemAt(index);
		if (item) {
			BMessage *message = item->Message();
			if (message) {
				entry_ref ref;
				if (message->FindRef("current", &ref) == B_OK) {

					//	if we have a match
					//	delete all the items below this item
					if (ref == fCurrentRef) {
						for (int32 j = count ; j > index ; j--) 
							delete fGroupMenu->RemoveItem(j);

						item->SetMarked(true);
						break;						
					}
				}
			}
		}
	}
	
	UpdateButtons();
}


void
TFavoritesConfigWindow::UpdateButtons()
{
	//	Open is enabled only if this item is an actual directory
	//	Edit is enabled for any selection
	//	Remove is selected only if there is a selection
	//	Add is always enabled
	int32 selection = fMenuThing->Value();
	
	if (selection >= 0 && selection < fMenuThing->ItemCount())
		fOpenBtn->SetEnabled(fMenuThing->ItemAt(selection)->IsDirectory());
	else
		fOpenBtn->SetEnabled(false);

	fEditBtn->SetEnabled(selection >= 0);
	fRemoveBtn->SetEnabled(selection >= 0);
}


void
TFavoritesConfigWindow::UpdateFoldersCount(int32 count, bool notifyTracker)
{
	if (count == -1)
		count = atoi(fRecentFoldersFld->Text());

	BMessage message(kUpdateFolderCount);
	message.AddInt32("count", count);
	fParent.SendMessage(&message);
	
	if (notifyTracker) {
		BMessenger tracker(kTrackerSignature);
		BMessage notificationMessage(kFavoriteCountChangedExternally);
		notificationMessage.AddInt32("RecentFolders", count);
		tracker.SendMessage(&notificationMessage);
	}
}


void
TFavoritesConfigWindow::UpdateDocsCount(int32 count, bool notifyTracker)
{
	if (count == -1)
		count = atoi(fRecentDocsFld->Text());

	BMessage message(kUpdateDocsCount);
	message.AddInt32("count", count);
	fParent.SendMessage(&message);

	if (notifyTracker) {
		BMessenger tracker(kTrackerSignature);
		BMessage notificationMessage(kFavoriteCountChangedExternally);
		notificationMessage.AddInt32("RecentDocuments", count);
		tracker.SendMessage(&notificationMessage);
	}
}


void
TFavoritesConfigWindow::UpdateAppsCount(int32 count, bool notifyTracker)
{
	if (count == -1)
		count = atoi(fRecentAppsFld->Text());

	BMessage message(kUpdateAppsCount);
	message.AddInt32("count", count);
	fParent.SendMessage(&message);

	if (notifyTracker) {
		BMessenger tracker(kTrackerSignature);
		BMessage notificationMessage(kFavoriteCountChangedExternally);
		notificationMessage.AddInt32("RecentApplications", count);
		tracker.SendMessage(&notificationMessage);
	}
}


// ***************************************************************************************


TDraggableIconButton::TDraggableIconButton(BRect frame, const char *label,
	BMessage *message, uint32 resizeMask, uint32 flags)
	:	BControl(frame, "draggable icon", label, message, resizeMask, flags),
		fIcon(NULL)
{
}


TDraggableIconButton::~TDraggableIconButton()
{
}


void 
TDraggableIconButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fIcon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
	fIcon->SetBits(kLargeNewGroupIcon, fIcon->BitsLength(), 0, B_CMAP8);
	//	calculate correct frame for icon and label
	//	sets icon rect and label rect for drawing
	ResizeToPreferred();	
}


void
TDraggableIconButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
	delete fIcon;
	fIcon = NULL;
}


void 
TDraggableIconButton::Draw(BRect)
{
	PushState();
	
	SetHighColor(kBlack);
	SetLowColor(ViewColor());
	
	if (fIcon) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(fIcon, fIconRect);
	}
	
	SetDrawingMode(B_OP_COPY);
	MovePenTo(fLabelRect.LeftBottom());
	DrawString(Label());
	
	if (IsEnabled() && IsFocus() && Window()->IsActive())
		SetHighColor(kBlack);
	else
		SetHighColor(ViewColor());
	StrokeRect(Bounds());
	
	PopState();
}


void 
TDraggableIconButton::MouseDown(BPoint where)
{
	ulong buttons;
	BPoint loc;
	GetMouse(&loc, &buttons);
	if (!buttons)
		return;

	fInitialClickRect.Set(where.x, where.y, where.x, where.y);
	fInitialClickRect.InsetBy(-4, -4);
	SetTracking(true);
	fDragging = false;
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
	InvertRect(fIconRect);
}


void 
TDraggableIconButton::MouseUp(BPoint where)
{
	if (IsTracking()) {
		InvertRect(fIconRect);
		SetTracking(false);
		fDragging = false;
		if (Bounds().Contains(where) && fInitialClickRect.Contains(where)) 
			//	tell parent to add a new group
			Invoke();

	} else
		BControl::MouseUp(where);
}


void 
TDraggableIconButton::MouseMoved(BPoint where, uint32 code,
	const BMessage *message)
{
	if (IsTracking()) {
		if (!fDragging && fIcon) {
			DragMessage(new BMessage('icon'), new BBitmap(fIcon),
				B_OP_BLEND, BPoint(15, 15));
			fDragging = true;
			fInitialClickRect.Set(0, 0, 0, 0);
		}
	} else
		BControl::MouseMoved(where, code, message);
}


void 
TDraggableIconButton::GetPreferredSize(float *width, float *height)
{
	float stringWidth = be_plain_font->StringWidth(Label());
	float fontHeight = FontHeight(be_plain_font, true);
	*width = stringWidth + 10;
	*height = 32 + fontHeight + 5;
	
	fIconRect.Set((*width / 2) - 16, 2, (*width / 2) + 15, 33);
	fLabelRect.Set((*width / 2) - (stringWidth / 2),
		*height - fontHeight - 5, *width, *height - 5);
}


void 
TDraggableIconButton::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}


const float kLeftGutter = 15.0f;
const float kHorizontalGap = 4.0f;
const float kItemGap = 4.0f;
const float kVerticalBorderSize = 2.0f;
const float kHorizontalBorderSize = 2.0f;
const int32 kMaxItemCount = 12;
const float kScrollerHeight = 16.0f;


TContentsMenu::TContentsMenu(BRect frame, BMessage *singleClick, BMessage *doubleClick,
	int32 visibleItemCount, const entry_ref *startRef)
	:	BControl(frame, "contents menu", "contents menu label",
			singleClick, B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE),
	fDoubleClickMessage(doubleClick),
	fVisibleItemCount(visibleItemCount),
	fStartRef(*startRef),
	fItemHeight(16),
#ifdef ITEM_EDIT
	fEditingItem(false),
	fEditingFld(NULL),
#endif
	fFirstItem(0),
	fContentsList(NULL),
	fUpBtn(NULL),
	fDownBtn(NULL)
{
}


TContentsMenu::~TContentsMenu()
{
	delete fDoubleClickMessage;
}


void 
TContentsMenu::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	menu_info minfo;
	get_menu_info(&minfo);

	//	cache the menu font for drawing later on
#ifdef USE_MENU_FONT
	//	!! this should change when the menu font is used
	fMenuFont = new BFont();
	fMenuFont->SetFamilyAndStyle(minfo.f_family, minfo.f_style);
	fMenuFont->SetSize(minfo.font_size);
#else
	fMenuFont = new BFont(be_plain_font);
#endif
	//	cache the item height, greater of font height and icon height
	fFontHeight = FontHeight(fMenuFont, true);
	fItemHeight = (fFontHeight>fItemHeight) ? fFontHeight : fItemHeight;
	fItemHeight += 3;	// add a gutter

	//	mimicing a menu
	SetViewColor(minfo.background_color);
#ifdef SNAKE
	fHiliteColor = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
#else
	fHiliteColor = tint_color(minfo.background_color, B_DARKEN_2_TINT);
#endif	
	//	add the buttons
	//	cache item frame, sub of frame - buttons
	fContentsList = new BObjectList<Model>(kMaxItemCount);
	
	SetValue(-1);		//	no item selected
	
	ResizeToPreferred();
	
	BRect frame(2, 2, Bounds().Width() - 2, 2 + kScrollerHeight);
	fUpBtn = new TScrollerButton(frame, new BMessage(kScrollDown), true);
	AddChild(fUpBtn);
	fUpBtn->SetTarget(this, Window());
	
	frame.bottom = Bounds().Height()-2;
	frame.top = frame.bottom - kScrollerHeight;
	fDownBtn = new TScrollerButton(frame, new BMessage(kScrollUp), false);
	AddChild(fDownBtn);
	fDownBtn->SetTarget(this, Window());

	//	cache the new group icon
	fSmallGroupIcon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
	fSmallGroupIcon->SetBits(kSmallNewGroupIcon, fSmallGroupIcon->BitsLength(),
		0, B_CMAP8);

	//	cache the symlink icon
	BMimeType symlink("application/x-vnd.Be-symlink");
	if (symlink.InitCheck() == B_OK) {
		fSymlinkIcon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
		if (symlink.GetIcon(fSymlinkIcon, B_MINI_ICON) != B_OK)
			fSymlinkIcon = NULL;		
	} else
		fSymlinkIcon = NULL;

	//	set up node watch etc
	SetStartRef(&fStartRef);
}


void
TContentsMenu::DetachedFromWindow()
{
	EmptyMenu();
	delete fContentsList;
	fContentsList = NULL;
	delete fMenuFont;
	fMenuFont = NULL;
	delete fSmallGroupIcon;
	fSmallGroupIcon = NULL;
	delete fSymlinkIcon;
	fSymlinkIcon = NULL;
	BControl::DetachedFromWindow();
}


const char *kUntitledItemStr = "<Untitled Item>";

void 
TContentsMenu::Draw(BRect updateRect)
{
	PushState();
	
	//	draw frame
	SetLowColor(ViewColor());
	
	BRect frame(Bounds());
	//	main border
		
#ifdef SNAKE
	SetHighColor(kBlack);
	StrokeRoundRect(frame, 3.0, 3.0);
#else
	//	exterior border
	SetHighColor(100, 100, 100);
	StrokeRect(frame);

	//	interior white	
	SetHighColor(233, 233, 233);
	//	top
	StrokeLine(frame.LeftTop() + BPoint(1, 1), frame.RightTop() + BPoint(-1, 1));
	// 	left
	StrokeLine(frame.LeftTop() + BPoint(1, 1), frame.LeftBottom() + BPoint(1, -1));

	//	interior gray
	SetHighColor(141, 141, 141);
	//	right
	StrokeLine(frame.RightTop() + BPoint(-1, -1), frame.RightBottom() + BPoint(-1, -1));
	// 	bottom
	StrokeLine(frame.LeftBottom() + BPoint(1, -1), frame.RightBottom() + BPoint(-1, -1));
	
#endif
	
	SetDrawingMode(B_OP_OVER);
	
	BRect iconFrame, textFrame, itemFrame;
	
	int32 max = fContentsList->CountItems() - fFirstItem;
	int32 count = count = (max > kMaxItemCount) ? kMaxItemCount : max;
	count += fFirstItem;

	for (int32 index = fFirstItem ; index < count ; index++) {
		Model *item = fContentsList->ItemAt(index);
		if (!item)
			continue;
		
		if (!ItemFrame(index - fFirstItem, &iconFrame, &textFrame, &itemFrame))
			continue;
		
		if (!itemFrame.Intersects(updateRect))
			continue;

		//	see if an item is selected
		if (Value() == index) {			
			SetHighColor(fHiliteColor);			
			SetLowColor(fHiliteColor);
#ifdef SNAKE
			FillRoundRect(itemFrame, 3.0, 3.0);

			// 	main frame
			SetHighColor(kBlack);
			StrokeRoundRect(itemFrame, 3.0, 3.0);
#else
			FillRect(itemFrame);
#endif
		} else {
			SetHighColor(ViewColor());
			SetLowColor(ViewColor());
			FillRect(itemFrame);
		}
				
		Model resolvedItem;
		if (item->IsSymLink()) {
			//	if this item is a symlink
			//	see if it points to anything
			//	if it doesn't draw the broken symlink icon
			BEntry entry(item->EntryRef(), true);
			resolvedItem.SetTo(&entry);
			if (entry.Exists()) {
				//	draw the real item icon
				IconCache::sIconCache->Draw(&resolvedItem, this, iconFrame.LeftTop(),
					kNormalIcon, B_MINI_ICON);
			} else if (fSymlinkIcon){

				SetDrawingMode(B_OP_OVER);
				DrawBitmapAsync(fSymlinkIcon, iconFrame);
				//	this item doesn't exist
				//	get the name that the symlink has
				resolvedItem.SetTo(item->EntryRef());
			} else {
				//	shouldn't ever get here
				TRESPASS();
				entry_ref ref;
				ref.set_name("");
				resolvedItem.SetTo(&ref);
			}	
		} else {
			if (item->IsDirectory() && fSmallGroupIcon) {
				SetDrawingMode(B_OP_OVER);
				DrawBitmapAsync(fSmallGroupIcon, iconFrame);
			} else
				IconCache::sIconCache->Draw(item, this, iconFrame.LeftTop(),
					kNormalIcon, B_MINI_ICON);
					
			resolvedItem.SetTo(item->EntryRef());
		}

		//	get the name of the resolved ref
		//	if this item points to /boot
		//	will retrieve the user name instead
		const char *name = NULL;
		if (resolvedItem.IsVolume()) {
			BVolume volume(resolvedItem.NodeRef()->device);
			char volumeName[B_FILE_NAME_LENGTH];
			volume.GetName(volumeName);

			name = volumeName;
		} else if (item->IsSymLink()) {
			//	use the items symlink name
			name = item->EntryRef()->name;			
		} else if (resolvedItem.Name() && resolvedItem.Name()[0] != '\0')
			//	use the items actual name
			name = resolvedItem.Name();
		else
			name = kUntitledItemStr;

		//	truncate to fit appropriately
		BString truncatedString(name);
		fMenuFont->TruncateString(&truncatedString, B_TRUNCATE_END, textFrame.Width());

		SetHighColor(kBlack);
		SetFont(fMenuFont);
		MovePenTo(textFrame.LeftBottom());
		DrawString(truncatedString.String());
	}
	
	PopState();
}


void
TContentsMenu::InvalidateItem(int32 index)
{
	BRect dummy, itemrect;
	if (ItemFrame(index, &dummy, &dummy, &itemrect))
		Invalidate(itemrect);
}


void
TContentsMenu::InvalidateAbsoluteItem(int32 index)
{
	index -= fFirstItem;
	if (index >= 0 && index < kMaxItemCount)
		InvalidateItem(index);
}


void 
TContentsMenu::KeyDown(const char *bytes, int32 numBytes)
{
	if (IsEnabled() && IsFocus() && Window()->IsActive()) {
		switch (bytes[0]) {
			case B_DOWN_ARROW:
			case B_LEFT_ARROW:
				{
#ifdef ITEM_EDIT
					StopItemEdit();
#endif
					int32 selection = Value() + 1;
					if (selection < fContentsList->CountItems()) {
						SetValueNoUpdate(selection);
						InvalidateAbsoluteItem(selection - 1);
						InvalidateAbsoluteItem(selection);
						Invoke();
					}
					//
					//	if the selection is the last item
					//	scroll the list down
					//	bottom item will be the selection
					//
					if (Value() - fFirstItem == kMaxItemCount)
						Scroll(true);
				}
				break;
			case B_UP_ARROW:
			case B_RIGHT_ARROW:
				{
#ifdef ITEM_EDIT
					StopItemEdit();
#endif
					int32 selection = Value()-1;
					if (selection >= 0) {
						SetValueNoUpdate(selection);
						InvalidateAbsoluteItem(selection + 1);
						InvalidateAbsoluteItem(selection);
						Invoke();
					}

					if (Value() - fFirstItem < 0)
						Scroll(false);
				}
				break;
			case B_ENTER:
			case B_SPACE:
				OpenItem(Value());
				break;
			default:
				BControl::KeyDown(bytes, numBytes);
				break;
		}
	}
}


void 
TContentsMenu::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		if (message->what == 'icon') {
			BPoint where;
			if (message->FindPoint("_drop_point_", &where) != B_OK) {
				where.x = -1;
				where.y = -1;
			}
			AddTempItem(where);
		} else {
			TFavoritesConfigWindow *window = dynamic_cast<TFavoritesConfigWindow*>(Window());
			if (window)
				window->AddRefs(message);
		}
	}
	
	switch (message->what) {
		//	node monitor of be menu directory
		case B_NODE_MONITOR:
			{
				//	stash the entry_ref to the selected item
				//	use it after FillMenu to reselect the item
				const entry_ref *oldref = ItemAt(Value())->EntryRef();
				entry_ref ref;
				if (!oldref)
					ref.set_name("");
				else
					ref = *oldref;

				EmptyMenu();
				FillMenu(&fStartRef);
				Select(&ref);
					// checks for invalid ref
				Invalidate();			

				// 	ask the window to update the buttons
				Window()->PostMessage(kItemSelected);
			}
			break;
			
		//	down button
		case kScrollUp:
#ifdef ITEM_EDIT
			StopItemEdit();
#endif
			Scroll(true);
			break;

		//	up button
		case kScrollDown:
#ifdef ITEM_EDIT
			StopItemEdit();
#endif
			Scroll(false);
			break;
			
		default:
			BControl::MessageReceived(message);
			break;
	}	
}


void 
TContentsMenu::MouseDown(BPoint where)
{
#ifdef ITEM_EDIT
	StopItemEdit();
#endif
			
	bigtime_t clicktime;
	get_click_speed(&clicktime);
	
	bigtime_t diff = system_time() - fInitialClickTime;
	if (diff < clicktime && fInitialClickRect.Contains(where)) {
		OpenItem(Value());
		return;
	}

	fInitialClickRect.Set(where.x, where.y, where.x, where.y);
	fInitialClickRect.InsetBy(-2, -2);
	fInitialClickTime = system_time();

	StartTracking(where);
	MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS);
}


void
TContentsMenu::StartTracking(BPoint where)
{
	SelectItemAt(where);
	SetTracking(true);
}


void
TContentsMenu::StopTracking()
{
	if (IsTracking())
		SetTracking(false);		
}


void 
TContentsMenu::MouseUp(BPoint where)
{
	StopTracking();
#ifdef ITEM_EDIT
	BeginItemEdit(where);
#endif	
	BControl::MouseUp(where);
}


#ifdef ITEM_EDIT
void
TContentsMenu::BeginItemEdit(BPoint where)
{
	bigtime_t clicktime;
	get_click_speed(&clicktime);
	
	bigtime_t diff = system_time() - fInitialClickTime;
	
	if (where == fInitialClickLoc && (diff > clicktime)) {
		BRect iconFrame, textFrame, itemFrame;

		//	!! 	get textFrame instead
		//		set viewcolor to menucolor
		fItemIndex = ItemAt(where, &iconFrame, &textFrame, &itemFrame) + fFirstItem;
		if (fItemIndex >= 0) {
			//	find the item
			//	add a textview
			fEditingItem = true;

			delete fEditingFld;
			
			BRect trect(textFrame);
			trect.OffsetTo(0, 0);
			fEditingFld = new BTextView(textFrame, "edit", trect, B_FOLLOW_NONE);
			AddChild(fEditingFld);
			fEditingFld->SetViewColor(fHiliteColor);			

			fEditingFld->SetText(ItemAt(fItemIndex)->EntryRef()->name);
			fEditingFld->SelectAll();
		}
	} else
		fEditingItem = false;
}
#endif


#ifdef ITEM_EDIT
void
TContentsMenu::StopItemEdit()
{
	if (fEditingItem && fEditingFld) {
		if (fItemIndex > -1) {
			const char *name = fEditingFld->Text();
			const entry_ref *ref = ItemAt(fItemIndex)->EntryRef();
			if (strcmp(ref->name, name) != 0) {							
				BEntry entry(ref);
				if (entry.InitCheck() == B_OK && entry.Exists())
					entry.Rename(name);
			}
		}
		
		// dispose the textview
		if (fEditingFld) {
			fEditingFld->RemoveSelf();
			delete fEditingFld;
			fEditingFld = NULL;
			fEditingItem = false;
			fItemIndex = -1;
		}	
	}
}
#endif


void 
TContentsMenu::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	switch (code) {
		case B_ENTERED_VIEW:

			//	if there is a message incoming
			//	then track with it for attempt at
			//	placement drop
			//	!! disabled for now
#if 0
			if (message) {
				SelectItemAt(where);
				SetTracking(true);
			}
#endif
			break;
			
		case B_INSIDE_VIEW:
			if (IsTracking()) 
				SelectItemAt(where);
			break;
			
		case B_EXITED_VIEW:
			break;
			
	}
	BControl::MouseMoved(where, code, message);
}


void 
TContentsMenu::GetPreferredSize(float *width, float *height)
{
	//	width should accomodate border, left gap, icon, separation, text, border
	//	height should acccomodate 14 itemheights + borders

	*width = kLeftGutter + B_MINI_ICON + kItemGap
		+ (fMenuFont->StringWidth("O") * 15)
		+ (2 * kVerticalBorderSize);

	*height = (2 * kScrollerHeight)				//	buttons
		+ (kMaxItemCount * fItemHeight)			//	items
		+ (2 * kHorizontalBorderSize) + 1;		//	border
}


void 
TContentsMenu::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}


void
TContentsMenu::SetStartRef(const entry_ref *ref)
{
	node_ref nref;
	BEntry entry(&fStartRef);
	if (entry.InitCheck() == B_OK) {
		entry.GetNodeRef(&nref);
		watch_node(&nref, B_STOP_WATCHING, this, Window());
	}
		
	fStartRef = *ref;
	entry.SetTo(&fStartRef);
	entry.GetNodeRef(&nref);
	watch_node(&nref, B_WATCH_DIRECTORY, this, Window());

	EmptyMenu();
	FillMenu(&fStartRef);
	Invalidate();
}


void
TContentsMenu::UpdateScrollers()
{
	int32 count = fContentsList->CountItems();
	if (count <= kMaxItemCount) {
		fUpBtn->SetEnabled(false);
		fDownBtn->SetEnabled(false);
	} else {
		fUpBtn->SetEnabled(fFirstItem != 0);
		fDownBtn->SetEnabled(count-fFirstItem > kMaxItemCount);
	}
}


void
TContentsMenu::Scroll(bool direction)
{
	int32 count = fContentsList->CountItems();
	if (count <= kMaxItemCount)
		return;
		
	bool needtoupdate = false;
	if (direction && fDownBtn->IsEnabled()) {			//	down 
		if ((count - fFirstItem) > kMaxItemCount) {
			fFirstItem++;
			needtoupdate = true;
		}
	} else if (!direction && fUpBtn->IsEnabled()) {		// up
		if (fFirstItem >= 1) {
			fFirstItem--;
			needtoupdate = true;
		}
	}

	if (needtoupdate) {
		UpdateScrollers();
		BRect dummy, highrect;
		ItemFrame(0, &dummy, &dummy, &highrect);
		highrect.bottom = highrect.bottom + fItemHeight * (kMaxItemCount - 2);
		BRect lowrect = highrect.OffsetByCopy(0, fItemHeight);
		if (direction) {
			CopyBits(lowrect, highrect);
			InvalidateItem(kMaxItemCount - 1);
		} else {
			CopyBits(highrect, lowrect);
			InvalidateItem(0);
		}
	}
}


static int
CompareOne(const Model *model1, const Model *model2)
{
	return strcasecmp(model1->Name(), model2->Name());
}


void
TContentsMenu::FillMenu(const entry_ref *ref)
{
	if (!fContentsList)
		fContentsList = new BObjectList<Model>(kMaxItemCount);
		
	BEntry entry(ref);
	if (entry.InitCheck() == B_OK && entry.Exists()) {
		BDirectory dir(ref);
		BEntry nextEntry;
		while (dir.GetNextEntry(&nextEntry) == B_OK) {

			//	create a model for the actual item in the be folder
			Model *model = new Model(&nextEntry);
			fContentsList->AddItem(model);
		}

		fContentsList->SortItems(CompareOne);
	}
	
	int32 count = fContentsList->CountItems();
	if (count <= kMaxItemCount)
		fFirstItem = 0;
	else if (count - fFirstItem < kMaxItemCount)
		fFirstItem = count - kMaxItemCount;
		
	UpdateScrollers();
}


void
TContentsMenu::EmptyMenu()
{
	if (!fContentsList)
		return;
		
	int32 count = fContentsList->CountItems()-1;
	for (int32 index = count ; index >= 0 ; index--) {
		Model *item = fContentsList->ItemAt(index);
		if (item) {
			fContentsList->RemoveItem(item);
			delete item;
		}
	}
}


/**	returns frames for visible items */

bool
TContentsMenu::ItemFrame(int32 index, BRect *iconFrame, BRect *textFrame,
	BRect *itemFrame) const
{
	if (index >= kMaxItemCount || index < 0)
		return false;
	
	float halfheight = fItemHeight / 2;
	float halficon = B_MINI_ICON / 2;
	float halffont = fFontHeight / 2;
	BPoint loc;
	loc.x = kLeftGutter;
	loc.y = fUpBtn->Frame().bottom + 1 + halfheight;
	
	loc.y += index * fItemHeight;

	iconFrame->Set(loc.x, loc.y - halficon, loc.x + B_MINI_ICON, loc.y + halficon);

	textFrame->Set(loc.x + B_MINI_ICON + kItemGap, loc.y - halffont,
		Bounds().right - 2, loc.y + halffont);
		
	itemFrame->Set(2, loc.y - halfheight, Bounds().Width() - 2, loc.y + halfheight - 1);
	
	return true;
}


/**	returns index of visible item at location */

int32
TContentsMenu::ItemAt(BPoint where, BRect *iconFrame, BRect *textFrame,
	BRect *itemFrame)
{
	int32 count = fContentsList->CountItems();
	count = (count < kMaxItemCount) ? count : kMaxItemCount;	
	for (int32 index = 0 ; index < count ; index++)
		if (ItemFrame(index, iconFrame, textFrame, itemFrame)
			&& itemFrame->Contains(where)) 
			return index;
	
	return -1;
}


/**	returns entry_ref for item at absolute index */

const Model*
TContentsMenu::ItemAt(int32 index) const
{
	return fContentsList->ItemAt(index);
}


void
TContentsMenu::SelectItemAt(BPoint where)
{
	BRect iconFrame, textFrame, itemFrame;

	int32 previousvalue = Value();
	//	get visible item
	int32 index = ItemAt(where, &iconFrame, &textFrame, &itemFrame);
	if (index <= -1) {
		SetValueNoUpdate(-1);
		Invoke();
	} else {
		//	get offset to actual item selected
		index += fFirstItem;
		if (index != Value()) {
			SetValueNoUpdate(index);
			Invoke();
		}
	}
	int32 newvalue = Value();
	if (previousvalue != newvalue) {
		InvalidateAbsoluteItem(previousvalue);
		InvalidateAbsoluteItem(newvalue);
	}
}


void
TContentsMenu::Select(const entry_ref *ref)
{
	int32 select = -1;
	if (ref && ref->name && ref->name[0] != '\0') {
		int32 count = fContentsList->CountItems();
		for (int32 index = 0 ; index < count ; index++) {
			Model *item = fContentsList->ItemAt(index);
			if (item) {
				if (*ref == *(item->EntryRef())) {
					select = index;
					break;
				}
			}
		}
	}
	SetValue(select);
	Invoke();
}


void
TContentsMenu::OpenItem(int32 index)
{
	if (Value() >= 0 && index >= 0 && index < fContentsList->CountItems()) {
		Model *item = fContentsList->ItemAt(index);
		//	see if we have an item
		//	only actual directories can be traversed
		//	and only if their hierarchy lives in the Be folder

		if (item && item->IsDirectory()) {
			//	if we have a double click message
			//	pass the new directory ref to the window
			if (fDoubleClickMessage) {
				if (fDoubleClickMessage->HasRef("current"))
					fDoubleClickMessage->ReplaceRef("current", item->EntryRef());
				else
					fDoubleClickMessage->AddRef("current", item->EntryRef());
										
				Invoke(fDoubleClickMessage);
			}
		}
	}
}


int32
TContentsMenu::ItemCount() const
{
	return fContentsList->CountItems();
}


static void
RemoveEntries(const entry_ref *ref)
{
	//	should this delete on DB created items?

	BEntry entry(ref);
	if (entry.InitCheck() == B_OK && entry.Exists()) {
		//	if its a directory
		//	check for contents and delete as necessary
		if (entry.IsDirectory()) {
			BDirectory dir(&entry);
			BEntry nextEntry;
			while (dir.GetNextEntry(&nextEntry) == B_OK) {
				if (nextEntry.IsDirectory()) {
					entry_ref nextref;
					nextEntry.GetRef(&nextref);
					RemoveEntries(&nextref);
				} else
					nextEntry.Remove();
			}
		}
		entry.Remove();
	}
}


void
TContentsMenu::RemoveItem(int32 index)
{
	int32 count = ItemCount() - 1;

	//	index out of bounds
	if (index < 0 || index > count)
		return;	

	RemoveEntries(ItemAt(index)->EntryRef());
		
	//	index was last item
	if (index == count) {
		SetValue(index - 1);
		Invoke();
	}
}


void
TContentsMenu::AddTempItem(BPoint)
{
	//	!! need to make a fake item for editing
	//	defaults, now, to simply adding a folder
	TFavoritesConfigWindow *window = dynamic_cast<TFavoritesConfigWindow*>(Window());
	if (window)
		window->AddNewGroup();
}


// ***************************************************************************************
//	#pragma mark -


TScrollerButton::TScrollerButton(BRect frame, BMessage *message, bool direction)
	:	BControl(frame, "scroller", "scroller label", message,
			B_FOLLOW_NONE, B_WILL_DRAW),
		fDirection(direction),
		fTicker(NULL)
{
}


void 
TScrollerButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	menu_info minfo;
	get_menu_info(&minfo);
	fSelectedColor = tint_color(minfo.background_color, B_DARKEN_2_TINT);
	
	SetViewColor(minfo.background_color);

	fHiliteFrame = Bounds();
	if (fDirection) 
		fHiliteFrame.bottom -= 6;
	else 
		fHiliteFrame.top += 4;
}


void 
TScrollerButton::DetachedFromWindow()
{
	delete fTicker;
	BControl::DetachedFromWindow();
}


void 
TScrollerButton::Draw(BRect)
{
	
	PushState();
	
	if (Value() == 0 || !IsEnabled()) {
		SetLowColor(ViewColor());
		SetHighColor(ViewColor());
	} else {
		SetLowColor(fSelectedColor);
		SetHighColor(fSelectedColor);
	}

	FillRect(fHiliteFrame);

	//	add the triangle
	if (IsEnabled())
		SetHighColor(kMediumGray);
	else
		SetHighColor(tint_color(kMediumGray, B_LIGHTEN_1_TINT));
	
	float width = Bounds().Width();	
	int32 linecount = Bounds().IntegerHeight() - 9;

	BPoint start(width / 2, fDirection ? 2 : Bounds().Height()-2);
	BPoint finish(start);
	
	for (int32 index = 0 ; index < linecount ; index++) {
		StrokeLine(start, finish);
		--start.x;
		++finish.x;
		if (fDirection) {
			++start.y;
			++finish.y;
		} else {
			--start.y;
			--finish.y;
		}
	}
	
	//	add the top/bottom delimiter
	SetHighColor(kLightGray);
	float y = fDirection ? start.y + 2 : start.y - 3;
	StrokeLine(BPoint(0, y), BPoint(width, y));
	SetHighColor(kAlmostWhite);
	y = fDirection ? start.y + 3 : start.y - 2;
	StrokeLine(BPoint(0, y), BPoint(width, y));
	
	PopState();
}


void 
TScrollerButton::MouseDown(BPoint where)
{
	if (IsEnabled() && fHiliteFrame.Contains(where)) {
		SetValue(1);
		Invoke();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS);
		fTicker = new BMessageRunner(BMessenger(Target()), Message(), 120000);
	}
}


void 
TScrollerButton::MouseUp(BPoint)
{
	SetValue(0);
	delete fTicker;
	fTicker = NULL;
	if (IsTracking())
		SetTracking(false);
}


void 
TScrollerButton::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	switch (code) {
		case B_EXITED_VIEW:
			delete fTicker;
			fTicker = NULL;
			SetValue(0);
			break;

		case B_ENTERED_VIEW:
			if (IsEnabled() && IsTracking()) {
				SetValue(1);
				Invoke();
				fTicker = new BMessageRunner(BMessenger(Target()), Message(), 120000);
			}
			break;

		default:
			break;
	}
	BControl::MouseMoved(where, code, message);
}


// ***************************************************************************************

const char *kNewItemNameLabel = "New item name:";

const int32 kPanelWidth = 260; 
const int32 kPanelHeight = 77;

NameItemPanel::NameItemPanel(BWindow *parent, const char *initialtext)
	:	BWindow(BRect(0, 0, kPanelWidth, kPanelHeight), "", B_MODAL_WINDOW,  
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE),
		fParent(parent)
{
	MoveTo(-1024, -1024);
	Show();
	Lock();
	AddParts(initialtext);
	ResizeTo(Bounds().Width(), fCancelBtn->Frame().bottom + 10);
	Unlock();
	CenterWindowOnScreen(this);
} 


NameItemPanel::~NameItemPanel()
{
}


void 
NameItemPanel::MessageReceived(BMessage *message) 
{
	switch (message->what){
		case 'done':
			{
				const char *text = fNameFld->Text();
				if (!text || text[0] == '\0'){
					if ((new BAlert("", "The new name is empty, please enter a name", 
						"Cancel", "OK", NULL, B_WIDTH_AS_USUAL))->Go() == 0)
						return;
				}
				BMessage nameChangeMessage(kNameChange);
				nameChangeMessage.AddString("name", text);
				fParent->PostMessage(&nameChangeMessage);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
	
		case 'canc':
			fParent->PostMessage('canc');
			PostMessage(B_QUIT_REQUESTED);
			break;
	
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void 
NameItemPanel::AddParts(const char *initialtext) 
{
	fBG = new BBox(Bounds(), "bg", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER);
	AddChild(fBG);
	
	BRect rect(10, 10, Bounds().Width()-10, 11);
	fNameFld = new BTextControl(rect, "", kNewItemNameLabel, "", NULL);
	fBG->AddChild(fNameFld);
	fNameFld->SetDivider(be_plain_font->StringWidth(kNewItemNameLabel) + 10);
	fNameFld->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	if (initialtext && strlen(initialtext) > 0)
		fNameFld->SetText(initialtext);
	fNameFld->MakeFocus(true);

	BTextView *textView = fNameFld->TextView();
	if (textView)
		textView->SetMaxBytes(32);
	
	rect.right = Bounds().Width() - 10;
	rect.left = rect.right - 75;
	rect.top = fNameFld->Frame().bottom + 10;
	rect.bottom = rect.top + 1;
	fDoneBtn = new BButton(rect, "", "Change", new BMessage('done'),
		B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	
	rect.right = rect.left - 10;
	rect.left = rect.right - 75;
	fCancelBtn = new BButton(rect, "", "Cancel", new BMessage('canc'),
		B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fBG->AddChild(fCancelBtn);	

	fBG->AddChild(fDoneBtn);
	SetDefaultButton(fDoneBtn);
}

