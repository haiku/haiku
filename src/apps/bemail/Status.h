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
//	Status.h
//	
//--------------------------------------------------------------------

#ifndef _STATUS_H
#define _STATUS_H

#include <Beep.h>
#include <Box.h>
#include <Button.h>
#include <fs_index.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#define	STATUS_WIDTH		220
#define STATUS_HEIGHT		80

#define STATUS_TEXT			"Status:"
#define STATUS_FIELD_H		 10
#define STATUS_FIELD_V		  8
#define STATUS_FIELD_WIDTH	(STATUS_WIDTH - STATUS_FIELD_H)
#define STATUS_FIELD_HEIGHT	 16

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define S_OK_BUTTON_X1		(STATUS_WIDTH - BUTTON_WIDTH - 6)
#define S_OK_BUTTON_Y1		(STATUS_HEIGHT - (BUTTON_HEIGHT + 10))
#define S_OK_BUTTON_X2		(S_OK_BUTTON_X1 + BUTTON_WIDTH)
#define S_OK_BUTTON_Y2		(S_OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define S_OK_BUTTON_TEXT	"OK"

#define S_CANCEL_BUTTON_X1	(S_OK_BUTTON_X1 - (BUTTON_WIDTH + 10))
#define S_CANCEL_BUTTON_Y1	S_OK_BUTTON_Y1
#define S_CANCEL_BUTTON_X2	(S_CANCEL_BUTTON_X1 + BUTTON_WIDTH)
#define S_CANCEL_BUTTON_Y2	S_OK_BUTTON_Y2
#define S_CANCEL_BUTTON_TEXT	"Cancel"

#define INDEX_STATUS		"_status"

enum status_messages {
	STATUS = 128,
	OK,
	CANCEL
};

class	TStatusView;

//====================================================================

class TStatusWindow : public BWindow
{
	public:
		TStatusWindow(BRect, BWindow *, const char *status);

	private:
		TStatusView *fView;
};

//--------------------------------------------------------------------

class TStatusView : public BBox
{
	public:
		TStatusView(BRect, BWindow *, const char *);

		virtual	void	AttachedToWindow();
		virtual void	MessageReceived(BMessage *);
		bool			Exists(const char *);

	private:
		const char		*fString;
		BTextControl	*fStatus;
		BWindow			*fWindow;
};

#endif // #ifndef _STATUS_H
