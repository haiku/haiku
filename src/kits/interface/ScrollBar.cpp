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
//	File Name:		ScrollBar.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Client-side class for scrolling
//
//------------------------------------------------------------------------------

#include <Message.h>
#include <Rect.h>
#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <Window.h>
#include "ScrollBar.h"

#define TEST_MODE

typedef enum
{
	ARROW_LEFT=0,
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

class BScrollBarPrivateData
{
public:
	BScrollBarPrivateData(void)
	{
		thumbframe.Set(0,0,B_V_SCROLL_BAR_WIDTH,B_H_SCROLL_BAR_HEIGHT);
		enabled=true;
		tracking=false;
		mousept.Set(0,0);
		thumbinc=1.0;
		repeaterid=-1;
		exit_repeater=false;
		arrowdown=ARROW_NONE;
		buttondown=NOARROW;
		
		#ifdef TEST_MODE
			sbinfo.proportional=true;
			sbinfo.double_arrows=false;
			sbinfo.knob=0;
			sbinfo.min_knob_size=14;
		#else
			get_scroll_bar_info(&sbinfo);
		#endif
	}
	
	~BScrollBarPrivateData(void)
	{
		if(repeaterid!=-1)
		{
			exit_repeater=false;
			kill_thread(repeaterid);
		}
	}
	void DrawScrollBarButton(BScrollBar *owner, arrow_direction direction, 
		const BPoint &offset, bool down=false);
	static int32 ButtonRepeaterThread(void *data);
	
	
	thread_id repeaterid;
	scroll_bar_info sbinfo;
	BRect thumbframe;
	bool enabled;
	bool tracking;
	BPoint mousept;
	float thumbinc;
	bool exit_repeater;
	arrow_direction arrowdown;
	int8 buttondown;
};

int32 BScrollBarPrivateData::ButtonRepeaterThread(void *data)
{
	BScrollBar *sb=(BScrollBar *)data;
	BRect oldframe(sb->privatedata->thumbframe);
//	BRect update(sb->privatedata->thumbframe);
	
	snooze(250000);
	
	bool exitval=false;
	status_t returnval;
	
	sb->Window()->Lock();
	exitval=sb->privatedata->exit_repeater;
	sb->Window()->Unlock();
	
	float scrollvalue;

	if(sb->privatedata->arrowdown==ARROW_LEFT || sb->privatedata->arrowdown==ARROW_UP)
		scrollvalue=-sb->fSmallStep;
	else
	if(sb->privatedata->arrowdown!=ARROW_NONE)
		scrollvalue=sb->fSmallStep;
	else
		exitval=true;
	
	while(!exitval)
	{
		oldframe=sb->privatedata->thumbframe;
		
		returnval=control_scrollbar(SBC_SCROLLBYVALUE, &scrollvalue, sb);
		
		snooze(50000);
		
		sb->Window()->Lock();
		exitval=sb->privatedata->exit_repeater;

		if(returnval==B_OK)
		{
			sb->CopyBits(oldframe,sb->privatedata->thumbframe);

			// TODO: Redraw the old area here
			
			sb->ValueChanged(sb->fValue);
		}
		sb->Window()->Unlock();
	}

	sb->Window()->Lock();
	sb->privatedata->exit_repeater=false;
	sb->privatedata->repeaterid=-1;
	sb->Window()->Unlock();
	return 0;
	exit_thread(0);
}

BScrollBar::BScrollBar(BRect frame,const char *name,BView *target,float min,
		float max,orientation direction)
 : BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fMin=min;
	fMax=max;
	fOrientation=direction;
	fValue=0;
	fSmallStep=1.0;
	fLargeStep=10.0;
	fTarget=target;
	privatedata=new BScrollBarPrivateData;

	if(fTarget)
	{
		fTargetName=new char[strlen(fTarget->Name()+1)];
		strcpy(fTargetName,target->Name());

		// theoretically, we should also set the target BView's scrollbar
		// pointer here
	}
	else
		fTargetName=NULL;
	
	if(direction==B_VERTICAL)
	{
		if(frame.Width()>B_V_SCROLL_BAR_WIDTH)
			ResizeTo(B_V_SCROLL_BAR_WIDTH,frame.Height()+1);

		privatedata->thumbframe.bottom=privatedata->sbinfo.min_knob_size;
		if(privatedata->sbinfo.double_arrows)
			privatedata->thumbframe.OffsetBy(0,(B_H_SCROLL_BAR_HEIGHT+1)*2);
		else
			privatedata->thumbframe.OffsetBy(0,B_H_SCROLL_BAR_HEIGHT+1);
	}
	else
	{
		if(frame.Height()>B_H_SCROLL_BAR_HEIGHT)
			ResizeTo(frame.Width()+1,B_H_SCROLL_BAR_HEIGHT);

		privatedata->thumbframe.right=privatedata->sbinfo.min_knob_size;
		if(privatedata->sbinfo.double_arrows)
			privatedata->thumbframe.OffsetBy((B_V_SCROLL_BAR_WIDTH+1)*2,0);
		else
			privatedata->thumbframe.OffsetBy(B_V_SCROLL_BAR_WIDTH+1,0);
	}

	SetResizingMode( (direction==B_VERTICAL)?
		B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT : 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
	
}

BScrollBar::BScrollBar(BMessage *data)
 : BView(data)
{
}

BScrollBar::~BScrollBar()
{
	delete privatedata;
	if(fTargetName)
		delete fTargetName;
}

BArchivable *BScrollBar::Instantiate(BMessage *data)
{
	return NULL;
}

status_t BScrollBar::Archive(BMessage *data, bool deep) const
{
	return B_ERROR;
}

void BScrollBar::AttachedToWindow()
{
	// if fValue!=0, BScrollbars tell the server its value
}

void BScrollBar::SetValue(float value)
{
	fValue=value;
	ValueChanged(fValue);
}

float BScrollBar::Value() const
{
	return fValue;
}

void BScrollBar::SetProportion(float value)
{
	fProportion=value;
}

float BScrollBar::Proportion() const
{
	return fProportion;
}

void BScrollBar::ValueChanged(float newValue)
{
}

void BScrollBar::SetRange(float min, float max)
{
	fMin=min;
	fMax=max;
	
	if(fValue>fMax)
		fValue=fMax;
	else
	if(fValue<fMin)
		fValue=fMin;
	
	Draw(Bounds());
	
	// Just a sort-of hack for now. ValueChanged is called, but with
	// what value??
	ValueChanged(fValue);
}

void BScrollBar::GetRange(float *min, float *max) const
{
	*min=fMin;
	*max=fMax;
}

void BScrollBar::SetSteps(float smallStep, float largeStep)
{
	// Under R5, steps can be set only after being attached to a window
	// We'll just remove that limitation... :P
	
	fSmallStep=smallStep;
	fLargeStep=largeStep;
}

void BScrollBar::GetSteps(float *smallStep, float *largeStep) const
{
	*smallStep=fSmallStep;
	*largeStep=fLargeStep;
}

void BScrollBar::SetTarget(BView *target)
{
	fTarget=target;
	if(fTargetName)
		delete fTargetName;
	if(fTarget)
	{
		fTargetName=new char[strlen(target->Name())+1];
		strcpy(fTargetName,target->Name());
		
		// theoretically, we should also set the target BView's scrollbar
		// pointer here
	}
	else
		fTargetName=NULL;
}

void BScrollBar::SetTarget(const char *targetName)
{
}

BView *BScrollBar::Target() const
{
	return fTarget;
}

orientation BScrollBar::Orientation() const
{
	return fOrientation;
}

void BScrollBar::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_VALUE_CHANGED:
		{
			int32 value;
			if(msg->FindInt32("value",&value)==B_OK)
				ValueChanged(value);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void BScrollBar::MouseDown(BPoint pt)
{
	if(!(fMin==0 && fMax==0))	// if enabled
	{
		// Hit test for thumb
		if(privatedata->thumbframe.Contains(pt))
		{
			privatedata->tracking=true;
			privatedata->mousept=pt;
			SetMouseEventMask(0,B_LOCK_WINDOW_FOCUS);
			Draw(privatedata->thumbframe);
			return;
		}
		
		BRect buttonrect(0,0,B_V_SCROLL_BAR_WIDTH,B_H_SCROLL_BAR_HEIGHT);
		float scrollval=0;
		status_t returnval=B_ERROR;
		
		
		// Hit test for arrow buttons
		if(fOrientation==B_VERTICAL)
		{
			if(buttonrect.Contains(pt))
			{
				scrollval= -fSmallStep;
				privatedata->arrowdown=ARROW_UP;
				privatedata->buttondown=ARROW1;
				returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
		
				if(returnval==B_OK)
				{
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(privatedata->repeaterid==-1)
					{
						privatedata->exit_repeater=false;
						privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
							"scroll repeater",B_NORMAL_PRIORITY,this);
						resume_thread(privatedata->repeaterid);
					}
				}
				return;

			}

			buttonrect.OffsetTo(0,Bounds().Height() - (B_H_SCROLL_BAR_HEIGHT));
			if(buttonrect.Contains(pt))
			{
				scrollval= fSmallStep;
				privatedata->arrowdown=ARROW_DOWN;
				privatedata->buttondown=ARROW4;
				returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
		
				if(returnval==B_OK)
				{
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(privatedata->repeaterid==-1)
					{
						privatedata->exit_repeater=false;
						privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
							"scroll repeater",B_NORMAL_PRIORITY,this);
						resume_thread(privatedata->repeaterid);
					}
				}
				return;
			}

			if(privatedata->sbinfo.double_arrows)
			{
				buttonrect.OffsetTo(0,B_H_SCROLL_BAR_HEIGHT+1);
				if(buttonrect.Contains(pt))
				{
					scrollval= fSmallStep;
					privatedata->arrowdown=ARROW_DOWN;
					privatedata->buttondown=ARROW2;
					returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			
					if(returnval==B_OK)
					{
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if(privatedata->repeaterid==-1)
						{
							privatedata->exit_repeater=false;
							privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
								"scroll repeater",B_NORMAL_PRIORITY,this);
							resume_thread(privatedata->repeaterid);
						}
					}
					return;
				}

				buttonrect.OffsetTo(0,Bounds().Height()-( (B_H_SCROLL_BAR_HEIGHT*2)+1 ));
				if(buttonrect.Contains(pt))
				{
					scrollval= -fSmallStep;
					privatedata->arrowdown=ARROW_UP;
					privatedata->buttondown=ARROW3;
					returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			
					if(returnval==B_OK)
					{
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if(privatedata->repeaterid==-1)
						{
							privatedata->exit_repeater=false;
							privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
								"scroll repeater",B_NORMAL_PRIORITY,this);
							resume_thread(privatedata->repeaterid);
						}
					}
					return;
	
				}
			}
			
			// TODO: add a repeater thread for large stepping and a call to it

			if(pt.y<privatedata->thumbframe.top)
			{
				scrollval=-fLargeStep;
				control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			}
			else
			{
				scrollval=fLargeStep;
				control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			}
		}
		else
		{
			if(buttonrect.Contains(pt))
			{
				scrollval= -fSmallStep;
				privatedata->arrowdown=ARROW_LEFT;
				privatedata->buttondown=ARROW1;
				returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
		
				if(returnval==B_OK)
				{
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(privatedata->repeaterid==-1)
					{
						privatedata->exit_repeater=false;
						privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
							"scroll repeater",B_NORMAL_PRIORITY,this);
						resume_thread(privatedata->repeaterid);
					}
				}
				return;
			}
			
			buttonrect.OffsetTo(Bounds().Width() - (B_V_SCROLL_BAR_WIDTH),0);
			if(buttonrect.Contains(pt))
			{
				scrollval= fSmallStep;
				privatedata->arrowdown=ARROW_RIGHT;
				privatedata->buttondown=ARROW4;
				returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
		
				if(returnval==B_OK)
				{
					Draw(buttonrect);
					ValueChanged(fValue);
					
					if(privatedata->repeaterid==-1)
					{
						privatedata->exit_repeater=false;
						privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
							"scroll repeater",B_NORMAL_PRIORITY,this);
						resume_thread(privatedata->repeaterid);
					}
				}
				return;
			}
			
			if(privatedata->sbinfo.proportional)
			{
				buttonrect.OffsetTo(B_V_SCROLL_BAR_WIDTH+1,0);
				if(buttonrect.Contains(pt))
				{
					scrollval= fSmallStep;
					privatedata->buttondown=ARROW2;
					privatedata->arrowdown=ARROW_LEFT;
					returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
					
					if(returnval==B_OK)
					{
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if(privatedata->repeaterid==-1)
						{
							privatedata->exit_repeater=false;
							privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
								"scroll repeater",B_NORMAL_PRIORITY,this);
							resume_thread(privatedata->repeaterid);
						}
					}
					return;
				}
				
				buttonrect.OffsetTo(Bounds().Width()-( (B_V_SCROLL_BAR_WIDTH*2)+1 ),0);
				if(buttonrect.Contains(pt))
				{
					scrollval= -fSmallStep;
					privatedata->buttondown=ARROW3;
					privatedata->arrowdown=ARROW_RIGHT;
					returnval=control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
				
					if(returnval==B_OK)
					{
						Draw(buttonrect);
						ValueChanged(fValue);
						
						if(privatedata->repeaterid==-1)
						{
							privatedata->exit_repeater=false;
							privatedata->repeaterid=spawn_thread(privatedata->ButtonRepeaterThread,
								"scroll repeater",B_NORMAL_PRIORITY,this);
							resume_thread(privatedata->repeaterid);
						}
					}
					return;
				}
			}
			
			// We got this far, so apparently the user has clicked on something
			// that isn't the thumb or a scroll button, so scroll by a large step
			
			// TODO: add a repeater thread for large stepping and a call to it


			if(pt.x<privatedata->thumbframe.left)
			{
				scrollval=-fLargeStep;
				control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			}
			else
			{
				scrollval=fLargeStep;
				control_scrollbar(SBC_SCROLLBYVALUE,&scrollval,this);
			}
		}
		ValueChanged(fValue);
		Draw(Bounds());
	}
}

void BScrollBar::MouseUp(BPoint pt)
{
	privatedata->arrowdown=ARROW_NONE;\
	privatedata->buttondown=NOARROW;
	privatedata->exit_repeater=true;
	
	// We'll be lazy here and just draw all the possible arrow regions for now...
	// TODO: optimize
	
	BRect r(0,0,B_V_SCROLL_BAR_WIDTH,B_H_SCROLL_BAR_HEIGHT);
	
	if(fOrientation==B_VERTICAL)
	{
		r.bottom+=B_H_SCROLL_BAR_HEIGHT+1;
		Draw(r);
		r.OffsetTo(0,Bounds().Height()-((B_H_SCROLL_BAR_HEIGHT*2)+1) );
		Draw(r);
	}
	else
	{
		r.bottom+=B_V_SCROLL_BAR_WIDTH+1;
		Draw(r);
		r.OffsetTo(0,Bounds().Height()-((B_V_SCROLL_BAR_WIDTH*2)+1) );
		Draw(r);
	}
	
	if(privatedata->tracking)
	{
		privatedata->tracking=false;
		SetMouseEventMask(0,0);
		Draw(privatedata->thumbframe);
	}
}

void BScrollBar::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if(!privatedata->enabled)
		return;
	
	if(transit==B_EXITED_VIEW || transit==B_OUTSIDE_VIEW)
		MouseUp(privatedata->mousept);

	if(privatedata->tracking)
	{
		float delta;
		if(fOrientation==B_VERTICAL)
		{
			if( (pt.y>privatedata->thumbframe.bottom && fValue==fMax) || 
				(pt.y<privatedata->thumbframe.top && fValue==fMin) )
				return;
			delta=pt.y-privatedata->mousept.y;
		}
		else
		{
			if( (pt.x>privatedata->thumbframe.right && fValue==fMax) || 
				(pt.x<privatedata->thumbframe.left && fValue==fMin) )
				return;
			delta=pt.x-privatedata->mousept.x;
		}
		control_scrollbar(SBC_SCROLLBYVALUE, &delta, this);
		ValueChanged(fValue);
		Draw(Bounds());
		privatedata->mousept=pt;
	}
}

void BScrollBar::DoScroll(float delta)
{
	if(!fTarget)
		return;
	
	float scrollval;

	if(delta>0)
		scrollval=(fValue+delta<=fMax)?delta:(fMax-fValue);
	else
		scrollval=(fValue-delta>=fMin)?delta:(fValue-fMin);

	if(fOrientation==B_VERTICAL)
	{
		fTarget->ScrollBy(0,scrollval);
		privatedata->thumbframe.OffsetBy(0,scrollval);
	}
	else
	{
		fTarget->ScrollBy(scrollval,0);
		privatedata->thumbframe.OffsetBy(scrollval,0);
	}
	fValue+=scrollval;
}

void BScrollBar::DetachedFromWindow()
{
	fTarget=NULL;
	delete fTargetName;
	fTargetName=NULL;
}

void BScrollBar::Draw(BRect updateRect)
{
	rgb_color light, dark,normal,c;
	c=ui_color(B_PANEL_BACKGROUND_COLOR);
	if(privatedata->enabled)
	{
		light=tint_color(c,B_LIGHTEN_MAX_TINT);
		dark=tint_color(c,B_DARKEN_3_TINT);
		normal=c;
	}
	else
	{
		light=tint_color(c,B_LIGHTEN_MAX_TINT);
		dark=tint_color(c,B_DARKEN_3_TINT);
		normal=c;
	}
	
	// Draw main area
	SetHighColor(normal);
	FillRect(updateRect);
	
	SetHighColor(dark);
	StrokeRect(Bounds());
	
	// Draw arrows
	BPoint buttonpt(0,0);
	if(fOrientation==B_HORIZONTAL)
	{
		privatedata->DrawScrollBarButton(this, ARROW_LEFT, buttonpt,
			privatedata->buttondown==ARROW1);
		
		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.Set(B_V_SCROLL_BAR_WIDTH+1,0);
			privatedata->DrawScrollBarButton(this, ARROW_RIGHT, buttonpt, 
				privatedata->buttondown==ARROW2);

			buttonpt.Set(Bounds().Width()-( (B_V_SCROLL_BAR_WIDTH*2)+1 ),0);
			privatedata->DrawScrollBarButton(this, ARROW_LEFT, buttonpt,
				privatedata->buttondown==ARROW3);
		
		}

		buttonpt.Set(Bounds().Width()-(B_V_SCROLL_BAR_WIDTH),0);
		privatedata->DrawScrollBarButton(this, ARROW_RIGHT, buttonpt,
			privatedata->buttondown==ARROW4);
	}
	else
	{
		privatedata->DrawScrollBarButton(this, ARROW_UP, buttonpt,
			privatedata->buttondown==ARROW1);
		
		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.Set(0,B_H_SCROLL_BAR_HEIGHT+1);
			privatedata->DrawScrollBarButton(this, ARROW_DOWN, buttonpt,
				privatedata->buttondown==ARROW2);

			buttonpt.Set(0,Bounds().Height()-( (B_H_SCROLL_BAR_HEIGHT*2)+1 ));
			privatedata->DrawScrollBarButton(this, ARROW_UP, buttonpt,
				privatedata->buttondown==ARROW3);
		
		}

		buttonpt.Set(0,Bounds().Height()-(B_H_SCROLL_BAR_HEIGHT));
		privatedata->DrawScrollBarButton(this, ARROW_DOWN, buttonpt,
			privatedata->buttondown==ARROW4);
	}
	
	// Draw scroll thumb
	
	if(privatedata->enabled)
	{
		BRect r(privatedata->thumbframe);
	
		SetHighColor(dark);
		StrokeRect(privatedata->thumbframe);
	
		r.InsetBy(1,1);
		SetHighColor(tint_color(c,B_DARKEN_2_TINT));
		StrokeLine(r.LeftBottom(),r.RightBottom());
		StrokeLine(r.RightTop(),r.RightBottom());
	
		SetHighColor(light);
		StrokeLine(r.LeftTop(),r.RightTop());
		StrokeLine(r.LeftTop(),r.LeftBottom());
	
		r.InsetBy(1,1);
		if(privatedata->tracking)
			SetHighColor(tint_color(normal,B_DARKEN_1_TINT));
		else
			SetHighColor(normal);
		FillRect(r);
	}
	
	// TODO: Add the other thumb styles - dots and lines
}

void BScrollBar::FrameMoved(BPoint new_position)
{
}

void BScrollBar::FrameResized(float new_width, float new_height)
{
}

BHandler *BScrollBar::ResolveSpecifier(BMessage *msg,int32 index,
		BMessage *specifier,int32 form,const char *property)
{
	return NULL;
}

void BScrollBar::ResizeToPreferred()
{
}

void BScrollBar::GetPreferredSize(float *width, float *height)
{
	if (fOrientation == B_VERTICAL)
		*width = B_V_SCROLL_BAR_WIDTH;
	else if (fOrientation == B_HORIZONTAL)
		*height = B_H_SCROLL_BAR_HEIGHT;
}

void BScrollBar::MakeFocus(bool state)
{
	if(fTarget)
		fTarget->MakeFocus(state);
}

void BScrollBar::AllAttached()
{
}

void BScrollBar::AllDetached()
{
}

status_t BScrollBar::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}

status_t BScrollBar::Perform(perform_code d, void *arg)
{
	return B_OK;
}

void BScrollBar::_ReservedScrollBar1()
{
}

void BScrollBar::_ReservedScrollBar2()
{
}

void BScrollBar::_ReservedScrollBar3()
{
}

void BScrollBar::_ReservedScrollBar4()
{
}

BScrollBar &BScrollBar::operator=(const BScrollBar &)
{
	return *this;
}

/*
	This cheat function will allow the scrollbar prefs app to act exactly like R5's and
	perform other stuff without mucking around with the virtual tables B_BAD_VALUE is 
	returned when a NULL scrollbar pointer is passed to it

	Command list:

	0:	Scroll By Value:	increment or decrement scrollbar's value by a certain amount.
				data parameter is cast to a float * and used to modify the value.
				Returns B_OK if everthing went well and the caller is to redraw the scrollbar
				B_ERROR is returned if the caller doesn't need to do anything
				B_BAD_VALUE is returned if data is NULL
	1: Set Double:			set usage mode to either single or double.
				data parameter is cast to a bool. true sets it to double, false sets to single
				B_OK is always returned
				B_BAD_VALUE is returned if data is NULL
	2: Set Proportional:	set usage mode to either proportional or fixed thumbsize.
				data parameter is cast to a bool. true sets it to proportional, false sets to fixed
				B_OK is always returned
				B_BAD_VALUE is returned if data is NULL
	3: Set Style:			set thumb style to blank, 3 dots, or 3 lines
				data parameter is cast to an int8. 0 is blank, 1 is dotted, 2 is lined
				B_ERROR is returned for values != 0, 1, or 2
				B_BAD_VALUE is returned if data is NULL
*/
status_t control_scrollbar(int8 command, void *data, BScrollBar *sb)
{
	if(!sb)
		return B_BAD_VALUE;
	
	switch(command)
	{
		case 0: 	// Scroll By Value
		{
			if(!data)
				return B_BAD_VALUE;
			
			float datavalue=*((float *)data);
			if(datavalue==0)
				return B_ERROR;
			
			if(sb->fOrientation==B_VERTICAL)
			{
				if(datavalue<0)
				{
					if(sb->fValue + datavalue >= sb->fMin)
					{
						sb->fValue += datavalue;
						if(sb->fTarget)
							sb->fTarget->ScrollBy(0,datavalue);
						sb->privatedata->thumbframe.OffsetBy(0,datavalue);
						sb->fValue--;
						return B_OK;
					}
					// fall through to return B_ERROR
				}
				else
				{
					if(sb->fValue + datavalue <= sb->fMax)
					{
						sb->fValue += datavalue;
						if(sb->fTarget)
							sb->fTarget->ScrollBy(0,datavalue);
						sb->privatedata->thumbframe.OffsetBy(0,datavalue);
						sb->fValue++;
						return B_OK;
					}
					// fall through to return B_ERROR
				}
			}
			else
			{
				if(datavalue<0)
				{
					if(sb->fValue + datavalue >= sb->fMin)
					{
						sb->fValue += datavalue;
						if(sb->fTarget)
							sb->fTarget->ScrollBy(datavalue,0);
						sb->privatedata->thumbframe.OffsetBy(datavalue,0);
						sb->fValue--;
						return B_OK;
					}
					// fall through to return B_ERROR
				}
				else
				{
					if(sb->fValue + datavalue <= sb->fMax)
					{
						sb->fValue += datavalue;
						if(sb->fTarget)
							sb->fTarget->ScrollBy(datavalue,0);
						sb->privatedata->thumbframe.OffsetBy(datavalue,0);
						sb->fValue++;
						return B_OK;
					}
					// fall through to return B_ERROR
				}
			}
			return B_ERROR;
		}// end case 0 (Scroll By Value)
		
		case 1:		// Set Double
		{
			if(!data)
				return B_BAD_VALUE;

			bool datavalue=*((bool *)data);

			if(sb->privatedata->sbinfo.double_arrows==datavalue)
				return B_OK;

			sb->privatedata->sbinfo.double_arrows=datavalue;
			
			int8 multiplier=(datavalue)?1:-1;
			
			if(sb->fOrientation==B_VERTICAL)
				sb->privatedata->thumbframe.OffsetBy(0,multiplier*B_H_SCROLL_BAR_HEIGHT);
			else
				sb->privatedata->thumbframe.OffsetBy(multiplier*B_V_SCROLL_BAR_WIDTH,0);
			return B_OK;
		}
		
		case 2:		// Set Proportional
		{
			// TODO: Figure out how proportional relates to the size of the thumb
			
			if(!data)
				return B_BAD_VALUE;
			
			bool datavalue=*((bool *)data);
			sb->privatedata->sbinfo.proportional=datavalue;
			return B_OK;
		}
		
		case 3:		// Set Style
		{
			// TODO: Add redraw code to reflect the changes

			if(!data)
				return B_BAD_VALUE;
			
			int8 datavalue=*((int8 *)data);
			
			if(datavalue!=0 && datavalue!=1 && datavalue!=2)
				return B_BAD_VALUE;
			
			sb->privatedata->sbinfo.knob=datavalue;
			return B_OK;
		}
		
		default:
		{
			printf("Unknown command value %d in control_scrollbar\n",command);
			break;
		}
	} // end command switch
	
	
	// We should never get here unless I've done something dumb in calling the function
	return B_BAD_VALUE;
}

void BScrollBarPrivateData::DrawScrollBarButton(BScrollBar *owner, arrow_direction direction, 
	const BPoint &offset, bool down)
{
	// Another hack for code size

	BRect r(offset.x,offset.y,offset.x+14,offset.y+14);
	
	rgb_color c=ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light, dark, normal,arrow,arrow2;

	if(down)
	{	
		light=tint_color(c,B_DARKEN_3_TINT);
		arrow2=dark=tint_color(c,B_LIGHTEN_MAX_TINT);
		normal=c;
		arrow=tint_color(c,B_DARKEN_MAX_TINT);
	}
	else
	{
		bool use_enabled_colors=enabled;
		
		// Add a usability perk - disable buttons if they would not do anything - 
		// like a left arrow if the value==fMin
		if( (direction==ARROW_LEFT || direction==ARROW_UP) && 
					(owner->fValue==owner->fMin) )
			use_enabled_colors=false;
		else
		if( (direction==ARROW_RIGHT || direction==ARROW_DOWN) && 
					(owner->fValue==owner->fMax) )
			use_enabled_colors=false;
		

		if(use_enabled_colors)
		{
			arrow2=light=tint_color(c,B_LIGHTEN_MAX_TINT);
			dark=tint_color(c,B_DARKEN_3_TINT);
			normal=c;
			arrow=tint_color(c,B_DARKEN_MAX_TINT);
		}
		else
		{
			arrow2=light=tint_color(c,B_LIGHTEN_1_TINT);
			dark=tint_color(c,B_DARKEN_1_TINT);
			normal=c;
			arrow=tint_color(c,B_DARKEN_1_TINT);
		}
	}
	
	BPoint tri1,tri2,tri3;
	
	switch(direction)
	{
		case ARROW_LEFT:
		{
			tri1.Set(r.left+3,(r.top+r.bottom)/2);
			tri2.Set(r.right-3,r.top+3);
			tri3.Set(r.right-3,r.bottom-3);
			break;
		}
		case ARROW_RIGHT:
		{
			tri1.Set(r.left+3,r.bottom-3);
			tri2.Set(r.left+3,r.top+3);
			tri3.Set(r.right-3,(r.top+r.bottom)/2);
			break;
		}
		case ARROW_UP:
		{
			tri1.Set(r.left+3,r.bottom-3);
			tri2.Set((r.left+r.right)/2,r.top+3);
			tri3.Set(r.right-3,r.bottom-3);
			break;
		}
		default:
		{
			tri1.Set(r.left+3,r.top+3);
			tri2.Set(r.right-3,r.top+3);
			tri3.Set((r.left+r.right)/2,r.bottom-3);
			break;
		}
	}

	r.InsetBy(1,1);
	owner->SetHighColor(normal);
	owner->FillRect(r);
	
	owner->SetHighColor(arrow);
	owner->FillTriangle(tri1,tri2,tri3);

	r.InsetBy(-1,-1);
	owner->SetHighColor(dark);
	owner->StrokeLine(r.LeftBottom(),r.RightBottom());
	owner->StrokeLine(r.RightTop(),r.RightBottom());
	owner->StrokeLine(tri2,tri3);
	owner->StrokeLine(tri1,tri3);

	owner->SetHighColor(light);
	owner->StrokeLine(r.LeftTop(),r.RightTop());
	owner->StrokeLine(r.LeftTop(),r.LeftBottom());

	owner->SetHighColor(arrow2);
	owner->StrokeLine(tri1,tri2);
}
