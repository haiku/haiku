/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <TextView.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"


class PinwheelWindow : public BWindow {
public:
	PinwheelWindow(BRect frame) 
		: BWindow(frame, "ALM Pinwheel",
			B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("1");
		button2 = new BButton("2");
		button3 = new BButton("3");
		button4 = new BButton("4");
		textView1 = new BTextView("textView1");
		textView1->SetText("5");	

		button1->SetExplicitMinSize(BSize(0, 0));
		button1->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button2->SetExplicitMinSize(BSize(0, 0));
		button2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button3->SetExplicitMinSize(BSize(0, 0));
		button3->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button4->SetExplicitMinSize(BSize(0, 0));
		button4->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

		// create a new BALMLayout and use  it for this window
		BALMLayout* layout = new BALMLayout();
		SetLayout(layout);

		// create extra tabs
		XTab* x1 = layout->AddXTab();
		XTab* x2 = layout->AddXTab();
		YTab* y1 = layout->AddYTab();
		YTab* y2 = layout->AddYTab();

		Area* a1 = layout->AddArea(
			layout->Left(), layout->Top(), 
			x2, y1,
			button1);
		Area* a2 = layout->AddArea(
			x2, layout->Top(), 
			layout->Right(), y2,
			button2);
		Area* a3 = layout->AddArea(
			x1, y2, 
			layout->Right(), layout->Bottom(),
			button3);
		Area* a4 = layout->AddArea(
			layout->Left(), y1, 
			x1, layout->Bottom(),
			button4);
		Area* a5 = layout->AddArea(
			x1, y1,
			x2, y2,
			textView1);

		a1->HasSameSizeAs(a3);
	}
	
private:
	BButton* button1;
	BButton* button2;
	BButton* button3;
	BButton* button4;
	BTextView* textView1;
};


class Pinwheel : public BApplication {
public:
	Pinwheel() 
		: BApplication("application/x-vnd.haiku.Pinwheel") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		PinwheelWindow* window = new PinwheelWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	Pinwheel app;
	app.Run();
	return 0;
}

