/*
	
	cl_view.cpp
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEBUG 1
#include <string.h>
#include <stdlib.h>

#include <time.h>
#include "clock.h"
#include "cl_view.h"
#include <float.h>
#include <math.h>

#include <Debug.h>
#include <Application.h>
#include <Roster.h>
#include <Alert.h>
#include <Entry.h>
#include <Resources.h>

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

//+	error = get_ref_for_path("/boot/apps/Clock", &ref);
	error = be_roster->FindApp(app_signature, &ref);

	if (error == B_NO_ERROR) {
		BFile	file(&ref, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			BResources	rsrcs(&file);
			error = 0;

			for (loop = 0; loop <= 8; loop++) {
				if ((picH = rsrcs.FindResource('PICT', loop+4, &len))) {
					fClockFace[loop] = new BBitmap(theRect,B_COLOR_8_BIT);
					fClockFace[loop]->SetBits(picH,len,0,B_COLOR_8_BIT);
					free(picH);
				} else {
					error = 1;
				}
			}

			theRect.Set(0,0,15,15);
			if ((picH = rsrcs.FindResource('MICN', "center", &len))) {
				fCenter = new BBitmap(theRect,B_COLOR_8_BIT);
				fCenter->SetBits(picH,len,0,B_COLOR_8_BIT);
				free(picH);
			} else {
				error = 1;
			}

			theRect.Set(0,0,2,2);
			if ((picH = rsrcs.FindResource('PICT', 13, &len))) {
				fInner = new BBitmap(theRect,B_COLOR_8_BIT);
				fInner->SetBits(picH,len,0,B_COLOR_8_BIT);
				free(picH);
			} else {
				error = 1;
			}

			if (error) {
				// ??? yikes. This is terrible. No good way to signal error
				// from here. How do we properly cleanup up here? Need to
				// make sure the app_server side also gets cleaned up!

				BAlert *alert = new BAlert("Error", "Clock: Could not find necessary resources.", "Quit");
				alert->Go();
				delete this;
				exit(1);
			}
		}
	} else {
	 	exit(1);
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

/* ---------------------------------------------------------------- */

void TOffscreenView::NextFace()
{	
	fFace++;
	if (fFace > 8)
		fFace = 1;
};

/* ---------------------------------------------------------------- */

void TOffscreenView::AttachedToWindow()
{
	SetFontSize(18);
	SetFont(be_plain_font);
}

/* ---------------------------------------------------------------- */

void TOffscreenView::DrawX()
{
//BRect		bound;
short		hours;
	
	ASSERT(Window());

	if (Window()->Lock()) {
		DrawBitmap(fClockFace[fFace],BPoint(0,0));
		
		//
		// Draw hands
		//
		SetHighColor(0,0,0);
		hours = fHours;
		if (hours >= 12)
			hours -= 12;
		hours *= 5;
		hours += (fMinutes / 12);
		StrokeLine(BPoint(fOffset,fOffset),fHourPoints[hours]);
		SetDrawingMode(B_OP_OVER);

		DrawBitmap(fCenter,BPoint(fOffset-3,fOffset-3));
		SetDrawingMode(B_OP_COPY);
		StrokeLine(BPoint(fOffset,fOffset),fMinutePoints[fMinutes]);
		SetHighColor(180,180,180);
		if (fShowSeconds)
			StrokeLine(BPoint(fOffset,fOffset),fMinutePoints[fSeconds]);
		DrawBitmap(fInner,BPoint(fOffset-1,fOffset-1));
		Sync();
		Window()->Unlock();
	}
}

/* ---------------------------------------------------------------- */

TOffscreenView::~TOffscreenView()
{
	short	counter;
	
	for (counter = 0; counter <= 8; counter++)
		delete fClockFace[counter];
};
	
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#include <Dragger.h>

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

void TOnscreenView::InitObject(BRect rect, short mRadius, short hRadius,
	short offset, long face, bool show)
{
	fmRadius = mRadius;
	fhRadius = hRadius;
	fOffset = offset;
	fRect = rect;
	OffscreenView = NULL;
	Offscreen = NULL;

#if 1
	OffscreenView = new TOffscreenView(rect, "freqd",mRadius,hRadius,
										offset, face, show);
	Offscreen = new BBitmap(rect, B_COLOR_8_BIT, TRUE);
	Offscreen->Lock();
	Offscreen->AddChild(OffscreenView);
	Offscreen->Unlock();
	OffscreenView->DrawX();
#endif
}

/* ---------------------------------------------------------------- */

TOnscreenView::~TOnscreenView()
{
	delete Offscreen;
}

/* ---------------------------------------------------------------- */

TOnscreenView::TOnscreenView(BMessage *data)
	: BView(data)
{
	InitObject(data->FindRect("bounds"), data->FindInt32("mRadius"),
		data->FindInt32("hRadius"), data->FindInt32("offset"),
		data->FindInt32("face"), data->FindBool("seconds"));
}

/* ---------------------------------------------------------------- */

status_t TOnscreenView::Archive(BMessage *data, bool deep) const
{
	inherited::Archive(data, deep);
	data->AddString("add_on", app_signature);
//+	data->AddString("add_on_path", "/boot/apps/Clock");

	data->AddRect("bounds", Bounds());
	data->AddInt32("mRadius", OffscreenView->fMinutesRadius);
	data->AddInt32("hRadius", OffscreenView->fHoursRadius);
	data->AddInt32("offset", OffscreenView->fOffset);
	data->AddBool("seconds", OffscreenView->fShowSeconds);
	data->AddInt32("face", OffscreenView->fFace);
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *TOnscreenView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "TOnscreenView"))
		return NULL;
	return new TOnscreenView(data);
}

/* ---------------------------------------------------------------- */

void TOnscreenView::AttachedToWindow()
{
//+	PRINT(("InitData m=%d, h=%d, offset=%d\n", fmRadius, fhRadius, fOffset));
//+	PRINT_OBJECT(fRect);
}

/* ---------------------------------------------------------------- */

void TOnscreenView::Pulse()
{
	short		hours,minutes,seconds;
	struct tm	*loctime;
	time_t		current;

	ASSERT(Offscreen);
	ASSERT(OffscreenView);
	current = time(0);
	loctime = localtime(&current);
	hours = loctime->tm_hour;
	minutes = loctime->tm_min;
	seconds = loctime->tm_sec;
	
	if ((OffscreenView->fShowSeconds && (seconds != OffscreenView->fSeconds)) ||
		(minutes != OffscreenView->fMinutes)) {
			OffscreenView->fHours = hours;
			OffscreenView->fMinutes = minutes;
			OffscreenView->fSeconds = seconds;
			BRect	b = Bounds();
			b.InsetBy(12,12);
			Draw(b);
	}
}

/* ---------------------------------------------------------------- */

void TOnscreenView::UseFace( short face )
{
	OffscreenView->fFace = face;
	BRect	b = Bounds();
	b.InsetBy(12,12);
	Draw(b);
}

/* ---------------------------------------------------------------- */

void TOnscreenView::ShowSecs( bool secs )
{
	OffscreenView->fShowSeconds = secs;
	BRect	b = Bounds();
	b.InsetBy(12,12);
	Invalidate(b);
}

/* ---------------------------------------------------------------- */

short TOnscreenView::ReturnFace( void )
{
	return(OffscreenView->fFace);
}

/* ---------------------------------------------------------------- */

short TOnscreenView::ReturnSeconds( void )
{
	return(OffscreenView->fShowSeconds);
}

/* ---------------------------------------------------------------- */

void TOnscreenView::Draw(BRect rect)
{
	time_t		current;
	ASSERT(Offscreen);
	ASSERT(OffscreenView);

	bool b = Offscreen->Lock();
	ASSERT(b);
	OffscreenView->DrawX();			// Composite the clock offscreen...
	DrawBitmap(Offscreen, rect, rect);
	Offscreen->Unlock();

	current = time(0);
};

/* ---------------------------------------------------------------- */

void TOnscreenView::MouseDown( BPoint point )
{
	BPoint	cursor;
	ulong	buttons;
	BRect	bounds = Bounds();
	
	GetMouse(&cursor,&buttons);
	if (buttons & 0x2) {
		OffscreenView->fShowSeconds = !OffscreenView->fShowSeconds;
		be_app->PostMessage(SHOW_SECONDS);
		bounds.InsetBy(12,12);
		Draw(bounds);
	} else {
		OffscreenView->NextFace();
		Draw(bounds);
		BView *c = ChildAt(0);
		if (c)
			c->Draw(c->Bounds());
	}
};

/* ---------------------------------------------------------------- */

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
