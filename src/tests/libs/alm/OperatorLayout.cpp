/*
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Button.h>
#include <ControlLook.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "ALMLayout.h"


class OperatorWindow : public BWindow {
public:
	OperatorWindow(BRect frame) 
		:
		BWindow(frame, "ALM Operator", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		BButton* button1 = new BButton("1");
		BButton* button2 = new BButton("2");
		BButton* button3 = new BButton("3");
		BButton* button4 = new BButton("4");
		BButton* button5 = new BButton("5");

		button1->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button2->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button3->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button4->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		button5->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

		// create a new BALMLayout and use  it for this window
		float spacing = be_control_look->DefaultItemSpacing();
		BALMLayout* layout = new BALMLayout(spacing);
		SetLayout(layout);
		layout->SetInsets(spacing);

		GroupItem item = GroupItem(button1) | (GroupItem(button2)
			/ (GroupItem(button3) | GroupItem(BSpaceLayoutItem::CreateGlue())
					| GroupItem(button4))
			/ GroupItem(button5));
		layout->BuildLayout(item);

		// test size limits
		BSize min = layout->MinSize();
		BSize max = layout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());
	}

};


int
main()
{
	BApplication app("application/x-vnd.haiku.ALMOperator");

	OperatorWindow* window = new OperatorWindow(BRect(100, 100, 300, 300));
	window->Show();
		
	app.Run();
	return 0;
}

