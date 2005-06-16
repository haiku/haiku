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
#include <Message.h>
#include <OS.h>
#include <ScrollBar.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define TEST_MODE

typedef enum {
	ARROW_LEFT = 0,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	ARROW_NONE
} arrow_direction;

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
		fRepeaterThread(-1),
		fExitRepeater(false),
		fTracking(false),
		fThumbInc(1),
		fArrowDown(ARROW_NONE),
		fButtonDown(NOARROW)		
	{
		fThumbFrame.Set(0, 0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT);
		fMousePos.Set(0,0);
		
		#ifdef TEST_MODE
			fScrollBarInfo.proportional = true;
			fScrollBarInfo.double_arrows = false;
			fScrollBarInfo.knob = 0;
			fScrollBarInfo.min_knob_size = 14;
		#else
			get_scroll_bar_info(&fScrollBarInfo);
		#endif
	}
	
	~Private()
	{
		if (fRepeaterThread >= 0) {
			status_t dummy;
			fExitRepeater = true;
			wait_for_thread(fRepeaterThread, &dummy);
		}
	}
	
	void DrawScrollBarButton(BScrollBar *owner, arrow_direction direction, 
		const BPoint &offset, bool down = false);
	
	static int32 ButtonRepeaterThread(void *data);
	
	bool fEnabled;
	
	// TODO: This should be a static, initialized by 
	// _init_interface_kit() at application startup-time,
	// like BMenu::sMenuInfo
	scroll_bar_info fScrollBarInfo;
	
	thread_id fRepeaterThread;
	bool fExitRepeater;
	
	BRect fThumbFrame;
	bool fTracking;
	BPoint fMousePos;
	float fThumbInc;
	
	arrow_direction fArrowDown;
	int8 fButtonDown;
};


// This thread is spawned when a button is initially pushed and repeatedly scrolls
// the scrollbar by a little bit after a short delay
int32
BScrollBar::Private::ButtonRepeaterThread(void *data)
{
	BScrollBar *scrollBar = static_cast<BScrollBar *>(data);
	BRect oldframe(scrollBar->fPrivateData->fThumbFrame);
//	BRect update(sb->fPrivateData->fThumbFrame);

	snooze(250000);

	bool exitval = false;
	status_t returnval;

	scrollBar->Window()->Lock();
	exitval = scrollBar->fPrivateData->fExitRepeater;
	scrollBar->Window()->Unlock();

	float scrollvalue = 0;

	if (scrollBar->fPrivateData->fArrowDown == ARROW_LEFT
		|| scrollBar->fPrivateData->fArrowDown == ARROW_UP)
		scrollvalue = -scrollBar->fSmallStep;
	else if (scrollBar->fPrivateData->fArrowDown != ARROW_NONE)
		scrollvalue = scrollBar->fSmallStep;
	else
		exitval = true;
	
	while (!exitval) {
		oldframe = scrollBar->fPrivateData->fThumbFrame;

		scrollBar->Window()->Lock();
		returnval = scroll_by_value(scrollvalue, scrollBar);
		scrollBar->Window()->Unlock();

		snooze(50000);

		scrollBar->Window()->Lock();
		exitval = scrollBar->fPrivateData->fExitRepeater;

		if (returnval == B_OK) {
			scrollBar->CopyBits(oldframe, scrollBar->fPrivateData->fThumbFrame);

			// TODO: Redraw the old area here

			scrollBar->ValueChanged(scrollBar->fValue);
		}
		scrollBar->Window()->Unlock();
	}

	scrollBar->Window()->Lock();
	scrollBar->fPrivateData->fExitRepeater = false;
	scrollBar->fPrivateData->fRepeaterThread = -1;
	scrollBar->Window()->Unlock();
	
	return 0;
}


BScrollBar::BScrollBar(BRect frame,const char *name,BView *target,float min,
		float max,orientation direction)
 : BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
 	fMin(min),
	fMax(max),
	fSmallStep(1),
	fLargeStep(10),
	fValue(0),
	fTarget(target),
	fOrientation(direction)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fPrivateData = new BScrollBar::Private;

	if (fTarget) {
		fTargetName = strdup(fTarget->Name());
		
		// TODO: theoretically, we should also set the target BView's scrollbar
		// pointer here
	}
	else
		fTargetName=NULL;
	
	if (direction == B_VERTICAL) {
		if (frame.Width() > B_V_SCROLL_BAR_WIDTH)
			ResizeTo(B_V_SCROLL_BAR_WIDTH, frame.Height() + 1);

		fPrivateData->fThumbFrame.bottom = fPrivateData->fScrollBarInfo.min_knob_size;
		if (fPrivateData->fScrollBarInfo.double_arrows)
			fPrivateData->fThumbFrame.OffsetBy(0, (B_H_SCROLL_BAR_HEIGHT + 1) * 2);
		else
			fPrivateData->fThumbFrame.OffsetBy(0, B_H_SCROLL_BAR_HEIGHT + 1);
	} else {
		if (frame.Height() > B_H_SCROLL_BAR_HEIGHT)
			ResizeTo(frame.Width() + 1, B_H_SCROLL_BAR_HEIGHT);

		fPrivateData->fThumbFrame.right = fPrivateData->fScrollBarInfo.min_knob_size;
		if (fPrivateData->fScrollBarInfo.double_arrows)
			fPrivateData->fThumbFrame.OffsetBy((B_V_SCROLL_BAR_WIDTH + 1) * 2, 0);
		else
			fPrivateData->fThumbFrame.OffsetBy(B_V_SCROLL_BAR_WIDTH + 1, 0);
	}
	
	
	SetResizingMode((direction == B_VERTICAL) ?
		B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT : 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
	
}


BScrollBar::BScrollBar(BMessage *data)
 : BView(data)
{
}


BScrollBar::~BScrollBar()
{
	delete fPrivateData;
	free(fTargetName);

	// TODO: Disconnect from target
}


BArchivable *
BScrollBar::Instantiate(BMessage *data)
{
	// TODO: Implement
	return NULL;
}


status_t
BScrollBar::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data,deep);
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
	if(value > fMax)
		value = fMax;
	if(value < fMin)
		value = fMin;
	
	fValue = value;
	if (Window())
		Invalidate();
	
	ValueChanged(fValue);
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
}


float
BScrollBar::Proportion() const
{
	return fProportion;
}


void
BScrollBar::ValueChanged(float newValue)
{
	// TODO: Implement
/*
	From the BeBook:
	
Responds to a notification that the value of the scroll bar has changed to 
newValue. For a horizontal scroll bar, this function interprets newValue 
as the coordinate value that should be at the left side of the target 
view's bounds rectangle. For a vertical scroll bar, it interprets 
newValue as the coordinate value that should be at the top of the rectangle. 
It calls ScrollTo() to scroll the target's contents into position, unless 
they have already been scrolled. 

ValueChanged() is called as the result both of user actions 
(B_VALUE_CHANGED messages received from the Application Server) and of 
programmatic ones. Programmatically, scrolling can be initiated by the 
target view (calling ScrollTo()) or by the BScrollBar 
(calling SetValue() or SetRange()). 

In all these cases, the target view and the scroll bars need to be kept 
in synch. This is done by a chain of function calls: ValueChanged() calls 
ScrollTo(), which in turn calls SetValue(), which then calls 
ValueChanged() again. It's up to ValueChanged() to get off this 
merry-go-round, which it does by checking the target view's bounds 
rectangle. If newValue already matches the left or top side of the 
bounds rectangle, if forgoes calling ScrollTo(). 

ValueChanged() does nothing if a target BView hasn't been set—or 
if the target has been set by name, but the name doesn't correspond to 
an actual BView within the scroll bar's window. 

*/
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
	
	Invalidate();
	
	// Just a sort-of hack for now. ValueChanged is called, but with
	// what value??
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
			int32 value;
			if (msg->FindInt32("value", &value) == B_OK)
				ValueChanged(value);
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
	if (!(fMin == 0 && fMax == 0)) { // if fEnabled
	
		// Hit test for thumb
		if (fPrivateData->fThumbFrame.Contains(pt)) {
			fPrivateData->fTracking = true;
			fPrivateData->fMousePos = pt;
			// TODO: empty event mask ? Is this okay ?
			SetMouseEventMask(0, B_LOCK_WINDOW_FOCUS);
			Draw(fPrivateData->fThumbFrame);
			return;
		}
		
		BRect buttonrect(0, 0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT);
		float scrollval = 0;
		status_t returnval = B_ERROR;
		
		// Hit test for arrow buttons
		if (fOrientation == B_VERTICAL) {
			if (buttonrect.Contains(pt)) {
				scrollval = -fSmallStep;
				fPrivateData->fArrowDown = ARROW_UP;
				fPrivateData->fButtonDown = ARROW1;
				returnval = scroll_by_value(scrollval, this);
		
				if (returnval == B_OK) {
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if (fPrivateData->fRepeaterThread == -1) {
						fPrivateData->fExitRepeater = false;
						fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
							"scroll repeater", B_NORMAL_PRIORITY, this);
						resume_thread(fPrivateData->fRepeaterThread);
					}
				}
				
				return;

			}

			buttonrect.OffsetTo(0, Bounds().Height() - (B_H_SCROLL_BAR_HEIGHT));
			if (buttonrect.Contains(pt)) {
				scrollval = fSmallStep;
				fPrivateData->fArrowDown = ARROW_DOWN;
				fPrivateData->fButtonDown = ARROW4;
				returnval = scroll_by_value(scrollval, this);

				if (returnval == B_OK) {
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if (fPrivateData->fRepeaterThread == -1) {
						fPrivateData->fExitRepeater = false;
						fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
							"scroll repeater", B_NORMAL_PRIORITY, this);
						resume_thread(fPrivateData->fRepeaterThread);
					}
				}
				return;
			}

			if (fPrivateData->fScrollBarInfo.double_arrows) {
				buttonrect.OffsetTo(0, B_H_SCROLL_BAR_HEIGHT + 1);
				if (buttonrect.Contains(pt)) {
					scrollval = fSmallStep;
					fPrivateData->fArrowDown = ARROW_DOWN;
					fPrivateData->fButtonDown = ARROW2;
					returnval = scroll_by_value(scrollval, this);
			
					if (returnval == B_OK) {
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if (fPrivateData->fRepeaterThread == -1) {
							fPrivateData->fExitRepeater = false;
							fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
								"scroll repeater", B_NORMAL_PRIORITY, this);
							resume_thread(fPrivateData->fRepeaterThread);
						}
					}
					return;
				}

				buttonrect.OffsetTo(0, Bounds().Height() - ((B_H_SCROLL_BAR_HEIGHT * 2) + 1));
				if (buttonrect.Contains(pt)) {
					scrollval = -fSmallStep;
					fPrivateData->fArrowDown = ARROW_UP;
					fPrivateData->fButtonDown = ARROW3;
					returnval = scroll_by_value(scrollval, this);
			
					if (returnval == B_OK) {
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if (fPrivateData->fRepeaterThread == -1) {
							fPrivateData->fExitRepeater = false;
							fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
								"scroll repeater", B_NORMAL_PRIORITY, this);
							resume_thread(fPrivateData->fRepeaterThread);
						}
					}
					return;
	
				}
			}
			
			// TODO: add a repeater thread for large stepping and a call to it

			if (pt.y < fPrivateData->fThumbFrame.top)
				scroll_by_value(-fLargeStep, this);  // do we not check the return value in these two cases like everywhere else?
			else
				scroll_by_value(fLargeStep, this);
		} else {
			if (buttonrect.Contains(pt)) {
				scrollval = -fSmallStep;
				fPrivateData->fArrowDown = ARROW_LEFT;
				fPrivateData->fButtonDown = ARROW1;
				returnval = scroll_by_value(scrollval, this);
		
				if (returnval == B_OK) {
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(fPrivateData->fRepeaterThread == -1) {
						fPrivateData->fExitRepeater = false;
						fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
							"scroll repeater", B_NORMAL_PRIORITY, this);
						resume_thread(fPrivateData->fRepeaterThread);
					}
				}
				return;
			}
			
			buttonrect.OffsetTo(Bounds().Width() - (B_V_SCROLL_BAR_WIDTH), 0);
			if (buttonrect.Contains(pt)) {
				scrollval = fSmallStep;
				fPrivateData->fArrowDown = ARROW_RIGHT;
				fPrivateData->fButtonDown = ARROW4;
				returnval = scroll_by_value(scrollval, this);
		
				if (returnval == B_OK) {
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(fPrivateData->fRepeaterThread == -1) {
						fPrivateData->fExitRepeater = false;
						fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
							"scroll repeater", B_NORMAL_PRIORITY,this);
						resume_thread(fPrivateData->fRepeaterThread);
					}
				}
				return;
			}
			
			if (fPrivateData->fScrollBarInfo.proportional) {
				buttonrect.OffsetTo(B_V_SCROLL_BAR_WIDTH + 1, 0);
				if (buttonrect.Contains(pt)) {
					scrollval = fSmallStep;
					fPrivateData->fButtonDown = ARROW2;
					fPrivateData->fArrowDown = ARROW_LEFT;
					returnval = scroll_by_value(scrollval, this);
					
					if (returnval == B_OK) {
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if (fPrivateData->fRepeaterThread == -1) {
							fPrivateData->fExitRepeater = false;
							fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
								"scroll repeater", B_NORMAL_PRIORITY, this);
							resume_thread(fPrivateData->fRepeaterThread);
						}
					}
					return;
				}
				
				buttonrect.OffsetTo(Bounds().Width() - ( (B_V_SCROLL_BAR_WIDTH * 2) + 1), 0);
				if (buttonrect.Contains(pt)) {
					scrollval = -fSmallStep;
					fPrivateData->fButtonDown = ARROW3;
					fPrivateData->fArrowDown = ARROW_RIGHT;
					returnval = scroll_by_value(scrollval, this);
				
					if (returnval == B_OK) {
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if(fPrivateData->fRepeaterThread == -1) {
							fPrivateData->fExitRepeater = false;
							fPrivateData->fRepeaterThread = spawn_thread(fPrivateData->ButtonRepeaterThread,
								"scroll repeater", B_NORMAL_PRIORITY, this);
							resume_thread(fPrivateData->fRepeaterThread);
						}
					}
					return;
				}
			}
			
			// We got this far, so apparently the user has clicked on something
			// that isn't the thumb or a scroll button, so scroll by a large step
			
			// TODO: add a repeater thread for large stepping and a call to it


			if (pt.x < fPrivateData->fThumbFrame.left)
				scroll_by_value(-fLargeStep, this);  // do we not check the return value in these two cases like everywhere else?
			else
				scroll_by_value(fLargeStep, this);

		}
		ValueChanged(fValue);
		Draw(Bounds());
	}
}


void
BScrollBar::MouseUp(BPoint pt)
{
	fPrivateData->fArrowDown = ARROW_NONE;
	fPrivateData->fButtonDown = NOARROW;
	fPrivateData->fExitRepeater = true;
	
	// We'll be lazy here and just draw all the possible arrow regions for now...
	// TODO: optimize
	
	BRect rect(0, 0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT);
	
	if (fOrientation == B_VERTICAL) {
		rect.bottom += B_H_SCROLL_BAR_HEIGHT + 1;
		Draw(rect);
		rect.OffsetTo(0, Bounds().Height() - ((B_H_SCROLL_BAR_HEIGHT * 2) + 1));
		Draw(rect);
	} else {
		rect.bottom += B_V_SCROLL_BAR_WIDTH + 1;
		Draw(rect);
		rect.OffsetTo(0, Bounds().Height() - ((B_V_SCROLL_BAR_WIDTH * 2) + 1));
		Draw(rect);
	}
	
	if (fPrivateData->fTracking) {
		fPrivateData->fTracking = false;
		SetMouseEventMask(0, 0);
		Draw(fPrivateData->fThumbFrame);
	}
}


void
BScrollBar::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if (!fPrivateData->fEnabled)
		return;
	
	if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW)
		MouseUp(fPrivateData->fMousePos);

	if (fPrivateData->fTracking) {
		float delta;
		if (fOrientation == B_VERTICAL) {
			if( (pt.y > fPrivateData->fThumbFrame.bottom && fValue == fMax) || 
				(pt.y < fPrivateData->fThumbFrame.top && fValue == fMin) )
				return;
			delta = pt.y - fPrivateData->fMousePos.y;
		} else {
			if((pt.x > fPrivateData->fThumbFrame.right && fValue == fMax) || 
				(pt.x < fPrivateData->fThumbFrame.left && fValue == fMin))
				return;
			delta = pt.x - fPrivateData->fMousePos.x;
		}
		scroll_by_value(delta, this);  // do we not check the return value here?
		ValueChanged(fValue);
		Invalidate(Bounds());
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

	if (fOrientation == B_VERTICAL) {
		fTarget->ScrollBy(0, scrollval);
		fPrivateData->fThumbFrame.OffsetBy(0, scrollval);
	} else {
		fTarget->ScrollBy(scrollval, 0);
		fPrivateData->fThumbFrame.OffsetBy(scrollval, 0);
	}
	
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
		normal = panelColor;
	} else {
		light = tint_color(panelColor, B_LIGHTEN_MAX_TINT);
		dark = tint_color(panelColor, B_DARKEN_3_TINT);
		normal = panelColor;
	}
	
	// Draw main area
	SetHighColor(normal);
	FillRect(updateRect);
	
	SetHighColor(dark);
	StrokeRect(Bounds());

	SetDrawingMode(B_OP_OVER);

	// Draw arrows
	BPoint buttonpt(0,0);
	if (fOrientation == B_HORIZONTAL) {
		fPrivateData->DrawScrollBarButton(this, ARROW_LEFT, buttonpt,
			fPrivateData->fButtonDown == ARROW1);
		
		if (fPrivateData->fScrollBarInfo.double_arrows) {
			buttonpt.Set(B_V_SCROLL_BAR_WIDTH + 1, 0);
			fPrivateData->DrawScrollBarButton(this, ARROW_RIGHT, buttonpt, 
				fPrivateData->fButtonDown == ARROW2);

			buttonpt.Set(Bounds().Width() - ((B_V_SCROLL_BAR_WIDTH * 2) + 1), 0);
			fPrivateData->DrawScrollBarButton(this, ARROW_LEFT, buttonpt,
				fPrivateData->fButtonDown == ARROW3);
		
		}

		buttonpt.Set(Bounds().Width() - (B_V_SCROLL_BAR_WIDTH), 0);
		fPrivateData->DrawScrollBarButton(this, ARROW_RIGHT, buttonpt,
			fPrivateData->fButtonDown == ARROW4);
	} else {
		fPrivateData->DrawScrollBarButton(this, ARROW_UP, buttonpt,
			fPrivateData->fButtonDown == ARROW1);
		
		if (fPrivateData->fScrollBarInfo.double_arrows) {
			buttonpt.Set(0,B_H_SCROLL_BAR_HEIGHT + 1);
			fPrivateData->DrawScrollBarButton(this, ARROW_DOWN, buttonpt,
				fPrivateData->fButtonDown == ARROW2);

			buttonpt.Set(0,Bounds().Height() - ((B_H_SCROLL_BAR_HEIGHT * 2) + 1));
			fPrivateData->DrawScrollBarButton(this, ARROW_UP, buttonpt,
				fPrivateData->fButtonDown == ARROW3);
		
		}

		buttonpt.Set(0,Bounds().Height() - (B_H_SCROLL_BAR_HEIGHT));
		fPrivateData->DrawScrollBarButton(this, ARROW_DOWN, buttonpt,
			fPrivateData->fButtonDown == ARROW4);
	}

	SetDrawingMode(B_OP_COPY);

	// Draw scroll thumb
	
	if (fPrivateData->fEnabled) {
		BRect rect(fPrivateData->fThumbFrame);
	
		SetHighColor(dark);
		StrokeRect(fPrivateData->fThumbFrame);
	
		rect.InsetBy(1,1);
		SetHighColor(tint_color(panelColor, B_DARKEN_2_TINT));
		StrokeLine(rect.LeftBottom(), rect.RightBottom());
		StrokeLine(rect.RightTop(), rect.RightBottom());
	
		SetHighColor(light);
		StrokeLine(rect.LeftTop(), rect.RightTop());
		StrokeLine(rect.LeftTop(), rect.LeftBottom());
	
		rect.InsetBy(1,1);
		if (fPrivateData->fTracking)
			SetHighColor(tint_color(normal, B_DARKEN_1_TINT));
		else
			SetHighColor(normal);
		
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
	// TODO: Implement
	return NULL;
}


void
BScrollBar::ResizeToPreferred()
{
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
}


void
BScrollBar::AllDetached()
{
}


status_t
BScrollBar::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
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

/*
	scroll_by_value: increment or decrement scrollbar's value by a certain amount.
	
	Returns B_OK if everthing went well and the caller is to redraw the scrollbar,
			B_ERROR if the caller doesn't need to do anything, or
			B_BAD_VALUE if data is NULL.
*/
status_t scroll_by_value(float valueByWhichToScroll, BScrollBar *bar)
{
	if(!bar)
		return B_BAD_VALUE;

	if(valueByWhichToScroll==0)
		return B_ERROR;
	
	if(bar->fOrientation==B_VERTICAL)
	{
		if(valueByWhichToScroll<0)
		{
			if(bar->fValue + valueByWhichToScroll >= bar->fMin)
			{
				bar->fValue += valueByWhichToScroll;
				if(bar->fTarget)
					bar->fTarget->ScrollBy(0,valueByWhichToScroll);
				bar->fPrivateData->fThumbFrame.OffsetBy(0,valueByWhichToScroll);
				bar->fValue--;
				return B_OK;
			}
			// fall through to return B_ERROR
		}
		else
		{
			if(bar->fValue + valueByWhichToScroll <= bar->fMax)
			{
				bar->fValue += valueByWhichToScroll;
				if(bar->fTarget)
					bar->fTarget->ScrollBy(0,valueByWhichToScroll);
				bar->fPrivateData->fThumbFrame.OffsetBy(0,valueByWhichToScroll);
				bar->fValue++;
				return B_OK;
			}
			// fall through to return B_ERROR
		}
	}
	else
	{
		if(valueByWhichToScroll<0)
		{
			if(bar->fValue + valueByWhichToScroll >= bar->fMin)
			{
				bar->fValue += valueByWhichToScroll;
				if(bar->fTarget)
					bar->fTarget->ScrollBy(valueByWhichToScroll,0);
				bar->fPrivateData->fThumbFrame.OffsetBy(valueByWhichToScroll,0);
				bar->fValue--;
				return B_OK;
			}
			// fall through to return B_ERROR
		}
		else
		{
			if(bar->fValue + valueByWhichToScroll <= bar->fMax)
			{
				bar->fValue += valueByWhichToScroll;
				if(bar->fTarget)
					bar->fTarget->ScrollBy(valueByWhichToScroll,0);
				bar->fPrivateData->fThumbFrame.OffsetBy(valueByWhichToScroll,0);
				bar->fValue++;
				return B_OK;
			}
			// fall through to return B_ERROR
		}
	}
	return B_ERROR;
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
		
		int8 multiplier = (info->double_arrows) ? 1 : -1;
		
		if (bar->fOrientation == B_VERTICAL)
			bar->fPrivateData->fThumbFrame.OffsetBy(0, multiplier * B_H_SCROLL_BAR_HEIGHT);
		else
			bar->fPrivateData->fThumbFrame.OffsetBy(multiplier * B_V_SCROLL_BAR_WIDTH, 0);
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


void
BScrollBar::Private::DrawScrollBarButton(BScrollBar *owner, arrow_direction direction, 
	const BPoint &offset, bool down)
{
	// Another hack for code size

	BRect r(offset.x, offset.y, offset.x + 14,offset.y + 14);
	
	rgb_color c = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light, dark, normal,arrow,arrow2;

	if (down) {	
		light = tint_color(c, B_DARKEN_3_TINT);
		arrow2 = dark = tint_color(c, B_LIGHTEN_MAX_TINT);
		normal = c;
		arrow = tint_color(c, B_DARKEN_MAX_TINT);
	
	} else {
		bool use_enabled_colors = fEnabled;
		
		// Add a usability perk - disable buttons if they would not do anything - 
		// like a left arrow if the value==fMin
		if ((direction == ARROW_LEFT || direction == ARROW_UP) && 
					(owner->fValue == owner->fMin) )
			use_enabled_colors = false;
		else if ((direction == ARROW_RIGHT || direction == ARROW_DOWN) && 
					(owner->fValue == owner->fMax) )
			use_enabled_colors = false;
		

		if (use_enabled_colors) {
			arrow2 = light = tint_color(c, B_LIGHTEN_MAX_TINT);
			dark = tint_color(c, B_DARKEN_3_TINT);
			normal = c;
			arrow = tint_color(c, B_DARKEN_MAX_TINT);
		} else {
			arrow2 = light = tint_color(c, B_LIGHTEN_1_TINT);
			dark = tint_color(c, B_DARKEN_1_TINT);
			normal = c;
			arrow = tint_color(c, B_DARKEN_1_TINT);
		}
	}
	
	BPoint tri1, tri2, tri3;
	
	switch (direction) {
		case ARROW_LEFT:
		{
			tri1.Set(r.left + 3, (r.top + r.bottom) /2 );
			tri2.Set(r.right - 3, r.top + 3);
			tri3.Set(r.right - 3, r.bottom - 3);
			break;
		}
		case ARROW_RIGHT:
		{
			tri1.Set(r.left + 3, r.bottom - 3);
			tri2.Set(r.left + 3, r.top + 3);
			tri3.Set(r.right-3, (r.top + r.bottom) / 2);
			break;
		}
		case ARROW_UP:
		{
			tri1.Set(r.left + 3, r.bottom - 3);
			tri2.Set((r.left + r.right) / 2, r.top + 3);
			tri3.Set(r.right - 3, r.bottom - 3);
			break;
		}
		default:
		{
			tri1.Set(r.left + 3, r.top + 3);
			tri2.Set(r.right - 3, r.top + 3);
			tri3.Set((r.left + r.right) / 2, r.bottom - 3);
			break;
		}
	}

	r.InsetBy(1, 1);
	owner->SetHighColor(normal);
	owner->FillRect(r);
	
	owner->SetHighColor(arrow);
	owner->FillTriangle(tri1, tri2, tri3);

	r.InsetBy(-1, -1);
	owner->SetHighColor(dark);
	owner->StrokeLine(r.LeftBottom(), r.RightBottom());
	owner->StrokeLine(r.RightTop(), r.RightBottom());
	owner->StrokeLine(tri2, tri3);
	owner->StrokeLine(tri1, tri3);

	owner->SetHighColor(light);
	owner->StrokeLine(r.LeftTop(), r.RightTop());
	owner->StrokeLine(r.LeftTop(), r.LeftBottom());

	owner->SetHighColor(arrow2);
	owner->StrokeLine(tri1, tri2);
}
