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
//	File Name:		ScrollBar.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Client-side class for scrolling
//
//------------------------------------------------------------------------------
#ifndef	_SCROLL_BAR_H
#define	_SCROLL_BAR_H

#include <BeBuild.h>
#include <View.h>

//----------------------------------------------------------------
//----- scroll bar defines ---------------------------------------

#define B_V_SCROLL_BAR_WIDTH	14.0
#define B_H_SCROLL_BAR_HEIGHT	14.0

#define SCROLL_BAR_MAXIMUM_KNOB_SIZE	50
#define SCROLL_BAR_MINIMUM_KNOB_SIZE	9

//----------------------------------------------------------------
//----- BScrollBar class -----------------------------------------

class BScrollArrowButton;
class BScrollBarPrivateData;

class BScrollBar : public BView {

public:
					BScrollBar(	BRect frame,
								const char *name,
								BView *target,
								float min,
								float max,
								orientation direction);
					BScrollBar(BMessage *data);
virtual				~BScrollBar();
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;
			
virtual	void		AttachedToWindow();
		void		SetValue(float value);
		float		Value() const;
		void		SetProportion(float);
		float		Proportion() const;
virtual	void		ValueChanged(float newValue);

		void		SetRange(float min, float max);
		void		GetRange(float *min, float *max) const;
		void		SetSteps(float smallStep, float largeStep);
		void		GetSteps(float *smallStep, float *largeStep) const;
		void		SetTarget(BView *target);
		void		SetTarget(const char *targetName);
		BView		*Target() const;
		orientation	Orientation() const;

virtual void		MessageReceived(BMessage *msg);
virtual	void		MouseDown(BPoint pt);
virtual	void		MouseUp(BPoint pt);
virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void		DetachedFromWindow();
virtual	void		Draw(BRect updateRect);
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);

virtual BHandler	*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);

virtual void		ResizeToPreferred();
virtual void		GetPreferredSize(float *width, float *height);
virtual void		MakeFocus(bool state = true);
virtual void		AllAttached();
virtual void		AllDetached();
virtual status_t	GetSupportedSuites(BMessage *data);


//----- Private or reserved -----------------------------------------
virtual status_t	Perform(perform_code d, void *arg);

private:
		friend class BScrollArrowButton;
		friend class BScrollBarPrivateData;
		friend status_t control_scrollbar(scroll_bar_info *info, BScrollBar *bar);		// for use within the preflet
		friend status_t scroll_by_value(float valueByWhichToScroll, BScrollBar *bar);	// for use here within the IK
virtual	void		_ReservedScrollBar1();
virtual	void		_ReservedScrollBar2();
virtual	void		_ReservedScrollBar3();
virtual	void		_ReservedScrollBar4();

		BScrollBar	&operator=(const BScrollBar &);
		void 		DoScroll(float delta);

		void		InitObject(float min, float max, orientation o, BView *t);
		float		fMin;
		float		fMax;
		float		fSmallStep;
		float		fLargeStep;
		float		fValue;
		float		fProportion;
		BView*		fTarget;	
		orientation	fOrientation;
		char		*fTargetName;

		BScrollBarPrivateData *privatedata;

		uint32		_reserved[3];
};

//-------------------------------------------------------------
//-------------------------------------------------------------

#endif // _SCROLL_BAR_H
