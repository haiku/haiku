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
//	File Name:		DecView.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	decorator handler for the app
//  
//------------------------------------------------------------------------------
#ifndef DEC_VIEW_H_
#define DEC_VIEW_H_

#include <View.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include "ColorSet.h"
#include "LayerData.h"
class PreviewDriver;
class Decorator;

class DecView : public BView
{
public:
	DecView(BRect frame, const char *name, int32 resize, int32 flags);
	~DecView(void);
	void AllAttached(void);
	void MessageReceived(BMessage *msg);
	void SaveSettings(void);
	void LoadSettings(void);
	void NotifyServer(void);
	void GetDecorators(void);
	void SetColors(const ColorSet &set);
	bool LoadDecorator(const char *path);
	BString ConvertIndexToPath(int32 index);
protected:
	bool UnpackSettings(ColorSet *set, const BMessage &msg);
	BButton *apply;
	BListView *declist;
	BMessage settings;
	BScrollView *scrollview;
	PreviewDriver *driver;
	BView *preview;
	Decorator *decorator;
	image_id decorator_id;
	BString decpath;
	LayerData ldata;
	uint64 pat_solid_high;
	ColorSet colorset;
};

#endif
