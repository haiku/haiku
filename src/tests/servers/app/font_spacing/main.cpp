/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <GradientLinear.h>
#include <Message.h>
#include <Picture.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>


static const char* kAppSignature = "application/x.vnd-Haiku.FontSpacing";


class TestView : public BView {
public:
								TestView();
	virtual						~TestView();

	virtual	void				Draw(BRect updateRect);
};


TestView::TestView()
	:
	BView(NULL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
}


TestView::~TestView()
{
}


void
TestView::Draw(BRect updateRect)
{
	float scale = Bounds().Width() / 400.0f;

	if (scale < 0.25f)
		scale = 0.25f;
	else if (scale > 3.0f)
		scale = 3.0f;

	SetScale(scale);

	const char* string = "Testing the various BFont spacing modes.";

	BFont font;
	SetFont(&font);
	DrawString(string, BPoint(30.0f, 25.0f));

	font.SetSpacing(B_STRING_SPACING);
	SetFont(&font);
	DrawString(string, BPoint(30.0f, 45.0f));

	font.SetSpacing(B_CHAR_SPACING);
	SetFont(&font);
	DrawString(string, BPoint(30.0f, 65.0f));
}


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	BWindow* window = new BWindow(BRect(50, 50, 450, 450), "Font spacing test",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_QUIT_ON_WINDOW_CLOSE
			| B_AUTO_UPDATE_SIZE_LIMITS);

	BLayoutBuilder::Group<>(window)
		.Add(new TestView())
	;

	window->Show();

	app.Run();
	return 0;
}
