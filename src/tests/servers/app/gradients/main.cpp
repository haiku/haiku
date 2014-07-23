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
#include <GradientRadial.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <Message.h>
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


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow("Gradient tests");

	window->AddTest(new RadialGradientTest());

	window->SetToTest(0);
	window->Show();

	app.Run();
	return 0;
}
