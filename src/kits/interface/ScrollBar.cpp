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

class BScrollArrowButton : public BView
{
public:
	BScrollArrowButton(BPoint location, const char *name, 
		arrow_direction dir);
	~BScrollArrowButton(void);
	void AttachedToWindow(void);
	void DetachedToWindow(void);
	void MouseDown(BPoint pt);
	void MouseUp(BPoint pt);
	void MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
	void Draw(BRect update);
	void SetEnabled(bool value);
private:
	arrow_direction fDirection;
	BPoint tri1,tri2,tri3;
	BScrollBar *parent;
	bool mousedown;
	bool enabled;
};

class BScrollBarPrivateData
{
public:
	BScrollBarPrivateData(void)
	{
		thumbframe.Set(0,0,0,0);
		enabled=true;
		tracking=false;
		mousept.Set(0,0);
		thumbinc=1.0;
		repeaterid=-1;
		exit_repeater=false;
		arrowdown=ARROW_NONE;
		
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
			ResizeTo(B_V_SCROLL_BAR_WIDTH,frame.Height());
		privatedata->thumbframe=Bounds().InsetByCopy(0,1);

		BPoint buttonpt(1,1);
		BScrollArrowButton *arrow;

		arrow=new BScrollArrowButton(buttonpt,"up",ARROW_UP);
		AddChild(arrow);

		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.y+=arrow->Bounds().Height();
			arrow=new BScrollArrowButton(BPoint(1,
				B_V_SCROLL_BAR_WIDTH+2),"down2",ARROW_DOWN);
			AddChild(arrow);
		}
		
		buttonpt.y=Bounds().bottom-B_H_SCROLL_BAR_HEIGHT-1;
		arrow=new BScrollArrowButton(buttonpt,"down",ARROW_DOWN);
		AddChild(arrow);

		privatedata->thumbframe.bottom=privatedata->sbinfo.min_knob_size;
		privatedata->thumbframe.right=privatedata->sbinfo.min_knob_size;
		privatedata->thumbframe.OffsetBy(0,arrow->Bounds().Height()+1);

		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.y-=arrow->Bounds().Height()+1;
			arrow=new BScrollArrowButton(buttonpt,"up2",ARROW_UP);
			AddChild(arrow);

			privatedata->thumbframe.OffsetBy(0,arrow->Bounds().Height()+1);
		}
	}
	else
	{
		if(frame.Height()>B_H_SCROLL_BAR_HEIGHT)
			ResizeTo(frame.Width()+1,B_H_SCROLL_BAR_HEIGHT);
		privatedata->thumbframe=Bounds().InsetByCopy(1,0);

		BPoint buttonpt(1,1);
		BScrollArrowButton *arrow;

		arrow=new BScrollArrowButton(buttonpt,"left",ARROW_LEFT);
		AddChild(arrow);

		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.x+=arrow->Bounds().Width();
			arrow=new BScrollArrowButton(buttonpt,"right2",ARROW_RIGHT);
			AddChild(arrow);
 		}

		privatedata->thumbframe.right=privatedata->sbinfo.min_knob_size;
		privatedata->thumbframe.OffsetBy(arrow->Bounds().Width()+1,0);

		arrow=new BScrollArrowButton(BPoint(Bounds().right-B_V_SCROLL_BAR_WIDTH+1,1),
			"right",ARROW_RIGHT);
		AddChild(arrow);

		if(privatedata->sbinfo.double_arrows)
		{
			buttonpt.x-=arrow->Bounds().Width()+1;
			arrow=new BScrollArrowButton(buttonpt,"left2",ARROW_LEFT);
			AddChild(arrow);

			privatedata->thumbframe.OffsetBy(arrow->Bounds().Width()+1,0);
		}
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
	if(min==0 && max==0 && privatedata->enabled)
	{
		BScrollArrowButton *arrow;
		int32 i=0;
		arrow=(BScrollArrowButton *)ChildAt(i++);
		while(arrow)
		{
			arrow->SetEnabled(false);
			arrow=(BScrollArrowButton *)ChildAt(i++);
		}
		privatedata->enabled=false;
	}
	else
	if(fMin==0 && fMax==0 && privatedata->enabled==false)
	{
		BScrollArrowButton *arrow;
		int32 i=0;
		arrow=(BScrollArrowButton *)ChildAt(i++);
		while(arrow)
		{
			arrow->SetEnabled(true);
			arrow=(BScrollArrowButton *)ChildAt(i++);
		}
		privatedata->enabled=true;
	}

	fMin=min;
	fMax=max;
	
	if(fValue>fMax)
		fValue=fMax;
	else
	if(fValue<fMin)
		fValue=fMin;
	
	
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
	if(privatedata->enabled)
	{
		if(privatedata->thumbframe.Contains(pt))
		{
			privatedata->tracking=true;
			privatedata->mousept=pt;
			SetMouseEventMask(0,B_LOCK_WINDOW_FOCUS);
			Draw(privatedata->thumbframe);
		}
		else
		{
			// figure out which way to scroll by fLargeStep
			if(fOrientation==B_VERTICAL)
			{
				if(pt.y<privatedata->thumbframe.top)
					DoScroll(-fLargeStep);
				else
					DoScroll(fLargeStep);
			}
			else
			{
				if(pt.x<privatedata->thumbframe.left)
					DoScroll(-fLargeStep);
				else
					DoScroll(fLargeStep);
			}
			ValueChanged(fValue);
			Draw(Bounds());
		}
	}
}

void BScrollBar::MouseUp(BPoint pt)
{
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
	
	
	SetHighColor(normal);
	FillRect(updateRect);
	
	SetHighColor(dark);
	StrokeRect(Bounds());
	
	// Draw scroll thumb
	BRect r(privatedata->thumbframe);
	StrokeRect(privatedata->thumbframe);

	r.InsetBy(1,1);
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

BScrollArrowButton::BScrollArrowButton(BPoint location, 
	const char *name, arrow_direction dir)
 :BView(BRect(0,0,B_V_SCROLL_BAR_WIDTH-1,B_H_SCROLL_BAR_HEIGHT-1).OffsetToCopy(location),name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fDirection=dir;
	enabled=true;
	BRect r=Bounds();
	
	switch(fDirection)
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
	mousedown=false;
}

BScrollArrowButton::~BScrollArrowButton(void)
{
}

void BScrollArrowButton::MouseDown(BPoint pt)
{
	if(enabled==false)
		return;
	
	mousedown=true;
	Draw(Bounds());

	// Test for which button clicked here
	status_t returnval=B_ERROR;

	if(parent)
	{
		parent->privatedata->arrowdown=fDirection;
		returnval=control_scrollbar(SBC_SCROLLBYVALUE,&parent->fSmallStep,parent);

		if(returnval==B_OK)
		{
			parent->Draw(parent->Bounds());
			parent->ValueChanged(parent->fValue);
			
			if(parent->privatedata->repeaterid==-1)
			{
				parent->privatedata->exit_repeater=false;
				parent->privatedata->repeaterid=spawn_thread(parent->privatedata->ButtonRepeaterThread,
					"scroll repeater",B_NORMAL_PRIORITY,parent);
				resume_thread(parent->privatedata->repeaterid);
			}
		}
	}
}

void BScrollArrowButton::MouseUp(BPoint pt)
{
	if(enabled)
	{
		mousedown=false;
		
		if(parent)
		{
			parent->privatedata->arrowdown=ARROW_NONE;
			parent->privatedata->exit_repeater=true;
		}
		Draw(Bounds());
	}
}

void BScrollArrowButton::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if(enabled==false)
		return;
	
	if(transit==B_ENTERED_VIEW)
	{
		BPoint point;
		uint32 buttons;
		GetMouse(&point,&buttons);
		if( (buttons & B_PRIMARY_MOUSE_BUTTON)==0 &&
				(buttons & B_SECONDARY_MOUSE_BUTTON)==0 &&
				(buttons & B_PRIMARY_MOUSE_BUTTON)==0 )
			mousedown=false;
		else
			mousedown=true;
		Draw(Bounds());
	}
	
	if(transit==B_EXITED_VIEW || transit==B_OUTSIDE_VIEW)
		MouseUp(Bounds().LeftTop());
}

void BScrollArrowButton::Draw(BRect update)
{
	rgb_color c=ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect r(Bounds());
	
	rgb_color light, dark, normal,arrow,arrow2;
	if(mousedown)
	{	
		light=tint_color(c,B_DARKEN_3_TINT);
		arrow2=dark=tint_color(c,B_LIGHTEN_MAX_TINT);
		normal=c;
		arrow=tint_color(c,B_DARKEN_MAX_TINT);
	}
	else
	if(enabled)
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

	r.InsetBy(1,1);
	SetHighColor(normal);
	FillRect(r);
	
	SetHighColor(arrow);
	FillTriangle(tri1,tri2,tri3);

	r.InsetBy(-1,-1);
	SetHighColor(dark);
	StrokeLine(r.LeftBottom(),r.RightBottom());
	StrokeLine(r.RightTop(),r.RightBottom());
	StrokeLine(tri2,tri3);
	StrokeLine(tri1,tri3);

	SetHighColor(light);
	StrokeLine(r.LeftTop(),r.RightTop());
	StrokeLine(r.LeftTop(),r.LeftBottom());

	SetHighColor(arrow2);
	StrokeLine(tri1,tri2);

}

void BScrollArrowButton::AttachedToWindow(void)
{
	parent=(BScrollBar*)Parent();
}

void BScrollArrowButton::DetachedToWindow(void)
{
	parent=NULL;
}

void BScrollArrowButton::SetEnabled(bool value)
{
	enabled=value;
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
	
*/
status_t control_scrollbar(int8 command, void *data, BScrollBar *sb)
{
	if(!sb)
		return B_BAD_VALUE;
	
	switch(command)
	{
		case 0: 
		{
			if(!data)
				return B_BAD_VALUE;
			
			float datavalue=*((float *)data);
			if(datavalue==0)
				return B_ERROR;
			
/*			if(sb->privatedata->arrowdown==ARROW_LEFT || sb->privatedata->arrowdown==ARROW_UP)
			{
				if(sb->fValue - datavalue >= sb->fMin)
				{
					sb->fValue -= datavalue;
					if(sb->privatedata->arrowdown==ARROW_UP)
					{
						if(sb->fTarget)
							sb->fTarget->ScrollBy(0,-datavalue);
						sb->privatedata->thumbframe.OffsetBy(0,-datavalue);
					}
					else
					{
						if(sb->fTarget)
							sb->fTarget->ScrollBy(-datavalue,0);
						sb->privatedata->thumbframe.OffsetBy(-datavalue,0);
					}
					sb->fValue--;
					return B_OK;
				}
			}
			else
			{
				if(sb->fValue + datavalue <= sb->fMax)
				{
					sb->fValue += datavalue;
					if(sb->privatedata->arrowdown==ARROW_DOWN)
					{
						if(sb->fTarget)
							sb->fTarget->ScrollBy(0,datavalue);
						sb->privatedata->thumbframe.OffsetBy(0,datavalue);
					}
					else
					{
						if(sb->fTarget)
							sb->fTarget->ScrollBy(datavalue,0);
						sb->privatedata->thumbframe.OffsetBy(datavalue,0);
					}
					sb->fValue++;
					return B_OK;
				}
			}
*/
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
		
		default:
		{
			printf("Unknown command value %d in control_scrollbar\n",command);
			break;
		}
	} // end command switch
	
	
	// We should never get here unless I've done something dumb in calling the function
	return B_BAD_VALUE;
}

