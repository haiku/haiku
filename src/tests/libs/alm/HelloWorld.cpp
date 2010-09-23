/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"


class HelloWorldWindow : public BWindow {
public:
	HelloWorldWindow(BRect frame) 
		:
		BWindow(frame, "ALM Hello World", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("Hello World!");

		// create a new BALMLayout and use  it for this window
		fLayout = new BALMLayout();
		SetLayout(fLayout);

		// add an area containing the button
		// use the borders of the layout as the borders for the area
		fLayout->AddView(button1, fLayout->Left(), fLayout->Top(),
			fLayout->Right(), fLayout->Bottom());
		button1->SetExplicitMinSize(BSize(0, 50));
		button1->SetExplicitMaxSize(BSize(500, 500));
		button1->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));

		// test size limits
		BSize min = fLayout->MinSize();
		BSize max = fLayout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());
	}
	
private:
	BALMLayout* fLayout;
	BButton* button1;
};


class HelloWorld : public BApplication {
public:
	HelloWorld() 
		:
		BApplication("application/x-vnd.haiku.HelloWorld") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		HelloWorldWindow* window = new HelloWorldWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	HelloWorld app;
	app.Run();
	return 0;
}
