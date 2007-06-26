#include "TrackMenu.h"

#include <Font.h>
#include <Message.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include <stdio.h>


TrackMenu::TrackMenu(const BRect &frame, const char *name, BMessage *msg,
	const int32 &resize, const int32 &flags)
	: BView(frame, name, resize, flags), BInvoker(msg, NULL),
 	fCurrentItem(-1),
 	fCount(0),
 	fIsTracking(false)
{
	SetViewColor(20, 20, 20);
	
	fItemRect.Set(1, 1, 1 + StringWidth("00") + 3, Bounds().bottom - 1);
	
	BFont font;
	font.SetSize(11);
	font.SetSpacing(B_STRING_SPACING);
	SetFont(&font);
	
	font_height fh;
	GetFontHeight(&fh);
	
	fFontHeight = fh.ascent + fh.descent + fh.leading;
}


TrackMenu::~TrackMenu()
{
}


void
TrackMenu::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());

	BView::AttachedToWindow();
}

	
void
TrackMenu::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
TrackMenu::SetItemCount(const int32 &count)
{
	if (count < 0) {
		fCount = 0;
		fCurrentItem = -1;
		Invalidate();
		return;
	}

	fCount = count;

	if (fCurrentItem > fCount - 1)
		fCurrentItem = fCount - 1;
	else if(fCurrentItem < 0)
		fCurrentItem = 0;

	Invalidate();
}


void
TrackMenu::SetValue(const int32 &value)
{
	if (value < 0 || value > fCount)
		return;

	if (value != Value()) {
		fCurrentItem = value;
		Invalidate();
	}
}


int32
TrackMenu::ItemAt(const BPoint &pt)
{
	// TODO: Optimize. This is simple, but costly in performance
	BRect r(fItemRect);
	for (int32 i = 0; i < fCount; i++) {
		if (r.Contains(pt))
			return i;
		r.OffsetBy(r.Width()+1,0);
	}

	return -1;
}


void
TrackMenu::MouseDown(BPoint point)
{
	BPoint pt(point);
	int32 saveditem = fCurrentItem;
	int32 item = ItemAt(pt);

	if (item >= 0) {
		fCurrentItem = item;
		Invalidate();

		// Shamelessly stolen from BButton. :D
		if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
	 		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	 		fIsTracking = true;
	 	
	 	} else {
			uint32 buttons;

			do {
				Window()->UpdateIfNeeded();

				snooze(40000);

				GetMouse(&pt, &buttons, true);

				int32 mouseitem = ItemAt(pt);
				if (mouseitem > -1) {
					fCurrentItem = mouseitem;
					Draw(Bounds());
				}

			} while (buttons != 0);

			if (fCurrentItem != saveditem)
				Invoke();
		}
	}
}


void
TrackMenu::MouseUp(BPoint pt)
{
	if (!fIsTracking)
		return;

	int32 item = ItemAt(pt);

	if (item >= 0)
		Invoke();

	fIsTracking = false;
}


void
TrackMenu::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	if (!fIsTracking)
		return;

	int32 item = ItemAt(pt);

	if (item >= 0) {
		fCurrentItem = item;
		Invalidate();
	}
}


void
TrackMenu::Draw(BRect update)
{
	rgb_color dark = {20, 20, 20, 255};
	rgb_color light = {200, 200, 200, 255};
	BPoint pt1, pt2;

	// Draw the frame
	SetHighColor(dark);
	pt1.Set(0, 0);
	pt2 = Bounds().RightTop();
	StrokeLine(pt1, pt2);

	pt2.Set(0, Bounds().bottom);
	StrokeLine(pt1, pt2);

	SetHighColor(255,255,255);
	pt1 = Bounds().RightBottom();
	pt2.Set(Bounds().right, 1);
	StrokeLine(pt1, pt2);

	pt2.Set(1,Bounds().bottom);
	StrokeLine(pt1, pt2);

	// Draw the items
	BRect r(fItemRect);

	for (int32 i = 0; i < fCount; i++) {
		// Draw the item's frame
		if (i == fCurrentItem)
			SetHighColor(dark);
		else
			SetHighColor(light);

		pt1.Set(r.left, r.top);
		pt2.Set(r.right, r.top);
		StrokeLine(pt1, pt2);

		pt2.Set(r.left, r.bottom);
		StrokeLine(pt1, pt2);

		if (i == fCurrentItem) {
			SetHighColor(light);
			pt1.Set(r.right, r.bottom);
			pt2.Set(r.right, r.top + 1);
			StrokeLine(pt1, pt2);

			pt2.Set(r.left + 1, r.bottom);
			StrokeLine(pt1, pt2);

			SetHighColor(light);
			FillRect(r.InsetByCopy(1, 1));
			SetHighColor(dark);
		}
		else if (i == fCount - 1) {
			SetHighColor(light);
			pt1.Set(r.right, r.bottom);
			pt2.Set(r.right, r.top + 1);
			StrokeLine(pt1, pt2);
		}

		// Draw the label, center justified

		BString label;
		label << (i + 1);

		BPoint labelpt;
		labelpt.x = r.left + (r.Width() - StringWidth(label.String())) / 2 + 2;
		labelpt.y = r.bottom - (r.Height() - fFontHeight + 4) / 2;

		if (i == fCurrentItem) {
			SetHighColor(dark);
			SetLowColor(light);
		} else {
			SetHighColor(light);
			SetLowColor(dark);
		}
		DrawString(label.String(), labelpt);

		// Setup for next iteration
		r.OffsetBy(r.Width() + 1, 0);

		if (r.left > Bounds().right - 2) {
			ConstrainClippingRegion(NULL);
			break;
		}

		if (r.right > Bounds().right - 2) {
			r.right = Bounds().right - 2;
			BRegion reg(r);
			ConstrainClippingRegion(&reg);
		}
	}
}
