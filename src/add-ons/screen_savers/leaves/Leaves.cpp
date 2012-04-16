/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Deyan Genovski, deangenovski@gmail.com
 *		Geoffry Song, goffrie@gmail.com
 */
#include "Leaves.h"

#include <stdlib.h>
#include <time.h>

#include <AffineTransform.h>
#include <Catalog.h>
#include <GradientLinear.h>
#include <Shape.h>
#include <Slider.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Leaves"


// path data for the leaf shape
static const BPoint kLeafBegin(56.24793f, 15.46287f);
static BPoint kLeafCurves[][3] = {
	{ BPoint(61.14, 28.89), BPoint(69.78, 38.25), BPoint(83.48, 44.17) },
	{ BPoint(99.46, 37.52), BPoint(113.27, 29.61), BPoint(134.91, 30.86) },
	{ BPoint(130.58, 36.53), BPoint(126.74, 42.44), BPoint(123.84, 48.81) },
	{ BPoint(131.81, 42.22), BPoint(137.53, 38.33), BPoint(144.37, 33.10) },
	{ BPoint(169.17, 23.55), BPoint(198.90, 15.55), BPoint(232.05, 10.51) },
	{ BPoint(225.49, 18.37), BPoint(219.31, 28.17), BPoint(217.41, 40.24) },
	{ BPoint(227.70, 26.60), BPoint(239.97, 14.63), BPoint(251.43, 8.36) },
	{ BPoint(288.89, 9.12), BPoint(322.73, 14.33), BPoint(346.69, 31.67) },
	{ BPoint(330.49, 37.85), BPoint(314.36, 44.25), BPoint(299.55, 54.17) },
	{ BPoint(292.48, 52.54), BPoint(289.31, 49.70), BPoint(285.62, 47.03) },
	{ BPoint(283.73, 54.61), BPoint(284.46, 57.94), BPoint(285.62, 60.60) },
	{ BPoint(259.78, 76.14), BPoint(233.24, 90.54), BPoint(202.41, 98.10) },
	{ BPoint(194.43, 95.36), BPoint(185.96, 92.39), BPoint(179.63, 88.33) },
	{ BPoint(180.15, 94.75), BPoint(182.73, 99.76), BPoint(185.62, 104.53) },
	{ BPoint(154.83, 119.46), BPoint(133.21, 118.97), BPoint(125.62, 94.88) },
	{ BPoint(124.70, 98.79), BPoint(124.11, 103.67), BPoint(124.19, 110.60) },
	{ BPoint(116.42, 111.81), BPoint(85.82, 99.60), BPoint(83.25, 51.96) },
	{ BPoint(62.50, 42.57), BPoint(58.12, 33.18), BPoint(50.98, 23.81) } };
static const int kLeafCurveCount = sizeof(kLeafCurves) / sizeof(kLeafCurves[0]);
static const float kLeafWidth = 372.f;
static const float kLeafHeight = 121.f;

// leaf colors
static const rgb_color kColors[][2] = {
	{ {255, 114, 0, 255}, {255, 159, 0, 255} },
	{ {50, 160, 40, 255}, {125, 210, 32, 255} },
	{ {250, 190, 30, 255}, {255, 214, 125, 255} } };
static const int kColorCount = sizeof(kColors) / sizeof(kColors[0]);

// bounds for settings
static const int kMinimumDropRate = 2, kMaximumDropRate = 50;
static const int kMinimumLeafSize = 50, kMaximumLeafSize = 1000;
static const int kMaximumSizeVariation = 200;

enum {
	MSG_SET_DROP_RATE		= 'drop',
	MSG_SET_LEAF_SIZE		= 'size',
	MSG_SET_SIZE_VARIATION	= 'svry',
};


extern "C" BScreenSaver*
instantiate_screen_saver(BMessage* msg, image_id image)
{
	return new Leaves(msg, image);
}


Leaves::Leaves(BMessage* archive, image_id id)
	:
	BScreenSaver(archive, id),
	fDropRateSlider(NULL),
	fLeafSizeSlider(NULL),
	fSizeVariationSlider(NULL),
	fDropRate(10),
	fLeafSize(150),
	fSizeVariation(50)
{
	if (archive) {
		if (archive->FindInt32("Leaves drop rate", &fDropRate) != B_OK)
			fDropRate = 10;
		if (archive->FindInt32("Leaves size", &fLeafSize) != B_OK)
			fLeafSize = 150;
		if (archive->FindInt32("Leaves variation", &fSizeVariation) != B_OK)
			fSizeVariation = 50;
	}
}


void
Leaves::StartConfig(BView* view)
{
	BRect bounds = view->Bounds();
	bounds.InsetBy(10, 10);
	BRect frame(0, 0, bounds.Width(), 20);

	fDropRateSlider = new BSlider(frame, "drop rate",
		B_TRANSLATE("Drop rate:"), new BMessage(MSG_SET_DROP_RATE),
		kMinimumDropRate, kMaximumDropRate,	B_BLOCK_THUMB,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	fDropRateSlider->SetValue(fDropRate);
	fDropRateSlider->ResizeToPreferred();
	bounds.bottom -= fDropRateSlider->Bounds().Height() * 1.5;
	fDropRateSlider->MoveTo(bounds.LeftBottom());
	view->AddChild(fDropRateSlider);

	fLeafSizeSlider = new BSlider(frame, "leaf size",
		B_TRANSLATE("Leaf size:"), new BMessage(MSG_SET_LEAF_SIZE),
		kMinimumLeafSize, kMaximumLeafSize,	B_BLOCK_THUMB,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	fLeafSizeSlider->SetValue(fLeafSize);
	fLeafSizeSlider->ResizeToPreferred();
	bounds.bottom -= fLeafSizeSlider->Bounds().Height() * 1.5;
	fLeafSizeSlider->MoveTo(bounds.LeftBottom());
	view->AddChild(fLeafSizeSlider);

	fSizeVariationSlider = new BSlider(frame, "variation",
		B_TRANSLATE("Size variation:"),	new BMessage(MSG_SET_SIZE_VARIATION),
		0, kMaximumSizeVariation, B_BLOCK_THUMB,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	fSizeVariationSlider->SetValue(fSizeVariation);
	fSizeVariationSlider->ResizeToPreferred();
	bounds.bottom -= fSizeVariationSlider->Bounds().Height() * 1.5;
	fSizeVariationSlider->MoveTo(bounds.LeftBottom());
	view->AddChild(fSizeVariationSlider);

	BTextView* textView = new BTextView(bounds, B_EMPTY_STRING,
		bounds.OffsetToCopy(0., 0.), B_FOLLOW_ALL, B_WILL_DRAW);
	textView->SetViewColor(view->ViewColor());
	BString name = B_TRANSLATE("Leaves");
	BString text = name;
	text << "\n\n";
	text << B_TRANSLATE("by Deyan Genovski, Geoffry Song");
	text << "\n\n";

	textView->Insert(text.String());
	textView->SetStylable(true);
	textView->SetFontAndColor(0, name.Length(), be_bold_font);
	textView->MakeEditable(false);
	view->AddChild(textView);

	BWindow* window = view->Window();
	if (window) window->AddHandler(this);

	fDropRateSlider->SetTarget(this);
	fLeafSizeSlider->SetTarget(this);
	fSizeVariationSlider->SetTarget(this);
}


status_t
Leaves::StartSaver(BView* view, bool preview)
{
	SetTickSize(10000000 / fDropRate);
	srand48(time(NULL));

	view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);

	return B_OK;
}


status_t
Leaves::SaveState(BMessage* into) const
{
	status_t status;
	if ((status = into->AddInt32("Leaves drop rate", fDropRate)) != B_OK)
		return status;
	if ((status = into->AddInt32("Leaves size", fLeafSize)) != B_OK)
		return status;
	if ((status = into->AddInt32("Leaves variation", fSizeVariation)) != B_OK)
		return status;
	return B_OK;
}


void
Leaves::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_DROP_RATE:
			fDropRate = fDropRateSlider->Value();
			break;
		case MSG_SET_LEAF_SIZE:
			fLeafSize = fLeafSizeSlider->Value();
			break;
		case MSG_SET_SIZE_VARIATION:
			fSizeVariation = fSizeVariationSlider->Value();
			break;
		default:
			BHandler::MessageReceived(message);
	}
}


void
Leaves::Draw(BView* view, int32 frame)
{
	float scale = fLeafSize / kLeafWidth / (kMaximumLeafSize * 2);
	scale *= view->Bounds().Width();
	scale += scale * drand48() * fSizeVariation / 100.;

	BAffineTransform transform;
	transform.TranslateBy(-kLeafWidth / 2, -kLeafHeight / 2);
		// draw the leaf centered on the point
	transform.RotateBy(drand48() * 2. * M_PI);
	if ((rand() & 64) == 0) transform.ScaleBy(-1., 1.);
		// flip half of the time
	transform.ScaleBy(scale);
	transform.TranslateBy(_RandomPoint(view->Bounds()));

	BPoint center = transform.Apply(BPoint(kLeafWidth / 2, kLeafHeight / 2));
	BPoint gradientOffset = BPoint(60 * scale, 80 * scale);
	BGradientLinear gradient(center - gradientOffset, center + gradientOffset);
	int color = (rand() / 7) % kColorCount;
	gradient.AddColor(kColors[color][0], 0.f);
	gradient.AddColor(kColors[color][1], 255.f);

	BShape leafShape;
	leafShape.MoveTo(transform.Apply(kLeafBegin));
	for (int i = 0; i < kLeafCurveCount; ++i) {
		BPoint control[3];
		for (int j = 0; j < 3; ++j)
			control[j] = transform.Apply(kLeafCurves[i][j]);
		leafShape.BezierTo(control);
	}
	leafShape.Close();

	view->PushState();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 0, 50);
	for (int i = 2; i >= 0; --i) {
		view->SetOrigin(i * 0.1, i * 0.3);
		view->SetPenSize(i * 2);
		view->StrokeShape(&leafShape);
	}
	view->PopState();
	view->FillShape(&leafShape, gradient);
}


inline BPoint
Leaves::_RandomPoint(const BRect& bound)
{
	return BPoint(drand48() * (bound.right - bound.left) + bound.left,
		drand48() * (bound.bottom - bound.top) + bound.top);
}

