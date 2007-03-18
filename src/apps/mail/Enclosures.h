/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

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

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//--------------------------------------------------------------------
//	
//	Enclosures.h
//
//--------------------------------------------------------------------

#ifndef _ENCLOSURES_H
#define _ENCLOSURES_H

#include <Box.h>
#include <File.h>
#include <ListView.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <ScrollView.h>
#include <View.h>
#include <Volume.h>

#include <MailMessage.h>

#define ENCLOSURES_HEIGHT	 65

#define ENCLOSE_TEXT		"Enclosures:"
#define ENCLOSE_TEXT_H		 7
#define ENCLOSE_TEXT_V		 3
#define ENCLOSE_FIELD_V		 3

class TListView;
class TMailWindow;
class TScrollView;


//====================================================================

class TEnclosuresView : public BView
{
	public:
		TEnclosuresView(BRect, BRect);
		~TEnclosuresView();

		virtual	void Draw(BRect);
		virtual void MessageReceived(BMessage*);
		void Focus(bool);
		void AddEnclosuresFromMail(BEmailMessage *mail);

		TListView *fList;

	private:
		bool fFocus;
		float fOffset;
		TMailWindow *fWindow;
};


//====================================================================

class TListView : public BListView
{
	public:
		TListView(BRect, TEnclosuresView *);

		virtual	void AttachedToWindow();
		virtual void MakeFocus(bool);
		virtual void MouseDown(BPoint point);
		virtual void KeyDown(const char *bytes,int32 numBytes);

	private:
		TEnclosuresView *fParent;
};


//====================================================================

class TListItem : public BListItem
{
	public:
		TListItem(entry_ref *);
		TListItem(BMailComponent *);

		virtual void DrawItem(BView *, BRect, bool);
		virtual	void Update(BView *, const BFont *);

		BMailComponent *Component() { return fComponent; };
		entry_ref	*Ref() { return &fRef; }
		node_ref	*NodeRef() { return &fNodeRef; }

	private:
		BMailComponent	*fComponent;
		entry_ref	fRef;
		node_ref	fNodeRef;
};

#endif // #ifndef _ENCLOSURES_H
