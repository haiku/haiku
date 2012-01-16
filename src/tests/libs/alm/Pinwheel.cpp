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
#include "ALMLayoutBuilder.h"


class PinwheelWindow : public BWindow {
public:
	PinwheelWindow(BRect frame) 
		:
		BWindow(frame, "ALM Pinwheel", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("1");
		button2 = new BButton("2");
		button3 = new BButton("3");
		button4 = new BButton("4");
		textView1 = new BTextView("textView1");
		textView1->SetText("5");	

		button1->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button3->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button4->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

		// create a new BALMLayout and use  it for this window
		BALMLayout* layout = new BALMLayout(10, 10);

		BReference<XTab> xTabs[2];
		BReference<YTab> yTabs[2];
		layout->AddXTabs(xTabs, 2);
		layout->AddYTabs(yTabs, 2);

		BALM::BALMLayoutBuilder(this, layout)
			.SetInsets(5)
			.Add(textView1, xTabs[0], yTabs[0], xTabs[1], yTabs[1])
			.StartingAt(textView1)
				.AddAbove(button1, layout->Top(), layout->Left())
				.AddToRight(button2, layout->Right(), NULL, yTabs[1])
				.AddBelow(button3, layout->Bottom(), xTabs[0])
				.AddToLeft(button4, layout->Left(),  yTabs[0]);

		// alternative setup
		/*
		SetLayout(layout);

		layout->SetInsets(5.);

		// create extra tabs
		BReference<XTab> x1 = layout->AddXTab();
		BReference<XTab> x2 = layout->AddXTab();
		BReference<YTab> y1 = layout->AddYTab();
		BReference<YTab> y2 = layout->AddYTab();

		layout->AddView(button1, layout->Left(), layout->Top(), x2,
			y1);
		layout->AddView(button2, x2, layout->Top(), layout->Right(), y2);
		layout->AddView(button3, x1, y2, layout->Right(),
			layout->Bottom());
		layout->AddView(button4, layout->Left(), y1, x1, layout->Bottom());
		layout->AddView(textView1, x1, y1, x2, y2);
		*/

		// test size limits
		BSize min = layout->MinSize();
		BSize max = layout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());
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
		:
		BApplication("application/x-vnd.haiku.Pinwheel") 
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

