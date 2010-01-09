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

#ifndef _BUTTON_BAR_H
#define _BUTTON_BAR_H

#include <Box.h>

#include "BmapButton.h"

struct BBDivider;

class ButtonBar : public BView {
public:
	ButtonBar(BRect frame, const char *name, uint8 enabledOffset,
		uint8 disabledOffset, uint8 rollOffset, uint8 pressedOffset,
		float Hmargin, float Vmargin, 
		uint32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		int32 flags = B_NAVIGABLE_JUMP | B_FRAME_EVENTS | B_WILL_DRAW);
	virtual ~ButtonBar( void );
		
	// Hooks
	virtual void GetPreferredSize(float *width, float *height);
	virtual void AttachedToWindow(void);
	virtual void Draw(BRect updateRect);
		
	void ShowLabels(bool show);
	void Arrange(bool fixedWidth = true);
	BmapButton *AddButton(const char *label, int32 baseID, BMessage *msg,
							int32 position = -1);
	bool RemoveButton(BmapButton *button);
	int32 IndexOf(BmapButton *button);
	void AddDivider(float vmargin);
		
protected:
	float fMaxHeight;
	float fMaxWidth;
	float fNextXOffset;
	float fHMargin;
	float fVMargin;
	uint8 fEnabledOffset;
	uint8 fDisabledOffset;
	uint8 fRollOffset;
	uint8 fPressedOffset;
	BList fButtonList;
	BBDivider *fDividerArray;
	int32 fDividers;
	bool fShowLabels;
};

#endif // #ifndef _BUTTON_BAR_H

