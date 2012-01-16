/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>

#include "ALMLayout.h"
#include "ALMLayoutBuilder.h"


using namespace LinearProgramming;


class ViewsWindow : public BWindow {
public:
	ViewsWindow(BRect frame) 
		:
		BWindow(frame, "ALM Views", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		BButton* button1 = new BButton("BButton");
		BRadioButton* radioButton = new BRadioButton("BRadioButton", NULL);
		BButton* button3 = new BButton("3");
		BButton* button4 = new BButton("4");
		button4->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_CENTER));
		BButton* button5 = new BButton("5");
		BButton* button6 = new BButton("6");
		BTextView* textView1 = new BTextView("textView1");
		textView1->SetText("BTextView");
		BStatusBar* statusBar = new BStatusBar("BStatusBar", "label",
			"trailing label");
		statusBar->Update(50);

		BMenu* menu = new BMenu("Menu 1");
		BMenuField* menu1 = new BMenuField("MenuField 1", menu);
		menu->AddItem(new BPopUpMenu("Menu 1"));
		BStringView* stringView1 = new BStringView("string 1", "string 1");

		menu = new BMenu("Menu 2  + long text");
		BMenuField* menu2 = new BMenuField("MenuField 2", menu);
		menu->AddItem(new BPopUpMenu("Menu 2"));
		BStringView* stringView2 = new BStringView("string 2", "string 2");

		BALM::BALMLayout* layout = new BALMLayout(10, 10);
		BALM::BALMLayoutBuilder(this, layout)
			.SetInsets(10)
			.Add(button1, layout->Left(), layout->Top())
			.StartingAt(button1)
				.AddToRight(radioButton)
				.AddToRight(BSpaceLayoutItem::CreateGlue())
				.AddToRight(button3)
			.StartingAt(radioButton)
				.AddBelow(textView1, NULL, NULL, layout->RightOf(button3))
				.AddBelow(button4)
				.AddToRight(button5, layout->Right())
				.AddBelow(button6)
				.AddBelow(menu1, layout->AddYTab(), layout->Left(),
					layout->AddXTab())
				.AddToRight(stringView1)
				.AddToRight(BSpaceLayoutItem::CreateGlue(), layout->Right())
				.AddBelow(statusBar, NULL, layout->Left(), layout->Right())
			.StartingAt(statusBar)
				.AddBelow(menu2, layout->Bottom(), layout->Left(),
					layout->RightOf(menu1))
				.AddToRight(stringView2)
				.AddToRight(BSpaceLayoutItem::CreateGlue(), layout->Right());

		/*
		// create a new BALMLayout and use  it for this window
		BALMLayout* layout = new BALMLayout(10);
		SetLayout(layout);
		layout->SetInsets(10.);

		layout->AddView(button1, layout->Left(), layout->Top());
		layout->AddViewToRight(radioButton);
		layout->AddItemToRight(BSpaceLayoutItem::CreateGlue());
		Area* a3 = layout->AddViewToRight(button3);
		layout->SetCurrentArea(radioButton);
		layout->AddViewToBottom(textView1, NULL, NULL, a3->Right());
		layout->AddViewToBottom(button4);
		layout->AddViewToRight(button5, layout->Right());
		layout->AddViewToBottom(button6);

		layout->AddView(menu1, layout->Left(), layout->BottomOf(button6));
		layout->AddViewToRight(stringView1);
		layout->AddItemToRight(BSpaceLayoutItem::CreateGlue(), layout->Right());

		layout->AddViewToBottom(statusBar, NULL, layout->Left(),
			layout->Right());

		layout->AddView(menu2, layout->Left(), layout->BottomOf(statusBar),
			layout->RightOf(menu1), layout->Bottom());
		layout->AddViewToRight(stringView2);
		layout->AddItemToRight(BSpaceLayoutItem::CreateGlue(), layout->Right());
		*/

		layout->Solver()->AddConstraint(2, layout->TopOf(menu1), -1,
			layout->Bottom(), OperatorType(kEQ), 0);

		// test size limits
		BSize min = layout->MinSize();
		BSize max = layout->MaxSize();
		SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());
	}

};


class Views : public BApplication {
public:
	Views() 
		:
		BApplication("application/x-vnd.haiku.Views") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		ViewsWindow* window = new ViewsWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	Views app;
	app.Run();
	return 0;
}

