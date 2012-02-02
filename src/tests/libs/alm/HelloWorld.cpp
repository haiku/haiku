/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <List.h>
#include <StringView.h>
#include <Window.h>

// include this for ALM
#include "ALMLayout.h"


const uint32 kMsgClone = 'clne';


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
		BView* view = new BView("alm view", 0, fLayout);
		fLayout->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_USE_FULL_HEIGHT));
		SetLayout(new BGroupLayout(B_VERTICAL));
		AddChild(view);

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

		AddShortcut('c', B_COMMAND_KEY, new BMessage(kMsgClone));
	}

	void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kMsgClone:
			{
				BView* view = fLayout->View();
				BMessage archive;
				view->Archive(&archive, true);
				BWindow* window = new BWindow(BRect(30, 30, 100, 100),
					"ALM HelloWorld Clone", B_TITLED_WINDOW,
					B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS);
				window->SetLayout(new BGroupLayout(B_VERTICAL));
				BView* clone;
				status_t err = BUnarchiver::InstantiateObject(&archive, clone);
				if (err != B_OK)
					window->AddChild(new BStringView("", "An error occurred!"));
				else
					window->AddChild(clone);
				window->Show();

				break;
			}
			default:
				BWindow::MessageReceived(message);
		}
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
