#include <Application.h>
#include <TextView.h>
#include <Window.h>

class window : public BWindow {
public:
	window() : BWindow(BRect(30, 30, 300, 300), "BTextView test", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		BTextView *textview = new BTextView(Bounds(), "textview", Bounds(),
											B_FOLLOW_ALL, B_WILL_DRAW);
		AddChild(textview);
		textview->SetText("Type into the Haiku BTextView!");
		textview->MakeFocus();
	}
	
};

class application : public BApplication {
public:
	application()
		:BApplication("application/x-vnd.test")
	{
	}
	
	virtual void ReadyToRun()
	{
		(new window())->Show();
	}
	
};

int main()
{
	application app;
	app.Run();
	return 0;
}
