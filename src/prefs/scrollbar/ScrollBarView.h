//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		ScrollBarView.cpp
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	view which does all the work
//  
//------------------------------------------------------------------------------
#ifndef SCROLLBAR_VIEW
#define SCROLLBAR_VIEW

#include <View.h>
#include <Box.h>
#include <StringView.h>
#include <Button.h>
#include <ScrollBar.h>

class FakeScrollBar;
class KnobSizeAdjuster;

class ScrollBarView : public BView
{
public:
						ScrollBarView(void);
						~ScrollBarView(void);
	void				MessageReceived(BMessage *msg);
	void				AttachedToWindow(void);
	
private:

	void				HandleButtonStates(void);
	void				SetAllScrollBars(void);
	
	BButton				*fDefaultsButton;
	BButton				*fRevertButton;
	
	KnobSizeAdjuster	*fSizeAdjuster;
	
	FakeScrollBar		*fSingle;
	FakeScrollBar		*fDouble;
	FakeScrollBar		*fProportional;
	FakeScrollBar		*fFixed;
	FakeScrollBar		*fFlat;
	FakeScrollBar		*fDots;
	FakeScrollBar		*fLines;
};

#endif
