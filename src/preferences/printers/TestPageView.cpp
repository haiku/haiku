/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "TestPageView.h"

#include <math.h>

#include <AffineTransform.h>
#include <Catalog.h>
#include <Font.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutUtils.h>
#include <TextView.h>
#include <Shape.h>
#include <String.h>
#include <StringView.h>

#include "PrinterListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TestPageView"


// #pragma mark LeafView


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
static const int kNumLeafCurves = sizeof(kLeafCurves) / sizeof(kLeafCurves[0]);
static const float kLeafWidth = 372.f;
static const float kLeafHeight = 121.f;


class LeafView : public BView {
public:
								LeafView();
	virtual	void				Draw(BRect updateRect);
};


LeafView::LeafView()
	:
	BView("leafview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	SetViewColor(255, 255, 255);
}


void
LeafView::Draw(BRect updateRect)
{
	float scale = Bounds().Width() / kLeafWidth;
	BAffineTransform transform;
	transform.ScaleBy(scale);

	// BGradientRadial gradient(BPoint(kLeafWidth * 0.75, kLeafHeight * 1.5),
	//	kLeafWidth * 2);
	BGradientLinear gradient(B_ORIGIN,
		transform.Apply(BPoint(kLeafWidth, kLeafHeight)));
	rgb_color lightBlue = make_color(6, 169, 255);
	rgb_color darkBlue = make_color(0, 50, 126);
	gradient.AddColor(darkBlue, 0.0);
	gradient.AddColor(lightBlue, 255.0);

	// build leaf shape
	BShape leafShape;
	leafShape.MoveTo(transform.Apply(kLeafBegin));
	for (int i = 0; i < kNumLeafCurves; ++i) {
		BPoint controlPoints[3];
		for (int j = 0; j < 3; ++j)
			controlPoints[j] = transform.Apply(kLeafCurves[i][j]);
		leafShape.BezierTo(controlPoints);
	}
	leafShape.Close();

	PushState();
	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(0, 0, 0, 50);
	for (int i = 2; i >= 0; --i) {
		SetOrigin(i * 0.1, i * 0.3);
		SetPenSize(i * 2);
		StrokeShape(&leafShape);
	}
	PopState();

	FillShape(&leafShape, gradient);
}


// #pragma mark -


class RadialLinesView : public BView {
public:
								RadialLinesView();

	virtual	void				Draw(BRect updateRect);

	virtual bool				HasHeightForWidth() { return true; }
	virtual void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);
};


RadialLinesView::RadialLinesView()
	: BView("radiallinesview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	SetViewColor(255, 255, 255);
}


void
RadialLinesView::GetHeightForWidth(float width,
	float* min, float* max, float* preferred)
{
	// Enforce a width/height ratio of 1

	if (min)
		*min = width;

	if (max)
		*max = width;

	if (preferred)
		*preferred = width;
}


void
RadialLinesView::Draw(BRect updateRect)
{
	const rgb_color black = { 0, 0, 0, 255 };
	const int angleStep = 4;

	BRect rect(Bounds());
	float size = rect.Width();
	if (size > rect.Height())
		size = rect.Height();
	size *= 0.45; // leave 10% of margin

	BPoint center(rect.Width() / 2, rect.Height() / 2);

	BeginLineArray(360 / angleStep);
	for (int i = 0; i < 360; i += angleStep) {
		double angle = i * M_PI / 180;
		BPoint pt(size * cos(angle), size * sin(angle));
		AddLine(center, center + pt, black);
	}
	EndLineArray();
}


// #pragma mark -


static const struct {
	const char* name;
	rgb_color	color;
} kColorGradients[] = {
	{ B_TRANSLATE_MARK("Red"), 		{255, 0, 0, 255} },
	{ B_TRANSLATE_MARK("Green"), 	{0, 255, 0, 255} },
	{ B_TRANSLATE_MARK("Blue"), 	{0, 0, 255, 255} },
	{ B_TRANSLATE_MARK("Yellow"), 	{255, 255, 0, 255} },
	{ B_TRANSLATE_MARK("Magenta"), 	{255, 0, 255, 255} },
	{ B_TRANSLATE_MARK("Cyan"), 	{0, 255, 255, 255} },
	{ B_TRANSLATE_MARK("Black"), 	{0, 0, 0, 255} }
};
static const int kNumColorGradients = sizeof(kColorGradients)
	/ sizeof(kColorGradients[0]);


class ColorGradientView : public BView {
public:
								ColorGradientView(rgb_color color);
	virtual	void				Draw(BRect updateRect);

private:
			rgb_color			fColor;
};


ColorGradientView::ColorGradientView(rgb_color color)
	:
	BView("colorgradientview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fColor(color)
{
}


void
ColorGradientView::Draw(BRect updateRect)
{
	BRect rect(Bounds());

	BGradientLinear gradient(rect.LeftTop(), rect.RightBottom());
	rgb_color white = make_color(255, 255, 255);
	gradient.AddColor(white, 0.0);
	gradient.AddColor(fColor, 255.0);

	FillRect(rect, gradient);
	StrokeRect(rect);
}


// #pragma mark -


TestPageView::TestPageView(BRect frame, PrinterItem* printer)
	: BView(frame, "testpage", B_FOLLOW_ALL,
		B_DRAW_ON_CHILDREN | B_FULL_UPDATE_ON_RESIZE),
	fPrinter(printer)
{
	SetViewColor(255, 255, 255);
}


void
TestPageView::AttachedToWindow()
{
	BTextView* statusView = new BTextView("statusView",
		be_plain_font, NULL, B_WILL_DRAW);

	statusView->SetInsets(10, 10, 10, 10);
	statusView->MakeEditable(false);
	statusView->MakeSelectable(false);

	const char* title = B_TRANSLATE("Test page");
	BString text;
	text << title << "\n\n";
	text << B_TRANSLATE(
		"Printer: %printer_name%\n"
		"Driver:  %driver%\n");

	text.ReplaceFirst("%printer_name%", fPrinter->Name());
	text.ReplaceFirst("%driver%", fPrinter->Driver());
	if (strlen(fPrinter->Transport()) > 0) {
		text << B_TRANSLATE("Transport: %transport% %transport_address%");

		text.ReplaceFirst("%transport%", fPrinter->Transport());
		text.ReplaceFirst("%transport_address%", fPrinter->TransportAddress());
	}

	statusView->SetText(text.String());
	BFont font;
	statusView->SetStylable(true);
	statusView->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.7);
	statusView->SetFontAndColor(0, strlen(title), &font);

	BGridLayoutBuilder gradiants(2.0);
	gradiants.View()->SetViewColor(B_TRANSPARENT_COLOR);

	for (int i = 0; i < kNumColorGradients; ++i) {
		BStringView* label = new BStringView(
			kColorGradients[i].name,
			B_TRANSLATE(kColorGradients[i].name));
		// label->SetAlignment(B_ALIGN_RIGHT);
		gradiants.Add(label, 0, i);
		gradiants.Add(new ColorGradientView(kColorGradients[i].color), 1, i);
	}

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(statusView)
			.Add(new LeafView())
		)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(gradiants, 0.60)
			.Add(new RadialLinesView(), 0.40)
		)
		.AddGlue()
		.End()
	);

	// set layout background color to transparent instead
	// of default UI panel color
	ChildAt(0)->SetViewColor(B_TRANSPARENT_COLOR);
}


void
TestPageView::DrawAfterChildren(BRect updateRect)
{
	// Draw corners marks

	float width = Bounds().Width();
	float height = Bounds().Height();
	float minDimension = MIN(width, height);

	float size = minDimension * 0.05;

	SetPenSize(3.0);

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
}
