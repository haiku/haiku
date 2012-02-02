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
#include <Message.h>
#include <StringView.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"
#include "ALMLayoutBuilder.h"
#include "LinearProgrammingTypes.h"


class FriendWindow : public BWindow {
public:
	FriendWindow(BRect frame) 
		:
		BWindow(frame, "ALM Friend Test", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS),
		fLayout2(NULL),
		fBoom(NULL),
		fLeft(NULL),
		fTop(NULL),
		fRight(NULL),
		fBottom(NULL)
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

		fLayout2 = new BALMLayout(10, 10, layout1);
		BView* almView2 = _MakeALMView(fLayout2);

		BALM::BALMLayoutBuilder(fLayout2)
			.Add(button4, fLayout2->Left(), fLayout2->Top(), xTabs[0])
			.StartingAt(button4)
				.AddBelow(button5, NULL, xTabs[1], fLayout2->Right())
				.AddBelow(button6, fLayout2->Bottom(), xTabs[0]);

		fLeft = fLayout2->Left();
		fBottom = fLayout2->BottomOf(button5);
		fTop = fLayout2->BottomOf(button4);
		fRight = xTabs[1];

		layout1->AreaFor(button2)->SetContentAspectRatio(1.0f);
		fLayout2->Solver()->AddConstraint(-1.0f, xTabs[0], 1.0f, xTabs[1],
			LinearProgramming::kGE, 20.0f);

		BButton* archiveButton = new BButton("clone", new BMessage('arcv'));
		archiveButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
				B_SIZE_UNSET));
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.Add(almView1->GetLayout())
			.Add(almView2->GetLayout())
			.Add(archiveButton);
	}


	void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case 'BOOM':
				if (!fBoom) {
					fBoom = _MakeButton("BOOM");
					fLayout2->AddView(fBoom, fLeft, fTop,
						fRight, fBottom);
				} else {
					if (fBoom->IsHidden(fBoom))
						fBoom->Show();
					else
						fBoom->Hide();
				}
				break;
			case 'arcv': {
				BView* view = GetLayout()->View();
				BMessage archive;
				status_t err = view->Archive(&archive, true);
				BWindow* window = new BWindow(BRect(30, 30, 400, 400),
					"ALM Friend Test Clone", B_TITLED_WINDOW,
					B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS);
				window->SetLayout(new BGroupLayout(B_VERTICAL));
				BView* clone;
				if (err == B_OK)
					err = BUnarchiver::InstantiateObject(&archive, clone);
				if (err != B_OK)
					window->AddChild(new BStringView("", "An error occurred!"));
				else {
					window->AddChild(clone);
				}
				window->Show();

				break;
			}
			default:
				BWindow::MessageReceived(message);
		}
	}

private:
	BButton* _MakeButton(const char* label)
	{
		BButton* button = new BButton(label, new BMessage('BOOM'));
		button->SetExplicitMinSize(BSize(10, 50));
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
	
	BALMLayout* fLayout2;
	BButton*	fBoom;
	XTab*		fLeft;
	YTab*		fTop;
	XTab*		fRight;
	YTab*		fBottom;
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

