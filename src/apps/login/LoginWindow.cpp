#include "LoginWindow.h"
#include "LoginView.h"

LoginWindow::LoginWindow(BRect frame)
	: BWindow(frame, "Welcome to Haiku", B_TITLED_WINDOW_LOOK, 
		B_NORMAL_WINDOW_FEEL/*B_FLOATING_ALL_WINDOW_FEEL*/, 
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | 
		B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | 
		B_ASYNCHRONOUS_CONTROLS)
{
	LoginView *v = new LoginView(Bounds());
	AddChild(v);
	SetPulseRate(1000000LL);
}


LoginWindow::~LoginWindow()
{
}


