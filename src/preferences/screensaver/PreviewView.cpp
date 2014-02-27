/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "PreviewView.h"

#include <iostream>

#include <Catalog.h>
#include <Point.h>
#include <Rect.h>
#include <Size.h>
#include <TextView.h>

#include "Utility.h"


static const rgb_color kWhite = (rgb_color){ 255, 255, 255 };


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreviewView"


static float sampleX[]
	= { 0, 0.05, 0.15, 0.7, 0.725, 0.8, 0.825, 0.85, 0.950, 1.0 };
static float sampleY[] = { 0, 0.05, 0.90, 0.95, 0.966, 0.975, 1.0 };


inline BPoint
scale2(int x, int y, BRect area)
{
	return scale_direct(sampleX[x], sampleY[y], area);
}


inline BRect
scale2(int x1, int x2, int y1, int y2, BRect area)
{
	return scale_direct(sampleX[x1], sampleX[x2], sampleY[y1], sampleY[y2],
		area);
}


//	#pragma mark - PreviewView


PreviewView::PreviewView(const char* name)
	:
	BView(name, B_WILL_DRAW),
	fSaverView(NULL),
	fNoPreview(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	float aspectRatio = 4.0f / 3.0f;
		// 4:3 monitor
	float previewWidth = 160.0f;
	float previewHeight = ceilf(previewWidth / aspectRatio);

	SetExplicitSize(BSize(previewWidth, previewHeight));
}


PreviewView::~PreviewView()
{
}


void
PreviewView::Draw(BRect updateRect)
{
	SetHighColor(184, 184, 184);
	FillRoundRect(scale2(0, 9, 0, 3, Bounds()), 4, 4);
		// outer shape
	FillRoundRect(scale2(2, 7, 3, 6, Bounds()), 2, 2);
		// control console outline

	SetHighColor(96, 96, 96);
	StrokeRoundRect(scale2(2, 7, 3, 6, Bounds()), 2, 2);
		// control console outline
	StrokeRoundRect(scale2(0, 9, 0, 3, Bounds()), 4, 4);
		// outline outer shape

	SetHighColor(0, 0, 0);
	FillRect(scale2(1, 8, 1, 2, Bounds()));

	SetHighColor(184, 184, 184);
	BRect outerShape = scale2(2, 7, 2, 6, Bounds());
	outerShape.InsetBy(1, 1);
	FillRoundRect(outerShape, 4, 4);
		// outer shape

	SetHighColor(0, 255, 0);
	FillRect(scale2(3, 4, 4, 5, Bounds()));
	SetHighColor(96, 96, 96);
	FillRect(scale2(5, 6, 4, 5, Bounds()));
}


BView*
PreviewView::AddPreview()
{
	BRect rect(scale2(1, 8, 1, 2, Bounds()).InsetBySelf(1.0f, 1.0f));
	fSaverView = new BView(rect, "preview", B_FOLLOW_NONE, B_WILL_DRAW);
	fSaverView->SetViewColor(0, 0, 0);
	fSaverView->SetLowColor(0, 0, 0);
	AddChild(fSaverView);

	BRect textRect(rect);
	textRect.OffsetTo(-7.0f, 0.0f);
	textRect.InsetBy(15.0f, 20.0f);
	fNoPreview = new BTextView(rect, "no preview", textRect, B_FOLLOW_NONE,
		B_WILL_DRAW);
	fNoPreview->SetViewColor(0, 0, 0);
	fNoPreview->SetLowColor(0, 0, 0);
	fNoPreview->SetFontAndColor(be_plain_font, B_FONT_ALL, &kWhite);
	fNoPreview->SetText(B_TRANSLATE("No preview available"));
	fNoPreview->SetAlignment(B_ALIGN_CENTER);
	fNoPreview->MakeEditable(false);
	fNoPreview->MakeResizable(false);
	fNoPreview->MakeSelectable(false);

	fNoPreview->Hide();
	fSaverView->AddChild(fNoPreview);

	return fSaverView;
}


BView*
PreviewView::RemovePreview()
{
	if (fSaverView != NULL)
		RemoveChild(fSaverView);

	BView* saverView = fSaverView;
	fSaverView = NULL;
	return saverView;
}


BView*
PreviewView::SaverView()
{
	return fSaverView;
}


void
PreviewView::ShowNoPreview() const
{
	fNoPreview->Show();
}


void
PreviewView::HideNoPreview() const
{
	fNoPreview->Hide();
}
