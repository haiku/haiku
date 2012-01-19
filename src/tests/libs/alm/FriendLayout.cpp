/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"
#include "ALMLayoutBuilder.h"


class FriendWindow : public BWindow {
public:
	FriendWindow(BRect frame) 
		:
		BWindow(frame, "ALM Friend Test", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		BButton* button1 = _MakeButton("friends!");
		BButton* button2 = _MakeButton("friends!");
		BButton* button3 = _MakeButton("friends!");

		BButton* button4 = _MakeButton("friends!");
		BButton* button5 = _MakeButton("friends!");
		BButton* button6 = _MakeButton("friends!");

		BALMLayout* layout1 = new BALMLayout(10, 10);
		BView* almView1 = _MakeALMView(layout1);

		BReference<XTab> xTabs[2];
		layout1->AddXTabs(xTabs, 2);

		BALM::BALMLayoutBuilder(layout1)
			.Add(button1, layout1->Left(), layout1->Top(),
				xTabs[0], layout1->Bottom())
			.StartingAt(button1)
				.AddToRight(button2, xTabs[1])
				.AddToRight(button3, layout1->Right());

		BALMLayout* layout2 = new BALMLayout(10, 10, layout1);
		BView* almView2 = _MakeALMView(layout2);

		BALM::BALMLayoutBuilder(layout2)
			.Add(button4, layout2->Left(), layout2->Top(), xTabs[0])
			.StartingAt(button4)
				.AddBelow(button5, NULL, xTabs[0], xTabs[1])
				.AddBelow(button6, layout2->Bottom(),
					xTabs[0], layout2->Right());

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.Add(almView1)
			.Add(almView2);
	}

private:
	BButton* _MakeButton(const char* label)
	{
		BButton* button = new BButton(label);
		button->SetExplicitMinSize(BSize(0, 50));
		button->SetExplicitMaxSize(BSize(500, 500));
		button->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		return button;
	}

	BView* _MakeALMView(BALMLayout* layout)
	{
		BView* view = new BView(NULL, 0, layout);
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		return view;
	}
	
	BALMLayout* fLayout;
	BButton* button1;
};


class Friend : public BApplication {
public:
	Friend() 
		:
		BApplication("application/x-vnd.haiku.Friend") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		FriendWindow* window = new FriendWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	Friend app;
	app.Run();
	return 0;
}

