/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "TestPageView.h"

#include <Catalog.h>
#include <GradientLinear.h>

#include "PrinterListView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TestPageView"


TestPageView::TestPageView(BRect frame, PrinterItem* printer)
	: BView(frame, "testpage", B_FOLLOW_NONE, B_WILL_DRAW),
	fPrinter(printer)
{
}


void
TestPageView::Draw(BRect updateRect)
{
	// TODO: build using layout manager the page content,
	// using subviews, to adapt any page format.

	// Draw corners

	float width = Bounds().Width();
	float height = Bounds().Height();
	float minDimension = MIN(width, height);

	float size = minDimension * 0.02;

	BPoint pt = Bounds().LeftTop();
	StrokeLine(pt, BPoint(pt.x + size, pt.y));
	StrokeLine(pt, BPoint(pt.x, pt.y + size));

	pt = Bounds().RightTop();
	StrokeLine(pt, BPoint(pt.x - size, pt.y));
	StrokeLine(pt, BPoint(pt.x, pt.y + size));

	pt = Bounds().RightBottom();
	StrokeLine(pt, BPoint(pt.x - size, pt.y));
	StrokeLine(pt, BPoint(pt.x, pt.y - size));

	pt = Bounds().LeftBottom();
	StrokeLine(pt, BPoint(pt.x + size, pt.y));
	StrokeLine(pt, BPoint(pt.x, pt.y - size));

	// Draw some text

	BString text;

	text = B_TRANSLATE("Test Page for '%printer_name%'");
	text.ReplaceFirst("%printer_name%", fPrinter->Name());

	SetFont(be_bold_font);
	MovePenTo(50, 50);
	DrawString(text);

	SetFont(be_plain_font);
	text = B_TRANSLATE("Driver: %driver%");
	text.ReplaceFirst("%driver%", fPrinter->Driver());
	MovePenTo(50, 80);
	DrawString(text);

	if (strlen(fPrinter->Transport()) > 0) {
		text = B_TRANSLATE("Transport: %transport% %transport_address%");
		text.ReplaceFirst("%transport%", fPrinter->Transport());
		text.ReplaceFirst("%transport_address%", fPrinter->TransportAddress());
		MovePenTo(50, 110);
		DrawString(text);
	}

	// Draw some gradients

	BRect rect(50, 140, 50 + 200, 140 + 20);
	BGradientLinear gradient(rect.LeftTop(), rect.RightBottom());
	rgb_color white = make_color(255, 255, 255);
	gradient.AddColor(white, 0.0);

	// Red gradient
	gradient.AddColor(make_color(255, 0, 0), 255.0);
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Green gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(0, 255, 0));
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Blue gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(0, 0, 255));
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Yellow gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(255, 255, 0));
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Magenta gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(255, 0, 255));
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Cyan gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(0, 255, 255));
	FillRect(rect, gradient);
	StrokeRect(rect);

	// Gray gradient
	rect.OffsetBy(0, 20);
	gradient.SetColor(1, make_color(0, 0, 0));
	FillRect(rect, gradient);
	StrokeRect(rect);
}
