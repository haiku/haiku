#include <Application.h>
#include <Button.h>
#include <TextView.h>
#include <List.h>
#include <Window.h>

#include "BALMLayout.h"


class TwoViewsWindow : public BWindow {
public:
	TwoViewsWindow(BRect frame) 
		:
		BWindow(frame, "ALM Two Views", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		button1 = new BButton("View 1");
		textView1 = new BTextView("textView1");
		textView1->SetText("View 2");

		// create a new BALMLayout and use  it for this window
		BALMLayout* layout = new BALMLayout();
		SetLayout(layout);

		// create an extra tab
		XTab* x1 = layout->AddXTab();

		Area* a1 = layout->AddArea(
			layout->Left(), layout->Top(), 
			x1, layout->Bottom(),
			button1);
		Area* a2 = layout->AddArea(
			x1, layout->Top(), 
			layout->Right(), layout->Bottom(),
			textView1);

		// add a constraint: 2*x1 == right
		// i.e. x1 is in the middle of the layout 
		layout->Solver()->AddConstraint(2, x1, -1, layout->Right(),
			OperatorType(EQ), 0);
	}
	
private:
	BButton* button1;
	BTextView* textView1;
};


class TwoViews : public BApplication {
public:
	TwoViews() 
		:
		BApplication("application/x-vnd.haiku.TwoViews") 
	{
		BRect frameRect;
		frameRect.Set(100, 100, 300, 300);
		TwoViewsWindow* window = new TwoViewsWindow(frameRect);
		window->Show();
	}
};


int
main()
{
	TwoViews app;
	app.Run();
	return 0;
}

