#include <Screen.h>
#include <Window.h>

#include <stdio.h>
#include <unistd.h>

#include "ThemesApp.h"
#include "ThemeInterfaceView.h"

const char *kThemesAppSig = "application/x-vnd.mmu_man-Themes";


ThemesApp::ThemesApp()
	: BApplication(kThemesAppSig)
{
}


ThemesApp::~ThemesApp()
{
}


void
ThemesApp::ReadyToRun()
{
	BScreen s;
	BRect frame(0, 0, 500, 300);
	frame.OffsetBySelf(s.Frame().Width()/2 - frame.Width()/2, 
						s.Frame().Height()/2 - frame.Height()/2);
	BWindow *w = new BWindow(frame, "Themes", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_QUIT_ON_WINDOW_CLOSE);
	ThemeInterfaceView *v = new ThemeInterfaceView(w->Bounds());
	w->AddChild(v);
	w->Show();
}


void
ThemesApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
	default:
		BApplication::MessageReceived(message);
	}
}


int main(int argc, char **argv)
{
	ThemesApp app;
	app.Run();
}
