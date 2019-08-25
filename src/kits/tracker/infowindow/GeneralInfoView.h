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

#ifndef GENERALINFOVIEW_H
#define GENERALINFOVIEW_H


#include <GroupView.h>
#include <MenuField.h>
#include <Message.h>
#include <Rect.h>
#include <TextView.h>

#include "DialogPane.h"
#include "Model.h"


namespace BPrivate {

// States for tracking the mouse
enum track_state {
	no_track = 0,
	link_track,
	path_track,
	size_track,
};


class GeneralInfoView : public BGroupView {
public:
	GeneralInfoView(Model*);
	~GeneralInfoView();

	void ModelChanged(Model*, BMessage*);
	void ReLinkTargetModel(Model*);
	float CurrentFontHeight();

	off_t LastSize() const;
	void SetLastSize(off_t);

	void SetSizeString(const char*);

protected:
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32, const BMessage* dragMessage);
	virtual void MouseUp(BPoint where);
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	virtual void FrameResized(float, float);
	virtual void Draw(BRect);
	virtual void Pulse();
	virtual void WindowActivated(bool active);

private:
	void InitStrings(const Model*);
	void CheckAndSetSize();
	void OpenLinkSource();
	void OpenLinkTarget();

	BString fPathStr;
	BString fLinkToStr;
	BString fSizeString;
	BString fModifiedStr;
	BString fCreatedStr;
	BString fKindStr;
	BString fDescStr;

	off_t fFreeBytes;
	off_t fLastSize;

	BRect fPathRect;
	BRect fLinkRect;
	BRect fDescRect;
	BRect fSizeRect;
	float fDivider;

	BMenuField* fPreferredAppMenu;
	Model* fModel;
	bool fMouseDown;
	track_state fTrackingState;
	BWindow* fPathWindow;
	BWindow* fLinkWindow;
	BWindow* fDescWindow;
	color_which fCurrentLinkColorWhich;
	color_which fCurrentPathColorWhich;

	typedef BGroupView _inherited;
};

}	// namespace BPrivate


const uint32 kPermissionsSelected = 'sepe';
const uint32 kRecalculateSize = 'resz';
const uint32 kSetLinkTarget = 'link';


#endif /* !GENERALINFOVIEW_H */
