//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005 Haiku
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
//	File Name:		ScrollBar.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Client-side class for scrolling
//
//------------------------------------------------------------------------------
#include <Debug.h>
#include <Message.h>
#include <MessageRunner.h>
#include <OS.h>
#include <ScrollBar.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define TEST_MODE

#define SBC_SCROLLBYVALUE 0
#define SBC_SETDOUBLE 1
#define SBC_SETPROPORTIONAL 2
#define SBC_SETSTYLE 3

// Quick constants for determining which arrow is down and are defined with respect
// to double arrow mode. ARROW1 and ARROW4 refer to the outer pair of arrows and
// ARROW2 and ARROW3 refer to the inner ones. ARROW1 points left/up and ARROW4 
// points right/down.
#define ARROW1 0
#define ARROW2 1
#define ARROW3 2
#define ARROW4 3
#define NOARROW -1

// Because the R5 version kept a lot of data on server-side, we need to kludge our way
// into binary compatibility
class BScrollBar::Private {
public:
	Private()
		:
		fEnabled(true),
		fTracking(false),
		fThumbInc(1),
		fArrowDown(0),
		fButtonDown(NOARROW),
		fScrollRunner(NULL)
	{
		fMousePos.Set(0,0);
		
		//#ifdef TEST_MODE
			fScrollBarInfo.proportional = true;
			fScrollBarInfo.double_arrows = true;
			fScrollBarInfo.knob = 0;
			fScrollBarInfo.min_knob_size = 14;
		//#else
		//	get_scroll_bar_info(&fScrollBarInfo);
		//#endif
	}
	
	~Private()
	{
		StopMessageRunner();
	}
	
	status_t StartMessageRunner(BHandler *target, float value)
	{
		ASSERT(fScrollRunner == NULL);
		
		BMessage message(B_VALUE_CHANGED);
		message.AddFloat("value", value);
		fScrollRunner = new BMessageRunner(target, &message, 50000, -1);
		
		return fScrollRunner ? fScrollRunner->InitCheck() : B_NO_MEMORY;
	};
	
	void StopMessageRunner()
	{
		delete fScrollRunner;
		fScrollRunner = NULL;
	};
		
	bool fEnabled;
	
	// TODO: This should be a static, initialized by 
	// _init_interface_kit() at application startup-time,
	// like BMenu::sMenuInfo
	scroll_bar_info fScrollBarInfo;
		
	bool fTracking;
	BPoint fMousePos;
	float fThumbInc;
	
	uint32 fArrowDown;
	int8 fButtonDown;
	
	BMessageRunner *fScrollRunner;
};


BScrollBar::BScrollBar(BRect frame,const char *name,BView *target,float min,
		float max,orientation direction)
	:BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
 	fMin(min),
	fMax(max),
	fSmallStep(1),
	fLargeStep(10),
	fValue(0),
	fProportion(1),
	fTarget(NULL),
	fOrientation(direction)
{
	fPrivateData = new BScrollBar::Private;

	SetTarget(target);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	if (direction == B_VERTICAL) {
		if (frame.Width() > B_V_SCROLL_BAR_WIDTH)
			ResizeTo(B_V_SCROLL_BAR_WIDTH, frame.Height() + 1);

	} else {
		if (frame.Height() > B_H_SCROLL_BAR_HEIGHT)
			ResizeTo(frame.Width() + 1, B_H_SCROLL_BAR_HEIGHT);

	}
	
	SetResizingMode((direction == B_VERTICAL) ?
		B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT : 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
}


BScrollBar::BScrollBar(BMessage *archive)
	:BView(archive),
	fMin(0.0f),
	fMax(0.0f),
	fSmallStep(1.0f),
	fLargeStep(10.0f),
	fValue(0.0f),
	fProportion(1),
	fTarget(NULL),
	fOrientation(B_HORIZONTAL),
	fTargetName(NULL)
{
	archive->FindFloat("_range", &fMin);
	archive->FindFloat("_range", 1, &fMax);

	float smallStep, largeStep;
	if (archive->FindFloat("_steps", &smallStep) == B_OK &&
		archive->FindFloat("_steps", 1, &largeStep) == B_OK)
		SetSteps(smallStep, largeStep);

	float value;
	if (archive->FindFloat("_val", &value) == B_OK)
		SetValue(value);

	int32 direction;
	if (archive->FindInt32("_orient", &direction) == B_OK)
		fOrientation = (orientation)direction;
	else
		fOrientation = B_HORIZONTAL;

	float proportion;
	if (archive->FindFloat("_prop", &proportion) == B_OK)
		SetProportion(proportion);

	fPrivateData = new BScrollBar::Private;
	
	// TODO: SetTarget?
}


BScrollBar::~BScrollBar()
{
	fPrivateData->StopMessageRunner();
	
	if (fTarget) {
		if (Orientation() == B_VERTICAL)
			fTarget->fVerScroller = NULL;
		else
			fTarget->fHorScroller = NULL;
	}
	
	delete fPrivateData;
	free(fTargetName);
}


BArchivable *
BScrollBar::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BScrollBar"))
		return new BScrollBar(archive);
	else
		return NULL;
}


status_t
BScrollBar::Archive(BMessage *data, bool deep) const
{
	status_t err = BView::Archive(data, deep);

	if (err != B_OK)
		return err;
		
	data->AddFloat("_range",fMin);
	data->AddFloat("_range",fMax);
	data->AddFloat("_steps",fSmallStep);
	data->AddFloat("_steps",fLargeStep);
	data->AddFloat("_val",fValue);
	data->AddInt32("_orient",(int32)fOrientation);
	data->AddInt32("_prop",fProportion);
	
	return B_OK;
}


void
BScrollBar::AttachedToWindow()
{
	// R5's SB contacts the server if fValue!=0. I *think* we don't need to do anything here...
}


void
BScrollBar::SetValue(float value)
{
	if (value > fMax)
		value = fMax;
	if (value < fMin)
		value = fMin;
	
	if (value != fValue) {
		fValue = value;
		ValueChanged(fValue);
	}
}


float
BScrollBar::Value() const
{
	return fValue;
}


void
BScrollBar::SetProportion(float value)
{
	fProportion = value;
	Invalidate();
}


float
BScrollBar::Proportion() const
{
	return fProportion;
}


void
BScrollBar::ValueChanged(float newValue)
{
	if (fTarget == NULL)
		return;
		
	BPoint point = fTarget->Bounds().LeftTop();
	if (fOrientation == B_HORIZONTAL)
		point.x = newValue;
	else
		point.y = newValue;
				
	if (point != fTarget->Bounds().LeftTop()) {
		fTarget->ScrollTo(point);
		if (Window())
			Invalidate();
	}
}


void
BScrollBar::SetRange(float min, float max)
{
	fMin = min;
	fMax = max;
	
	if (fValue > fMax)
		fValue = fMax;
	else if (fValue < fMin)
		fValue = fMin;
	
	ValueChanged(fValue);
}


void
BScrollBar::GetRange(float *min, float *max) const
{
	if (min != NULL)
		*min = fMin;
	if (max != NULL)
		*max = fMax;
}


void
BScrollBar::SetSteps(float smallStep, float largeStep)
{
	// Under R5, steps can be set only after being attached to a window, probably because
	// the data is kept server-side. We'll just remove that limitation... :P
	
	// The BeBook also says that we need to specify an integer value even though the step
	// values are floats. For the moment, we'll just make sure that they are integers
	fSmallStep = (int32)smallStep;
	fLargeStep = (int32)largeStep;

	// TODO: test use of fractional values and make them work properly if they don't
}


void
BScrollBar::GetSteps(float *smallStep, float *largeStep) const
{
	if (smallStep != NULL)
		*smallStep = fSmallStep;
	if (largeStep)
		*largeStep = fLargeStep;
}


void
BScrollBar::SetTarget(BView *target)
{
	fTarget = target;
	free(fTargetName);
	
	if (fTarget) {
		fTargetName = strdup(target->Name());
		
		if (Orientation() == B_VERTICAL)
			fTarget->fVerScroller = this;
		else
			fTarget->fHorScroller = this;
	} else
		fTargetName = NULL;
}


void
BScrollBar::SetTarget(const char *targetName)
{
	if (!targetName)
		return;
	
	if (!Window())
		debugger("Method requires window and doesn't have one");
	
	BView *target = Window()->FindView(targetName);
	if (target)
		SetTarget(target);
}


BView *
BScrollBar::Target() const
{
	return fTarget;
}


orientation
BScrollBar::Orientation() const
{
	return fOrientation;
}


void
BScrollBar::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_VALUE_CHANGED:
		{
			float value;
			if (msg->FindFloat("value", &value) == B_OK)
				SetValue(Value() + value);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
BScrollBar::MouseDown(BPoint pt)
{
	BRect thumbFrame = KnobFrame();
		
	if (!(fMin == 0 && fMax == 0)) { // if fEnabled
		// Hit test for thumb
		if (thumbFrame.Contains(pt)) {
			fPrivateData->fTracking = true;
			fPrivateData->fMousePos = pt;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			Invalidate(thumbFrame);
			return;
		}
		
		BRect buttonrect(0, 0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT);
		
		// Hit test for arrow buttons
		if (fOrientation == B_VERTICAL) {
			if (buttonrect.Contains(pt)) {
				fPrivateData->fArrowDown = B_UP_ARROW;
				fPrivateData->fButtonDown = ARROW1;
				SetValue(Value() - fSmallStep);
				
				fPrivateData->StartMessageRunner(this, -fSmallStep);
				
				return;

			}

			buttonrect.OffsetTo(0, Bounds().Height() - (B_H_SCROLL_BAR_HEIGHT));
			if (buttonrect.Contains(pt)) {
				fPrivateData->fArrowDown = B_DOWN_ARROW;
				fPrivateData->fButtonDown = ARROW4;
				SetValue(Value() + fSmallStep);
				fPrivateData->StartMessageRunner(this, fSmallStep);
				return;
			}

			if (fPrivateData->fScrollBarInfo.double_arrows) {
				buttonrect.OffsetTo(0, B_H_SCROLL_BAR_HEIGHT + 1);
				if (buttonrect.Contains(pt)) {
					fPrivateData->fArrowDown = B_DOWN_ARROW;
					fPrivateData->fButtonDown = ARROW2;
					SetValue(Value() + fSmallStep);
					
					fPrivateData->StartMessageRunner(this, fSmallStep);
				
					return;
				}

				buttonrect.OffsetTo(0, Bounds().Height() - ((B_H_SCROLL_BAR_HEIGHT * 2) + 1));
				if (buttonrect.Contains(pt)) {
					fPrivateData->fArrowDown = B_UP_ARROW;
					fPrivateData->fButtonDown = ARROW3;
					SetValue(Value() - fSmallStep);
					fPrivateData->StartMessageRunner(this, -fSmallStep);
				
					return;
	
				}
			}
			
			// TODO: add a repeater thread for large stepping and a call to it

			if (pt.y < thumbFrame.top)
				SetValue(Value() - fLargeStep);
			else
				SetValue(Value() + fLargeStep);
		} else {
			if (buttonrect.Contains(pt)) {
				fPrivateData->fArrowDown = B_LEFT_ARROW;
				fPrivateData->fButtonDown = ARROW1;
				SetValue(Value() - fSmallStep);
				
				fPrivateData->StartMessageRunner(this, -fSmallStep);
				
				return;
			}
			
			buttonrect.OffsetTo(Bounds().Width() - (B_V_SCROLL_BAR_WIDTH), 0);
			if (buttonrect.Contains(pt)) {
				fPrivateData->fArrowDown = B_RIGHT_ARROW;
				fPrivateData->fButtonDown = ARROW4;
				SetValue(Value() + fSmallStep);
				
				fPrivateData->StartMessageRunner(this, fSmallStep);
				
				return;
			}
			
			if (fPrivateData->fScrollBarInfo.proportional) {
				buttonrect.OffsetTo(B_V_SCROLL_BAR_WIDTH + 1, 0);
				if (buttonrect.Contains(pt)) {
					fPrivateData->fButtonDown = ARROW2;
					fPrivateData->fArrowDown = B_LEFT_ARROW;
					SetValue(Value() + fSmallStep);
					
					fPrivateData->StartMessageRunner(this, fSmallStep);
				
					return;
				}
				
				buttonrect.OffsetTo(Bounds().Width() - ( (B_V_SCROLL_BAR_WIDTH * 2) + 1), 0);
				if (buttonrect.Contains(pt)) {
					fPrivateData->fButtonDown = ARROW3;
					fPrivateData->fArrowDown = B_RIGHT_ARROW;
					SetValue(Value() - fSmallStep);
				
					fPrivateData->StartMessageRunner(this, -fSmallStep);
				
					return;
				}
			}
			
			// We got this far, so apparently the user has clicked on something
			// that isn't the thumb or a scroll button, so scroll by a large step
			
			// TODO: add a repeater thread for large stepping and a call to it


			if (pt.x < thumbFrame.left)
				SetValue(Value() - fLargeStep);
			else
				SetValue(Value() + fLargeStep);
		}
	}
}


void
BScrollBar::MouseUp(BPoint pt)
{
	fPrivateData->StopMessageRunner();
				
	fPrivateData->fArrowDown = 0;
	fPrivateData->fButtonDown = NOARROW;
	
	// We'll be lazy here and just draw all the possible arrow regions for now...
	// TODO: optimize
	
	BRect rect(0, 0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT);
	
	if (fOrientation == B_VERTICAL) {
		rect.bottom += B_H_SCROLL_BAR_HEIGHT + 1;
		Invalidate(rect);
		rect.OffsetTo(0, Bounds().Height() - ((B_H_SCROLL_BAR_HEIGHT * 2) + 1));
		Invalidate(rect);
	} else {
		rect.bottom += B_V_SCROLL_BAR_WIDTH + 1;
		Invalidate(rect);
		rect.OffsetTo(0, Bounds().Height() - ((B_V_SCROLL_BAR_WIDTH * 2) + 1));
		Invalidate(rect);
	}
	
	if (fPrivateData->fTracking) {
		fPrivateData->fTracking = false;
		Invalidate(KnobFrame());
	}
}


void
BScrollBar::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if (!fPrivateData->fEnabled)
		return;
	
	if (fPrivateData->fTracking) {
		BRect thumbFrame = KnobFrame();
		float delta;
		if (fOrientation == B_VERTICAL) {
			if ((pt.y > thumbFrame.bottom && fValue == fMax) || 
				(pt.y < thumbFrame.top && fValue == fMin) )
				return;
			delta = pt.y - fPrivateData->fMousePos.y;
		} else {
			if ((pt.x > thumbFrame.right && fValue == fMax) || 
				(pt.x < thumbFrame.left && fValue == fMin))
				return;
			delta = pt.x - fPrivateData->fMousePos.x;
		}
		SetValue(Value() + delta);
		fPrivateData->fMousePos = pt;
	}
}


void
BScrollBar::DoScroll(float delta)
{
	if (!fTarget)
		return;
	
	float scrollval;

	if (delta > 0)
		scrollval = (fValue + delta <= fMax) ? delta : (fMax-fValue);
	else
		scrollval = (fValue - delta >= fMin) ? delta : (fValue - fMin);

	if (fOrientation == B_VERTICAL)
		fTarget->ScrollBy(0, scrollval);
	else
		fTarget->ScrollBy(scrollval, 0);
	
	
	fValue += scrollval;
}


void
BScrollBar::DetachedFromWindow()
{
	fTarget = NULL;
	free(fTargetName);
	fTargetName = NULL;
}


void
BScrollBar::Draw(BRect updateRect)
{
	rgb_color light, dark, normal, panelColor;
	panelColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (fPrivateData->fEnabled) {
		light = tint_color(panelColor, B_LIGHTEN_MAX_TINT);
		dark = tint_color(panelColor, B_DARKEN_3_TINT);
		normal = tint_color(panelColor, B_DARKEN_1_TINT);
	} else {
		light = tint_color(panelColor, B_LIGHTEN_MAX_TINT);
		dark = tint_color(panelColor, B_DARKEN_3_TINT);
		normal = panelColor;
	}
	
	// Draw main area
	SetHighColor(normal);
	FillRect(updateRect & BarFrame());
	
	SetHighColor(dark);
	StrokeRect(Bounds());

	SetDrawingMode(B_OP_OVER);

	DrawButtons(updateRect);
	
	SetDrawingMode(B_OP_COPY);

	// Draw scroll thumb
	if (fPrivateData->fEnabled) {
		BRect rect(KnobFrame());
		SetHighColor(dark);
		StrokeRect(rect);
	
		rect.InsetBy(1,1);
		SetHighColor(tint_color(panelColor, B_DARKEN_2_TINT));
		StrokeLine(rect.LeftBottom(), rect.RightBottom());
		StrokeLine(rect.RightTop(), rect.RightBottom());
	
		SetHighColor(light);
		StrokeLine(rect.LeftTop(), rect.RightTop());
		StrokeLine(rect.LeftTop(), rect.LeftBottom());
	
		rect.InsetBy(1,1);
		if (fPrivateData->fTracking)
			SetHighColor(normal);
		else
			SetHighColor(panelColor);
		
		FillRect(rect);
	}
	
	// TODO: Add the other thumb styles - dots and lines
}


void
BScrollBar::FrameMoved(BPoint new_position)
{
}


void
BScrollBar::FrameResized(float new_width, float new_height)
{
}


BHandler *
BScrollBar::ResolveSpecifier(BMessage *msg,int32 index,
		BMessage *specifier,int32 form,const char *property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


void
BScrollBar::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BScrollBar::GetPreferredSize(float *width, float *height)
{
	if (fOrientation == B_VERTICAL)
		*width = B_V_SCROLL_BAR_WIDTH;
	else if (fOrientation == B_HORIZONTAL)
		*height = B_H_SCROLL_BAR_HEIGHT;
}


void
BScrollBar::MakeFocus(bool state)
{
	if (fTarget)
		fTarget->MakeFocus(state);
}


void
BScrollBar::AllAttached()
{
	BView::AllAttached();
}


void
BScrollBar::AllDetached()
{
	BView::AllDetached();
}


status_t
BScrollBar::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


status_t
BScrollBar::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}


void BScrollBar::_ReservedScrollBar1() {}
void BScrollBar::_ReservedScrollBar2() {}
void BScrollBar::_ReservedScrollBar3() {}
void BScrollBar::_ReservedScrollBar4() {}


BScrollBar &
BScrollBar::operator=(const BScrollBar &)
{
	return *this;
}


void
BScrollBar::DrawArrow(BPoint pos, int32 which, bool pressed)
{
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
			lighten2 = { 255, 255, 255, 255 },
			darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
			darken2 = tint_color(no_tint, B_DARKEN_2_TINT);

	switch (which) {
		case B_LEFT_ARROW:
		{
			if (fMax > 0) {
				// Outer bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 3),
					BPoint(pos.x + 8.0f, pos.y));
				StrokeLine(BPoint(pos.x, pos.y + 4.0f),
					BPoint(pos.x, pos.y + 4.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 9.0f, pos.y),
					BPoint(pos.x + 9.0f, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 8.0f),
					BPoint(pos.x + 1.0f, pos.y + 5.0f));

				// Triangle
				SetHighColor(darken2);
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 1.0f),
					BPoint(pos.x + 8.0f, pos.y + 7.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 4.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 1.0f));

				// Inner bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 7.0f, pos.y + 6.0f),
					BPoint(pos.x + 5.0f, pos.y + 5.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 7, pos.y + 2.0f),
					BPoint(pos.x + 7.0f, pos.y + 5.0f));
				StrokeLine(BPoint(pos.x + 3.0f, pos.y + 4.0f),
					BPoint(pos.x + 6.0f, pos.y + 3.0f));
			} else {
				// Triangle
				SetHighColor(no_tint);
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 1.0f),
					BPoint(pos.x + 8.0f, pos.y + 7.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 4.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 1.0f));
			}

			break;
		}
		
		case B_RIGHT_ARROW:
		{
			if (fMax > 0) {
				// Outer bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x, pos.y), BPoint(pos.x, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y),
					BPoint(pos.x + 8.0f, pos.y + 3.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 9.0f, pos.y + 4.0f),
					BPoint(pos.x + 1.0f, pos.y + 8));

				// Triangle
				SetHighColor(darken2);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f),
					BPoint(pos.x + 1.0f, pos.y + 7.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 4.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f));

				// Inner bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 2.0f, pos.y + 6.0f),
					BPoint(pos.x + 4.0f, pos.y + 5.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 2.0f, pos.y + 2.0f),
					BPoint(pos.x + 2.0f, pos.y + 5.0f));
				StrokeLine(BPoint(pos.x + 3.0f, pos.y + 3.0f),
					BPoint(pos.x + 6.0f, pos.y + 4.0f));
			} else {
				// Triangle
				SetHighColor(no_tint);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f),
					BPoint(pos.x + 1.0f, pos.y + 7.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 4.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f));
			}

			break;
		}
		
		case B_UP_ARROW:
		{
			if (fMax > 0) {
				// Outer bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 3.0f, pos.y + 1), BPoint(pos.x, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y), BPoint(pos.x + 4.0f, pos.y));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x, pos.y + 9.0f), BPoint(pos.x + 8.0f, pos.y + 9.0f));
				StrokeLine(BPoint(pos.x + 8.0f, pos.y + 8.0f),
					BPoint(pos.x + 5.0f, pos.y + 1.0f));

				// Triangle
				SetHighColor(darken2);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 8.0f),
					BPoint(pos.x + 7.0f, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 1.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 8.0f));

				// Inner bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 6.0f, pos.y + 7.0f),
					BPoint(pos.x + 5.0f, pos.y + 5.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 2, pos.y + 7.0f),
					BPoint(pos.x + 5.0f, pos.y + 7.0f));
				StrokeLine(BPoint(pos.x + 3.0f, pos.y + 6.0f),
					BPoint(pos.x + 4.0f, pos.y + 3.0f));
			} else {
				// Triangle
				SetHighColor(no_tint);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 8.0f),
					BPoint(pos.x + 7.0f, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 1.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 8.0f));
			}

			break;
		}
		
		case B_DOWN_ARROW:
		{
			if (fMax > 0) {
				// Outer bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x, pos.y), BPoint(pos.x + 8.0f, pos.y));
				StrokeLine(BPoint(pos.x, pos.y + 1.0f),
					BPoint(pos.x + 3.0f, pos.y + 8.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 9.0f),
					BPoint(pos.x + 8.0f, pos.y + 1));

				// Triangle
				SetHighColor(darken2);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f),
					BPoint(pos.x + 7.0f, pos.y + 1.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f));

				// Inner bevel
				SetHighColor(darken1);
				StrokeLine(BPoint(pos.x + 6.0f, pos.y + 2.0f),
					BPoint(pos.x + 5.0f, pos.y + 4.0f));

				SetHighColor(lighten2);
				StrokeLine(BPoint(pos.x + 2, pos.y + 2.0f),
					BPoint(pos.x + 5.0f, pos.y + 2.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 6.0f),
					BPoint(pos.x + 3.0f, pos.y + 3.0f));	
			} else {
				// Triangle
				SetHighColor(no_tint);
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f),
					BPoint(pos.x + 7.0f, pos.y + 1.0f));
				StrokeLine(BPoint(pos.x + 4.0f, pos.y + 8.0f));
				StrokeLine(BPoint(pos.x + 1.0f, pos.y + 1.0f));
			}

			break;
		}
	}
}


void
BScrollBar::DrawButtons(BRect frame)
{
	const int8 &buttonDown = fPrivateData->fButtonDown;
	
	BRect bounds(Bounds());
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
		darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT);

	if (fOrientation == B_HORIZONTAL) {
		DrawButton(BRect(1.0f, 1.0f, 14.0f, bounds.bottom - 1.0f),
			B_LEFT_ARROW, buttonDown == ARROW1);

		SetHighColor(darken2);
		StrokeLine(BPoint(15.0f, 1.0f),
			BPoint(15.0f, bounds.bottom - 1.0f));

		// Second left button
		if (DoubleArrows()) {
			DrawButton(BRect(16.0f, 1.0f, 29.0f, bounds.bottom - 1.0f),
				B_RIGHT_ARROW, buttonDown == ARROW2);

			SetHighColor(darken2);
			StrokeLine(BPoint(30.0f, 1.0f),
				BPoint(30.0f, bounds.bottom - 1.0f));

		}

		// Second right button
		if (DoubleArrows()) {
			SetHighColor(darken2);
				StrokeLine(BPoint(bounds.right - 30.0f, 1.0f),
			BPoint(bounds.right - 30.0f, bounds.bottom - 1.0f));

			DrawButton(BRect(bounds.right - 29.0f, 1.0f,
				bounds.right - 16.0f, bounds.bottom - 1.0f),
				B_LEFT_ARROW, buttonDown == ARROW3);

		}

		// Right button
		SetHighColor(darken2);
		StrokeLine(BPoint(bounds.right - 15.0f, 1.0f),
			BPoint(bounds.right - 15.0f, bounds.bottom - 1.0f));

		DrawButton(BRect(bounds.right - 14.0f, 1.0f,
			bounds.right - 1.0f, bounds.bottom - 1.0f),
				B_RIGHT_ARROW, buttonDown == ARROW4);

	} else if (fOrientation == B_VERTICAL) {
		// Top button
		DrawButton(BRect(1.0f, 1.0f, bounds.right - 1.0f, 14.0f),
				B_UP_ARROW, buttonDown == ARROW1);

		SetHighColor(darken2);
		StrokeLine(BPoint(1.0f, 15.0f),
			BPoint(bounds.right - 1.0f, 15.0f));

		// Second top button
		if (DoubleArrows()) {
			DrawButton(BRect(1.0f, 16.0f, bounds.right - 1.0f, 29.0f),
				B_DOWN_ARROW, buttonDown == ARROW2);
				
			SetHighColor(darken2);
			StrokeLine(BPoint(1.0f, 30.0f),
				BPoint(bounds.right - 1.0f, 30.0f));
		}

		// Second bottom button
		if (DoubleArrows()) {
			SetHighColor(darken2);
				StrokeLine(BPoint(1.0f, bounds.bottom - 30.0f),
			BPoint(bounds.right - 1.0f, bounds.bottom - 30.0f));

			DrawButton(BRect(1.0f, bounds.bottom - 29.0f,
				bounds.right - 1.0f, bounds.bottom - 16.0f),
				B_UP_ARROW, buttonDown == ARROW3);

		}

		// bottom button
		SetHighColor(darken2);
		StrokeLine(BPoint(1.0f, bounds.bottom - 15.0f),
			BPoint(bounds.right - 1.0f, bounds.bottom - 15.0f));

		DrawButton(BRect(1.0f, bounds.bottom - 14.0f,
				bounds.right - 1.0f, bounds.bottom - 1.0f),
				B_DOWN_ARROW, buttonDown == ARROW4);
	}
}


void
BScrollBar::DrawButton(BRect frame, int32 arrowType, bool pressed)
{
	rgb_color no_tint = ui_color(B_PANEL_BACKGROUND_COLOR),
		lighten2 = tint_color(no_tint, B_LIGHTEN_2_TINT),
		lightenmax = tint_color(no_tint, B_LIGHTEN_MAX_TINT),
		darken1 = tint_color(no_tint, B_DARKEN_1_TINT),
		darken2 = tint_color(no_tint, B_DARKEN_2_TINT);

	if (fMax > 0) {
		SetHighColor(no_tint);
		FillRect(frame);
		
		SetHighColor(lighten2);
		StrokeLine(BPoint(frame.left, frame.bottom - 1.0f),
			BPoint(frame.left, frame.top));
		StrokeLine(BPoint(frame.right - 1.0f, frame.top));

		SetHighColor(darken1);
		StrokeLine(BPoint(frame.right, frame.top + 1.0f),
			BPoint(frame.right, frame.bottom));
		StrokeLine(BPoint(frame.left + 1.0f, frame.bottom));
	
	} else {
		SetHighColor(lighten2);
		FillRect(frame);

		SetHighColor(lightenmax);
		StrokeLine(BPoint(frame.left, frame.bottom - 1.0f),
			BPoint(frame.left, frame.top));
		StrokeLine(BPoint(frame.right - 1.0f, frame.top));

		SetHighColor(no_tint);
		StrokeLine(BPoint(frame.right, frame.top + 1.0f),
			BPoint(frame.right, frame.bottom));
		StrokeLine(BPoint(frame.left + 1.0f, frame.bottom));
	}
	
	BPoint arrowPoint = frame.LeftTop() + BPoint(2, 3);
	DrawArrow(arrowPoint, arrowType, pressed);
	
	if (pressed) {
		// TODO: This isn't cheap, but it'll do for now...
		drawing_mode mode = DrawingMode();
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(128, 128, 128, 128);
		FillRect(frame);
		SetDrawingMode(mode);
	}
}


bool
BScrollBar::DoubleArrows() const
{
	if (!fPrivateData->fScrollBarInfo.double_arrows)
		return false;

	if (fOrientation == B_HORIZONTAL)
		return Bounds().Width() > 16 * 4 + fPrivateData->fScrollBarInfo.min_knob_size * 2;
	else
		return Bounds().Height() > 16 * 4 + fPrivateData->fScrollBarInfo.min_knob_size * 2;
}


BRect
BScrollBar::BarFrame() const
{
	BRect rect(Bounds());

	if (fOrientation == B_HORIZONTAL) {
		float modifier = B_V_SCROLL_BAR_WIDTH + 1;
		if (DoubleArrows())
			modifier *= 2;
		
		rect.left += modifier;
		rect.right -= modifier;
	
	} else if (fOrientation == B_VERTICAL) {
		float modifier = B_H_SCROLL_BAR_HEIGHT + 1;
		if (DoubleArrows())
			modifier *= 2;
			
		rect.top += modifier;
		rect.bottom -= modifier;
		
	}

	return rect;
}


BRect
BScrollBar::KnobFrame() const
{
	BRect barFrame(BarFrame());
	BRect rect(barFrame);

	if (fOrientation == B_HORIZONTAL) {
		rect.left += ValueToPosition(fValue);
		rect.right = rect.left + fPrivateData->fScrollBarInfo.min_knob_size;
	
	} else if (fOrientation == B_VERTICAL) {
		rect.top += ValueToPosition(fValue);
		rect.bottom = rect.top + fPrivateData->fScrollBarInfo.min_knob_size;
	}
	
	return rect;
}


float
BScrollBar::ValueToPosition(float val) const
{
	return ceil(val - fMin);
}


float
BScrollBar::PositionToValue(float pos) const
{
	return pos;
}


/*
	This cheat function will allow the scrollbar prefs app to act like R5's and
	perform other stuff without mucking around with the virtual tables.
	// TODO: Using private friend methods for this is not nice, we should use a
	// custom control in the pref app instead
	
	B_BAD_VALUE is returned when a NULL scrollbar pointer is passed to it.

	The scroll_bar_info struct is read and used to re-style the given BScrollBar.
*/
status_t
control_scrollbar(scroll_bar_info *info, BScrollBar *bar)
{
	if (!bar || !info)
		return B_BAD_VALUE;

	if (bar->fPrivateData->fScrollBarInfo.double_arrows != info->double_arrows) {
		bar->fPrivateData->fScrollBarInfo.double_arrows = info->double_arrows;
		
		//int8 multiplier = (info->double_arrows) ? 1 : -1;
	}

	bar->fPrivateData->fScrollBarInfo.proportional = info->proportional;

	// TODO: Figure out how proportional relates to the size of the thumb
		
	// TODO: Add redraw code to reflect the changes

	if (info->knob >= 0 && info->knob <= 2)
		bar->fPrivateData->fScrollBarInfo.knob = info->knob;
	else
		return B_BAD_VALUE;
	
	if (info->min_knob_size >= SCROLL_BAR_MINIMUM_KNOB_SIZE && info->min_knob_size <= SCROLL_BAR_MAXIMUM_KNOB_SIZE)
		bar->fPrivateData->fScrollBarInfo.min_knob_size = info->min_knob_size;
	else
		return B_BAD_VALUE;

	return B_OK;
}
