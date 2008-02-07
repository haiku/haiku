#include <Application.h>
#include <Archivable.h>
#include <Box.h>
#include <Dragger.h>
#include <Message.h>
#include <Path.h>
#include <Shelf.h>
#include <Window.h>

#include <stdio.h>

#include "TermConst.h"
#include "TermView.h"

class App : public BApplication {
public:
	App();
};


class Window : public BWindow {
public:
	Window();
	void AttachTermView();
private:
	BShelf *fShelf;
};


int main()
{
	App app;
	app.Run();
	return 0;
}


// App
App::App()
	:BApplication("application/x-vnd-terminal-replicant")
{
	Window *window = new Window();
	window->Show();
}


// Window
Window::Window()
	:BWindow(BRect(100, 100, 400, 360), "RepliTerminal",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS|B_QUIT_ON_WINDOW_CLOSE)
{
	AttachTermView();
}


void
Window::AttachTermView()
{
	// BMessage containing the class name and the app_signature
	// for Terminal and TermView
	BMessage message;				
	message.AddString("class", "TermView");
	message.AddString("add_on", TERM_SIGNATURE);	
		
	BView *termView = dynamic_cast<BView *>(instantiate_object(&message));
	
	if (termView != NULL) {
		termView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(termView);
		
		termView->ResizeToPreferred();
	}
}
