/*
	
	cl_view.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef	_CL_VIEW_H_
#define _CL_VIEW_H_

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif

#include <time.h>

class TOffscreenView : public BView {

public:
				TOffscreenView(BRect frame, char *name, short mRadius,
						short hRadius, short offset, long face, bool show); 
virtual			~TOffscreenView();
virtual	void	AttachedToWindow();
virtual	void	DrawX();
		void	NextFace();

BBitmap	*fClockFace[9];
BBitmap	*fCenter;
BBitmap	*fInner;
short			fFace;
BPoint			fMinutePoints[60];
BPoint			fHourPoints[60];
short			fMinutesRadius;
short			fHoursRadius;
short			fOffset;
short			fHours;
short			fMinutes;
short			fSeconds;
bool			fShowSeconds;
};


class _EXPORT TOnscreenView : public BView {

public:
				TOnscreenView(BRect frame, char *name, short mRadius,
						short hRadius, short offset); 
virtual			~TOnscreenView();
						TOnscreenView(BMessage *data);
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;
		void			InitObject(BRect frame, short mRadius, short hRadius,
								short offset, long face, bool show);

//+virtual	void	AllAttached();
virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
virtual void	MouseDown( BPoint point);
virtual	void	MessageReceived(BMessage *msg);
virtual void	Pulse();
		void	UseFace( short face );
		void	ShowSecs( bool secs );
		short	ReturnFace( void );
		short	ReturnSeconds( void );


private:

	typedef	BView inherited;
		
BBitmap	*Offscreen;
TOffscreenView	*OffscreenView;
short	fmRadius;
short	fhRadius;
short	fOffset;
BRect	fRect;
};

#endif
