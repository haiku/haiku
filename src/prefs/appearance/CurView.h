//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		CurView.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	System cursor handler
//  
//------------------------------------------------------------------------------
#ifndef CUR_VIEW_H_
#define CUR_VIEW_H_

#include <View.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <FilePanel.h>
#include <Invoker.h>

class APRWindow;

class CurView : public BView
{
public:
	CurView(const BRect &frame, const char *name, int32 resize, int32 flags);
	~CurView(void);
	void AllAttached(void);
	void MessageReceived(BMessage *msg);
	void SaveSettings(void);
	void LoadSettings(void);
	void SetDefaults(void);
	void NotifyServer(void);
	rgb_color GetColorFromMessage(BMessage *msg, const char *name, int32 index=0);
protected:
	friend APRWindow;
	BMenu *LoadCursorSets(void);
	void SaveCursorSet(const BString &name);
	void LoadCursorSet(const BString &name);
	void SetCursorSetName(const char *name);

	BButton *apply,*revert,*defaults;
	BListView *attrlist;
	color_which attribute;
	BMessage settings;
	BString attrstring;
	BScrollView *scrollview;
	BStringView *cursorset_label;
	BMenu *cursorset_menu,*settings_menu;
	BFilePanel *savepanel;
	BString cursorset_name;
	BString prev_set_name;
};

#endif
