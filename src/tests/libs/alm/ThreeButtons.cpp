/*
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include <Application.h>
#include <Button.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"
#include "ALMLayoutBuilder.h"


class ThreeButtonsWindow : public BWindow {
public:
	ThreeButtonsWindow(BRect frame) 
		:
		BWindow(frame, "ALM Three Buttons", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
	{
		BButton* button1 = new BButton("A");
		BButton* button2 = new BButton("B");
		BButton* button3 = new BButton("C");

		button1->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button1->SetExplicitMaxSize(BSize(500, 50));

		button2->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button2->SetExplicitMaxSize(BSize(500, 500));

		button3->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button3->SetExplicitMaxSize(BSize(500, 500));

		fLayout = new BALMLayout(0, 0);
		BALM::BALMLayoutBuilder(this, fLayout)
			.Add(button1, fLayout->Left(), fLayout->Top(), fLayout->Right())
			.StartingAt(button1)
				.AddBelow(button2)
				.AddBelow(button3, fLayout->Bottom());

		/*
		// create a new BALMLayout and use  it for this window
		fLayout = new BALMLayout();
		SetLayout(fLayout);
		
		fLayout->AddView(button1, fLayout->Left(), fLayout->Top(),
			fLayout->Right(), NULL);
		fLayout->AddViewToBottom(button2);
		fLayout->AddViewToBottom(button3, fLayout->Bottom());
		*/

		// test size limits
		BSize min = fLayout->MinSize();
		BSize max = fLayout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());
	}
	
private:
	BALMLayout* fLayout;

};


int
main()
{
	BApplication app("application/x-vnd.haiku.ThreeButtons");

	BRect frameRect;
	frameRect.Set(100, 100, 600, 300);
	ThreeButtonsWindow* window = new ThreeButtonsWindow(frameRect);
	window->Show();

	app.Run();
	return 0;
}
