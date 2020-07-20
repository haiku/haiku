/*
 * Copyright 2003-2015 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "PreviewView.h"

#include <algorithm>
#include <iostream>

#include <CardLayout.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <Point.h>
#include <Rect.h>
#include <Size.h>
#include <StringView.h>
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
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fSaverView(NULL),
	fNoPreview(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	// We draw the "monitor" around the preview, hence the strange insets.
	layout->SetInsets(7, 6, 8, 12);
	SetLayout(layout);

	// A BStringView would be enough, if only it handled word wrapping.
	fNoPreview = new BTextView("no preview");
	fNoPreview->SetText(B_TRANSLATE("No preview available"));
	fNoPreview->SetFontAndColor(be_plain_font, B_FONT_ALL, &kWhite);
	fNoPreview->MakeEditable(false);
	fNoPreview->MakeResizable(false);
	fNoPreview->MakeSelectable(false);
	fNoPreview->SetViewColor(0, 0, 0);
	fNoPreview->SetLowColor(0, 0, 0);

	fNoPreview->Hide();

	BView* container = new BView("preview container", 0);
	container->SetLayout(new BCardLayout());
	AddChild(container);
	container->SetViewColor(0, 0, 0);
	container->SetLowColor(0, 0, 0);
	container->AddChild(fNoPreview);

	fNoPreview->SetHighColor(255, 255, 255);
	fNoPreview->SetAlignment(B_ALIGN_CENTER);
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
	fSaverView = new BView("preview", B_WILL_DRAW);
	fSaverView->SetViewColor(0, 0, 0);
	fSaverView->SetLowColor(0, 0, 0);
	ChildAt(0)->AddChild(fSaverView);

	float aspectRatio = 4.0f / 3.0f;
		// 4:3 monitor
	float previewWidth = 120.0f * std::max(1.0f, be_plain_font->Size() / 12.0f);
	float previewHeight = ceilf(previewWidth / aspectRatio);

	fSaverView->SetExplicitSize(BSize(previewWidth, previewHeight));
	fSaverView->ResizeTo(previewWidth, previewHeight);

	fNoPreview->SetExplicitSize(BSize(previewWidth, previewHeight));
	fNoPreview->ResizeTo(previewWidth, previewHeight);
	fNoPreview->SetTextRect(BRect(0, 0, previewWidth, previewHeight));
	fNoPreview->SetInsets(0, previewHeight / 3, 0, 0);

	return fSaverView;
}


BView*
PreviewView::RemovePreview()
{
	ShowNoPreview();

	if (fSaverView != NULL)
		ChildAt(0)->RemoveChild(fSaverView);

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
	((BCardLayout*)ChildAt(0)->GetLayout())->SetVisibleItem((int32)0);
}


void
PreviewView::HideNoPreview() const
{
	((BCardLayout*)ChildAt(0)->GetLayout())->SetVisibleItem(1);
}
