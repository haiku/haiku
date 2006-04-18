/*
	
	cl_view.cpp
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEBUG 1
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
#include <Dragger.h>
#include <Entry.h>
#include <Resources.h>
#include <Roster.h>

#include "clock.h"
#include "cl_view.h"
/* ---------------------------------------------------------------- */

TOffscreenView::TOffscreenView(BRect frame, char *name, short mRadius,
						short hRadius, short offset, long face, bool show)
	   		   :BView(frame, name, B_NOT_RESIZABLE, B_WILL_DRAW)
{
	BRect			theRect;
	short			loop;
	void			*picH;
	size_t			len;
	float			counter;
	short			index;
	float			x,y;
	entry_ref		ref;
	long			error;
	
	fFace = face;
	theRect.Set(0,0,82,82);

	for (index = 0; index <= 8; index++)
		fClockFace[index] = NULL;

#ifdef __HAIKU__
	BResources rsrcs;
	error = rsrcs.SetToImage(&&dummy_label);
dummy_label:
	if (error == B_OK) {
		{
#else	// !__HAIKU__
	// Note: Since we can be run as replicant, we get our resources this way,
	// not via be_app->AppResources().
	error = be_roster->FindApp(app_signature, &ref);
	printf("be_roster->FindApp() returned %s\n", strerror(error));
	if (error == B_NO_ERROR) {
		BFile file(&ref, O_RDONLY);
		error = file.InitCheck();
		if (error == B_NO_ERROR) {
			BResources rsrcs(&file);
#endif	// !__HAIKU__
			
			for (loop = 0; loop <= 8; loop++) {
				if ((picH = rsrcs.FindResource('PICT', loop+4, &len))) {
					fClockFace[loop] = new BBitmap(theRect, B_CMAP8);
					fClockFace[loop]->SetBits(picH,len,0, B_CMAP8);
					free(picH);
				}
			}

			theRect.Set(0,0,15,15);
			if ((picH = rsrcs.FindResource('MICN', "center", &len))) {
				fCenter = new BBitmap(theRect, B_CMAP8);
				fCenter->SetBits(picH,len,0, B_CMAP8);
				free(picH);
			}

			theRect.Set(0,0,2,2);
			if ((picH = rsrcs.FindResource('PICT', 13, &len))) {
				fInner = new BBitmap(theRect, B_CMAP8);
				fInner->SetBits(picH,len,0, B_CMAP8);
				free(picH);
			}
		}
	} 

	fMinutesRadius = mRadius;
	fHoursRadius = hRadius;
	fOffset = offset;
	fHours = 0;
	fMinutes = 0;
	fSeconds = 0;
	fShowSeconds = show;

	index = 0;

	//
	// Generate minutes points array
	//
	for (counter = 90; counter >= 0; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += 41;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += 41;
		fMinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += 41;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += 41;
		fHourPoints[index].Set(x,y);
	}
	for (counter = 354; counter > 90; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += 41;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += 41;
		fMinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += 41;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += 41;
		fHourPoints[index].Set(x,y);
	}
}


void
TOffscreenView::NextFace()
{	
	fFace++;
	if (fFace > 8)
		fFace = 1;
};


void
TOffscreenView::AttachedToWindow()
{
	SetFontSize(18);
	SetFont(be_plain_font);
}


void
TOffscreenView::DrawX()
{	
	ASSERT(Window());

	if (Window()->Lock()) {
		if (fClockFace != NULL)
			DrawBitmap(fClockFace[fFace], BPoint(0, 0));
		
		//
		// Draw hands
		//
		SetHighColor(0, 0, 0);
		int32 hours = fHours;
		if (hours >= 12)
			hours -= 12;
		hours *= 5;
		hours += (fMinutes / 12);
		SetDrawingMode(B_OP_OVER);
		StrokeLine(BPoint(fOffset, fOffset), fHourPoints[hours]);

		if (fCenter != NULL)
			DrawBitmap(fCenter, BPoint(fOffset - 3, fOffset - 3));
		StrokeLine(BPoint(fOffset, fOffset), fMinutePoints[fMinutes]);
		SetHighColor(180, 180, 180);
		if (fShowSeconds)
			StrokeLine(BPoint(fOffset, fOffset), fMinutePoints[fSeconds]);
		SetDrawingMode(B_OP_COPY);
		if (fInner != NULL)
			DrawBitmap(fInner, BPoint(fOffset - 1, fOffset - 1));
		Sync();
		Window()->Unlock();
	}
}


TOffscreenView::~TOffscreenView()
{
	for (int32 counter = 0; counter <= 8; counter++)
		delete fClockFace[counter];
};
	

/*
 * Onscreen view object
 */
TOnscreenView::TOnscreenView(BRect rect, char *title,
	short mRadius, short hRadius, short offset)
  	  :BView(rect, title, B_NOT_RESIZABLE,
		  B_WILL_DRAW | B_PULSE_NEEDED | B_DRAW_ON_CHILDREN)
{
	InitObject(rect, mRadius, hRadius, offset, 1, TRUE);

	BRect r = rect;
	r.OffsetTo(B_ORIGIN);
	r.top = r.bottom - 7;
	r.left = r.right - 7;

	BDragger *dw = new BDragger(r, this, 0);
	AddChild(dw);
}


void
TOnscreenView::InitObject(BRect rect, short mRadius, short hRadius,
	short offset, long face, bool show)
{
	fmRadius = mRadius;
	fhRadius = hRadius;
	fOffset = offset;
	fRect = rect;
	fOffscreenView = NULL;
	fOffscreen = NULL;

	fOffscreenView = new TOffscreenView(rect, "freqd",mRadius,hRadius, offset, face, show);
	fOffscreen = new BBitmap(rect, B_CMAP8, true);
	if (fOffscreen != NULL && fOffscreen->Lock()) {
		fOffscreen->AddChild(fOffscreenView);
		fOffscreen->Unlock();
		
		fOffscreenView->DrawX();
	}
}


TOnscreenView::~TOnscreenView()
{
	delete fOffscreen;
}


TOnscreenView::TOnscreenView(BMessage *data)
	: BView(data)
{
	InitObject(data->FindRect("bounds"), data->FindInt32("mRadius"),
		data->FindInt32("hRadius"), data->FindInt32("offset"),
		data->FindInt32("face"), data->FindBool("seconds"));
}


status_t
TOnscreenView::Archive(BMessage *data, bool deep) const
{
	inherited::Archive(data, deep);
	data->AddString("add_on", app_signature);
//+	data->AddString("add_on_path", "/boot/apps/Clock");

	data->AddRect("bounds", Bounds());
	data->AddInt32("mRadius", fOffscreenView->fMinutesRadius);
	data->AddInt32("hRadius", fOffscreenView->fHoursRadius);
	data->AddInt32("offset", fOffscreenView->fOffset);
	data->AddBool("seconds", fOffscreenView->fShowSeconds);
	data->AddInt32("face", fOffscreenView->fFace);
	return 0;
}


BArchivable *
TOnscreenView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "TOnscreenView"))
		return NULL;
	return new TOnscreenView(data);
}


void
TOnscreenView::AttachedToWindow()
{
//+	PRINT(("InitData m=%d, h=%d, offset=%d\n", fmRadius, fhRadius, fOffset));
//+	PRINT_OBJECT(fRect);
}


void
TOnscreenView::Pulse()
{
	short		hours,minutes,seconds;
	struct tm	*loctime;
	time_t		current;

	ASSERT(fOffscreen);
	ASSERT(fOffscreenView);
	current = time(0);
	loctime = localtime(&current);
	hours = loctime->tm_hour;
	minutes = loctime->tm_min;
	seconds = loctime->tm_sec;
	
	if ((fOffscreenView->fShowSeconds && (seconds != fOffscreenView->fSeconds)) ||
		(minutes != fOffscreenView->fMinutes)) {
			fOffscreenView->fHours = hours;
			fOffscreenView->fMinutes = minutes;
			fOffscreenView->fSeconds = seconds;
			BRect	b = Bounds();
			b.InsetBy(12,12);
			Draw(b);
	}
}


void
TOnscreenView::UseFace( short face )
{
	fOffscreenView->fFace = face;
	BRect	b = Bounds();
	b.InsetBy(12,12);
	Draw(b);
}


void
TOnscreenView::ShowSecs( bool secs )
{
	fOffscreenView->fShowSeconds = secs;
	BRect	b = Bounds();
	b.InsetBy(12,12);
	Invalidate(b);
}


short
TOnscreenView::ReturnFace( void )
{
	return(fOffscreenView->fFace);
}


short
TOnscreenView::ReturnSeconds( void )
{
	return(fOffscreenView->fShowSeconds);
}


void
TOnscreenView::Draw(BRect rect)
{
	ASSERT(fOffscreen);
	ASSERT(fOffscreenView);

	if (fOffscreen->Lock()) {
		fOffscreenView->DrawX();			// Composite the clock offscreen...
		DrawBitmap(fOffscreen, rect, rect);
		fOffscreen->Unlock();
	}
};


void
TOnscreenView::MouseDown( BPoint point )
{
	BPoint	cursor;
	ulong	buttons;
	BRect	bounds = Bounds();
	
	GetMouse(&cursor,&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		fOffscreenView->fShowSeconds = !fOffscreenView->fShowSeconds;
		be_app->PostMessage(SHOW_SECONDS);
		bounds.InsetBy(12,12);
		Invalidate(bounds);
	} else {
		fOffscreenView->NextFace();
		Invalidate(bounds);
		BView *child = ChildAt(0);
		if (child)
			child->Invalidate();
	}
};


void
TOnscreenView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("About Clock", "Clock (The Replicant version)\n\n(C)2002 OpenBeOS\n\nOriginally coded  by the folks at Be.\n  Copyright Be Inc., 1991-1998","OK"))->Go();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}
