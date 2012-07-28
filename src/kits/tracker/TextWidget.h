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

#ifndef _TEXT_WIDGET_H
#define _TEXT_WIDGET_H

#include "Model.h"
#include "WidgetAttributeText.h"

namespace BPrivate {

class BPose;
class BPoseView;
class BColumn;

class BTextWidget {
public:
	BTextWidget(Model*, BColumn*, BPoseView*);
	virtual ~BTextWidget();

	void Draw(BRect widgetRect, BRect widgetTextRect, float width, BPoseView*,
		bool selected, uint32 clipboardMode);
	void Draw(BRect widgetRect, BRect widgetTextRect, float width, BPoseView*,
		BView* drawView, bool selected, uint32 clipboardMode, BPoint offset, bool direct);
		// second call is used for offscreen drawing, where PoseView
		// and current drawing view are different

	void MouseUp(BRect bounds, BPoseView*, BPose*, BPoint mouseLoc);
	
	BRect CalcRect(BPoint poseLoc, const BColumn*, const BPoseView*);
		// returns the rect derived from the formatted string width
		// may force WidgetAttributeText recalculation
	BRect CalcClickRect(BPoint poseLoc, const BColumn*, const BPoseView*);
		// calls CalcRect, if result too narow, returns a wider rect for
		// easy clicking
	BRect ColumnRect(BPoint poseLoc, const BColumn*, const BPoseView*);
		// returns the rect of the widget in a column, regardless
		// of the string width; faster than CalcRect
	BRect CalcOldRect(BPoint poseLoc, const BColumn*, const BPoseView*);
		// after an update call this to determine the old rect so that
		// we can invalidate properly
		
	void StartEdit(BRect bounds, BPoseView*, BPose*);
	void StopEdit(bool saveChanges, BPoint loc, BPoseView*, BPose*, int32 index);

	void SelectAll(BPoseView* view);
	void CheckAndUpdate(BPoint, const BColumn*, BPoseView*, bool visible);

	uint32 AttrHash() const;
	bool IsEditable() const;
	void SetEditable(bool);
	bool IsVisible() const;
	void SetVisible(bool);
	bool IsActive() const;
	void SetActive(bool);

	const char* Text(const BPoseView* view) const;
		// returns the untruncated version of the text
	float TextWidth(const BPoseView*) const;
	float PreferredWidth(const BPoseView*) const;
	int Compare(const BTextWidget&, BPoseView*) const;
		// used for sorting in PoseViews
	
private:
	BRect CalcRectCommon(BPoint poseLoc, const BColumn*, const BPoseView*,
		float width);

	WidgetAttributeText* fText;
	uint32 fAttrHash;
		// TODO: get rid of this
	alignment fAlignment;

	bool fEditable : 1;
	bool fVisible : 1;
	bool fActive : 1;
	bool fSymLink : 1;
};


inline uint32
BTextWidget::AttrHash() const
{
	return fAttrHash;
}


inline void
BTextWidget::SetEditable(bool on)
{
	fEditable = on;
}


inline bool
BTextWidget::IsEditable() const
{
	return fEditable && fText->IsEditable();
}


inline bool
BTextWidget::IsVisible() const
{
	return fVisible;
}


inline void
BTextWidget::SetVisible(bool on)
{
	fVisible = on;
}


inline bool
BTextWidget::IsActive() const
{
	return fActive;
}


inline void
BTextWidget::SetActive(bool on)
{
	fActive = on;
}


inline void
BTextWidget::Draw(BRect widgetRect, BRect widgetTextRect, float width,
	BPoseView* view, bool selected, uint32 clipboardMode)
{
	Draw(widgetRect, widgetTextRect, width, view, (BView*)view, selected,
		clipboardMode, BPoint(0, 0), true);
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// _TEXT_WIDGET_H
