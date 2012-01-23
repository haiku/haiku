/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Window.h>

#include <stdio.h>

// include this for ALM
#include "ALMLayout.h"


class BAlertPolicy : public BALMLayout::BadLayoutPolicy {
	virtual bool OnBadLayout(BALMLayout* layout, ResultType result,
		BLayoutContext* context)
	{
		BAlert* alert = new BAlert("layout failure", "layout failed!",
							"sure");
		alert->Go();
		return true;
	}
};


class BadLayoutWindow : public BWindow {
public:
	BadLayoutWindow(BRect frame) 
		:
		BWindow(frame, "ALM Bad Layout", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("Bad Layout!");

		// create a new BALMLayout and use  it for this window
		fLayout = new BALMLayout();
		SetLayout(fLayout);

		// add an area containing the button
		// use the borders of the layout as the borders for the area
		fLayout->AddView(button1, fLayout->Left(), fLayout->Top(),
			fLayout->Right(), fLayout->Bottom());
		button1->SetExplicitMinSize(BSize(50, 50));
		button1->SetExplicitMaxSize(BSize(500, 500));
		button1->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));


		BButton* button2 = new BButton("can't fit!");
		fLayout->AddView(button2, fLayout->RightOf(button1), fLayout->Top(),
			fLayout->Right(), fLayout->Bottom());
		button2->SetExplicitMinSize(BSize(50, 50));

		fLayout->SetBadLayoutPolicy(new BAlertPolicy());
		// test size limits
		BSize min = fLayout->MinSize();
		BSize max = fLayout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());

		printf("moving to standard BALMLayout BadLayoutPolicy\n");
		fLayout->SetBadLayoutPolicy(NULL);
	}
	
private:
	BALMLayout* fLayout;
	BButton* button1;
};


class BadLayout : public BApplication {
public:
	BadLayout() 
		:
		BApplication("application/x-vnd.haiku.BadLayout") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		BadLayoutWindow* window = new BadLayoutWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	BadLayout app;
	app.Run();
	return 0;
}

