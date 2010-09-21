#include <Application.h>
#include <Button.h>
#include <List.h>
#include <Window.h>

// include this for ALM
#include "XTab.h"
#include "YTab.h"
#include "Area.h"
#include "BALMLayout.h"


class HelloWorldWindow : public BWindow {
public:
	HelloWorldWindow(BRect frame) 
		: BWindow(frame, "ALM Hello World",
			B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("Hello World!");

		// create a new BALMLayout and use  it for this window
		fLayout = new BALMLayout();
		SetLayout(fLayout);

		// add an area containing the button
		// use the borders of the layout as the borders for the area
		Area* a = fLayout->AddArea(
			fLayout->Left(), fLayout->Top(), 
			fLayout->Right(), fLayout->Bottom(),
			button1);
		button1->SetExplicitMinSize(BSize(0, 50));
		button1->SetExplicitMaxSize(BSize(500, 500));

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
		: BApplication("application/x-vnd.haiku.HelloWorld") 
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
