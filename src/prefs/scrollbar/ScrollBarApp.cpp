#include <ScrollBarApp.h>
#include <ScrollBarWindow.h>

ScrollBarApp::ScrollBarApp()
	: BApplication("application/x-vnd.obos.scroll-bar")
{
	window = new ScrollBarWindow();
}

ScrollBarApp::~ScrollBarApp()
{
	delete window;
}

void
ScrollBarApp::ReadyToRun()
{
	if (window) {
		window->Show();
	}
}
