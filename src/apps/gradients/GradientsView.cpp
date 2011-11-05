/*
 * Copyright (c) 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "GradientsView.h"
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>


GradientsView::GradientsView(const BRect &rect)
	: BView(rect, "gradientsview", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	fType(BGradient::TYPE_LINEAR)
{
}


GradientsView::~GradientsView()
{
}


void
GradientsView::Draw(BRect update)
{
	switch (fType) {
		case BGradient::TYPE_LINEAR:
			DrawLinear(update);
			break;

		case BGradient::TYPE_RADIAL:
			DrawRadial(update);
			break;

		case BGradient::TYPE_RADIAL_FOCUS:
			DrawRadialFocus(update);
			break;

		case BGradient::TYPE_DIAMOND:
			DrawDiamond(update);
			break;

		case BGradient::TYPE_CONIC:
			DrawConic(update);
			break;

		case BGradient::TYPE_NONE:
		default:
			break;
	}
}


void
GradientsView::DrawLinear(BRect update)
{
	BGradientLinear gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);

	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetStart(BPoint(120, 10));
	gradient.SetEnd(BPoint(220, 110));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetStart(BPoint(120, 120));
	gradient.SetEnd(BPoint(220, 220));
	FillRect(BRect(120, 120, 220, 220), gradient);

	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetStart(BPoint(60, 230));
	gradient.SetEnd(BPoint(60, 330));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330),
		gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetStart(BPoint(60, 340));
	gradient.SetEnd(BPoint(60, 440));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawRadial(BRect update)
{
	BGradientRadial gradient;
	gradient.AddColor(make_color(255, 0, 0), 0);
	gradient.AddColor(make_color(0, 255, 0), 127);
	gradient.AddColor(make_color(0, 0, 255), 255);

	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	gradient.SetRadius(50);
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);

	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330),
		gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawRadialFocus(BRect update)
{
	BGradientRadialFocus gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);

	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);

	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330),
		gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawDiamond(BRect update)
{
	BGradientDiamond gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);

	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);

	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330),
		gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::DrawConic(BRect update)
{
	BGradientConic gradient;
	rgb_color c;
	c.red = 255;
	c.green = 0;
	c.blue = 0;
	gradient.AddColor(c, 0);
	c.red = 0;
	c.green = 255;
	c.blue = 0;
	gradient.AddColor(c, 127);
	c.red = 0;
	c.green = 0;
	c.blue = 255;
	gradient.AddColor(c, 255);

	// RoundRect
	SetHighColor(0, 0, 0);
	FillRoundRect(BRect(10, 10, 110, 110), 5, 5);
	gradient.SetCenter(BPoint(170, 60));
	FillRoundRect(BRect(120, 10, 220, 110), 5, 5, gradient);

	// Rect
	SetHighColor(0, 0, 0);
	FillRect(BRect(10, 120, 110, 220));
	gradient.SetCenter(BPoint(170, 170));
	FillRect(BRect(120, 120, 220, 220), gradient);

	// Triangle
	SetHighColor(0, 0, 0);
	FillTriangle(BPoint(60, 230), BPoint(10, 330), BPoint(110, 330));
	gradient.SetCenter(BPoint(170, 280));
	FillTriangle(BPoint(170, 230), BPoint(120, 330), BPoint(220, 330),
		gradient);

	// Ellipse
	SetHighColor(0, 0, 0);
	FillEllipse(BPoint(60, 390), 50, 50);
	gradient.SetCenter(BPoint(170, 390));
	FillEllipse(BPoint(170, 390), 50, 50, gradient);
}


void
GradientsView::SetType(BGradient::Type type)
{
	fType = type;
	Invalidate();
}
