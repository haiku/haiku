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


class ComplexButtonsWindow : public BWindow {
public:
	ComplexButtonsWindow(BRect frame) 
		:
		BWindow(frame, "ALM Complex Buttons", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
	{
		BButton* button1 = new BButton("A");
		BButton* button2 = new BButton("B");
		BButton* button3 = new BButton("GHIJ");

		BButton* button4 = new BButton("abcdefghijklmnop");
		BButton* button5 = new BButton("Foo");

		const int32 kOffset = 80;

		button1->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button1->SetExplicitMaxSize(BSize(500, 500));
		button1->SetExplicitPreferredSize(BSize(10 + kOffset, B_SIZE_UNSET));

		button2->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button2->SetExplicitMaxSize(BSize(500, 500));
		button2->SetExplicitPreferredSize(BSize(10 + kOffset, B_SIZE_UNSET));

		button3->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button3->SetExplicitMaxSize(BSize(500, 500));
		button3->SetExplicitPreferredSize(BSize(40 + kOffset, B_SIZE_UNSET));

		button4->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button4->SetExplicitMaxSize(BSize(500, 500));
		button4->SetExplicitPreferredSize(BSize(160 + kOffset, B_SIZE_UNSET));
		
		button5->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		button5->SetExplicitMaxSize(BSize(500, 500));
		button5->SetExplicitPreferredSize(BSize(30 + kOffset, B_SIZE_UNSET));

		BALMLayout* fLayout = new BALMLayout(10, 10);
		BALM::BALMLayoutBuilder(this, fLayout)
			.Add(button1, fLayout->Left(), fLayout->Top())
			.StartingAt(button1)
				.AddToRight(button2)
				.AddToRight(button3, fLayout->Right())
			.AddBelow(button4, fLayout->Bottom(), fLayout->Left(),
				fLayout->AddXTab())
			.AddToRight(button5, fLayout->Right());
				
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
	BApplication app("application/x-vnd.haiku.ComplexButtons");

	BRect frameRect;
	frameRect.Set(100, 100, 600, 300);
	ComplexButtonsWindow* window = new ComplexButtonsWindow(frameRect);
	window->Show();

	app.Run();
	return 0;
}
