#include "ScrollBarApp.h"
#include "ScrollBarWindow.h"

ScrollBarApp * scroll_bar_app = 0;

ScrollBarApp::ScrollBarApp()
	: BApplication("application/x-vnd.obos.scroll-bar")
{
	window = new ScrollBarWindow();
	scroll_bar_app = this;
}

ScrollBarApp::~ScrollBarApp()
{
}

void
ScrollBarApp::ReadyToRun()
{
	if (window) {
		window->Show();
	}
}
