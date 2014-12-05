/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 * Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <Message.h>
#include <Picture.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "harness.h"


static const char* kAppSignature = "application/x-vnd.Haiku-Gradients";


// #pragma mark - Test1


class RadialGradientTest : public Test {
public:
	RadialGradientTest()
		:
		Test("Radial Gradient")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		// Draws two radial gradients with the same stops and different radiis,
		// and their enclosing circles. The gradients and circles should have
		// the same size.
		BGradientRadial g1(100, 100, 50);
		BGradientRadial g2(300, 100, 100);

		g1.AddColor(make_color(0,0,0,255), 0);
		g1.AddColor(make_color(255,255,255,255), 255);

		g2.AddColor(make_color(0,0,0,255), 0);
		g2.AddColor(make_color(255,255,255,255), 255);

		BRect r1(0, 0, 200, 200);
		BRect r2(200, 0, 400, 200);
		view->FillRect(r1, g1);
		view->FillRect(r2, g2);

		r1.InsetBy(50, 50);
		view->StrokeEllipse(r1);
		view->StrokeEllipse(r2);
	}
};


// Test for https://dev.haiku-os.org/ticket/2946
// Gradients with an alpha channel are not drawn properly
class AlphaGradientTest : public Test {
public:
	AlphaGradientTest()
		:
		Test("Alpha gradients")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		view->SetDrawingMode(B_OP_ALPHA);

		// These draw as almost transparent

		// Radial gradient
		BPoint center(50, 50);
		float radius = 50.0;
		BGradientRadial g(center, radius);
		g.AddColor((rgb_color){ 255, 0, 0, 255 }, 0.0);
		g.AddColor((rgb_color){ 0, 255, 0, 0 }, 255.0);
		view->FillEllipse(center, radius, radius, g);

		// Linear gradient - Horizontal
		BPoint from(100, 0);
		BPoint to(200, 0);
		BGradientLinear l(from, to);
		l.AddColor((rgb_color){ 255, 0, 0, 0 }, 0.0);
		l.AddColor((rgb_color){ 0, 255, 0, 255 }, 255.0);
		view->FillRect(BRect(100, 0, 200, 100), l);

		// Linear gradient - Vertical
		// (this uses a different code path in app_server)
		BPoint top(0, 0);
		BPoint bot(0, 100);
		BGradientLinear v(top, bot);
		v.AddColor((rgb_color){ 255, 0, 0, 0 }, 0.0);
		v.AddColor((rgb_color){ 0, 255, 0, 255 }, 255.0);
		view->FillRect(BRect(200, 0, 300, 100), v);

		// These draw as opaque or almost opaque
		
		view->SetOrigin(BPoint(0, 100));

		// Radial gradient
		BGradientRadial go(center, radius);
		go.AddColor((rgb_color){ 255, 0, 0, 0 }, 0.0);
		go.AddColor((rgb_color){ 0, 255, 0, 255 }, 255.0);
		view->FillEllipse(center, radius, radius, go);

		// Linear gradient - Horizontal
		BGradientLinear lo(from, to);
		lo.AddColor((rgb_color){ 255, 0, 0, 255 }, 0.0);
		lo.AddColor((rgb_color){ 0, 255, 0, 0 }, 255.0);
		view->FillRect(BRect(100, 0, 200, 100), lo);

		// Linear gradient - Vertical
		// (this uses a different code path in app_server)
		BGradientLinear vo(top, bot);
		vo.AddColor((rgb_color){ 255, 0, 0, 255 }, 0.0);
		vo.AddColor((rgb_color){ 0, 255, 0, 0 }, 255.0);
		view->FillRect(BRect(200, 0, 300, 100), vo);

		// Finally, do the same using ClipToPicture. This forces using an
		// unpacked scanline rasterizer, and it works.
		view->SetOrigin(BPoint(0, 0));

		view->SetDrawingMode(B_OP_COPY);

		BPicture picture;
		view->BeginPicture(&picture);
		view->SetHighColor(make_color(0,0,0,255));
		view->FillRect(BRect(0, 200, 300, 300));
		view->EndPicture();
		view->ClipToPicture(&picture);

		view->SetOrigin(BPoint(0, 200));

		view->SetDrawingMode(B_OP_ALPHA);

		view->FillEllipse(center, radius, radius, g);
		view->FillRect(BRect(100, 0, 200, 100), l);
		view->FillRect(BRect(200, 0, 300, 100), v);
	}
};


// Test for https://dev.haiku-os.org/ticket/2945
// Gradients with no stop at offset 0 or 255 draw random colors
class OutOfBoundsGradientTest : public Test {
public:
	OutOfBoundsGradientTest()
		:
		Test("Out of bounds gradients")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		{
			// Linear gradient - Vertical
			BPoint from(275, 10);
			BPoint to(275, 138);
			BGradientLinear l(from, to);
			l.AddColor((rgb_color){ 255, 0, 0, 255 }, 100.0);
			l.AddColor((rgb_color){ 255, 255, 0, 255 }, 156.0);
			view->FillRect(BRect(275, 10, 2*265, 265), l);
		}

		{
			// Linear gradient - Horizontal
			BPoint from(10, 10);
			BPoint to(138, 10);
			BGradientLinear l(from, to);
			l.AddColor((rgb_color){ 255, 0, 0, 255 }, 100.0);
			l.AddColor((rgb_color){ 255, 255, 0, 255 }, 156.0);
			view->FillRect(BRect(10, 10, 265, 265), l);
		}
	}
};


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow("Gradient tests");

	window->AddTest(new RadialGradientTest());
	window->AddTest(new AlphaGradientTest());
	window->AddTest(new OutOfBoundsGradientTest());

	window->SetToTest(0);
	window->Show();

	app.Run();
	return 0;
}
