/*******************************************************************************
/
/	File:			ScrollBar.h
/
/   Description:    BScrollBar displays and controls a scrollbar. 
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_SCROLL_BAR_H
#define	_SCROLL_BAR_H

#include <BeBuild.h>
#include <View.h>

/*----------------------------------------------------------------*/
/*----- scroll bar defines ---------------------------------------*/

#define B_V_SCROLL_BAR_WIDTH	14.0
#define B_H_SCROLL_BAR_HEIGHT	14.0

/*----------------------------------------------------------------*/
/*----- BScrollBar class -----------------------------------------*/

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


/*----- Private or reserved -----------------------------------------*/
virtual status_t	Perform(perform_code d, void *arg);

private:

friend class BWindow;
friend class BApplication;

virtual	void		_ReservedScrollBar1();
virtual	void		_ReservedScrollBar2();
virtual	void		_ReservedScrollBar3();
virtual	void		_ReservedScrollBar4();

		BScrollBar	&operator=(const BScrollBar &);

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
		uint32		_reserved[4];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SCROLL_BAR_H */
