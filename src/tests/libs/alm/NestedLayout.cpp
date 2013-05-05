/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2012, Haiku, Inc. 
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"


class NestedLayoutWindow : public BWindow {
public:
	NestedLayoutWindow(BRect frame) 
		:
		BWindow(frame, "ALM Nested Layout", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		button1 = new BButton("There should be space above this button!");

		fLayout = new BALMLayout();
		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.SetInsets(0)
			.AddStrut(30)
			.Add(fLayout);

		// add an area containing the button
		// use the borders of the layout as the borders for the area
		fLayout->AddView(button1, fLayout->Left(), fLayout->Top(),
			fLayout->Right(), fLayout->Bottom());
		button1->SetExplicitMinSize(BSize(0, 50));
		button1->SetExplicitMaxSize(BSize(500, 500));
		button1->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
	}
	
private:
	BALMLayout* fLayout;
	BButton* button1;
};


class NestedLayout : public BApplication {
public:
	NestedLayout() 
		:
		BApplication("application/x-vnd.haiku.NestedLayout") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		NestedLayoutWindow* window = new NestedLayoutWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	NestedLayout app;
	app.Run();
	return 0;
}
