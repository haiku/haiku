/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "BackgroundsView.h"

#include <Application.h>
#include <TrackerAddOn.h>
#include <Window.h>


static const char *kSignature = "application/x-vnd.haiku-backgrounds";


class BackgroundsApplication : public BApplication {
	public:
		BackgroundsApplication();
};

class BackgroundsWindow : public BWindow {
	public:
		BackgroundsWindow(BRect frame, bool standalone = true);

		void ProcessRefs(entry_ref dir, BMessage* refs);

	protected:
		virtual	bool QuitRequested();
		virtual void WorkspaceActivated(int32 oldWorkspaces, bool active);

		BackgroundsView *fBackgroundsView;
		bool fIsStandalone;
};


//	#pragma mark -


BackgroundsApplication::BackgroundsApplication()
	: BApplication(kSignature)
{
	BWindow* window = new BackgroundsWindow(BRect(100, 100, 570, 325));
	window->Show();
}


//	#pragma mark -


BackgroundsWindow::BackgroundsWindow(BRect frame, bool standalone)
	: BWindow(frame, "Backgrounds", B_TITLED_WINDOW, 
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
		fIsStandalone(standalone)
{
	fBackgroundsView = new BackgroundsView(Bounds(), "BackgroundsView",
		B_FOLLOW_ALL, B_WILL_DRAW);
	fBackgroundsView->ResizeToPreferred();
	ResizeTo(fBackgroundsView->Bounds().Width(), fBackgroundsView->Bounds().Height());

	AddChild(fBackgroundsView);

	// TODO: center on screen
	// TODO: save window position?
}


bool
BackgroundsWindow::QuitRequested()
{
	fBackgroundsView->SaveSettings();
	if (fIsStandalone)
		be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
BackgroundsWindow::WorkspaceActivated(int32 oldWorkspaces, bool active)
{
	fBackgroundsView->WorkspaceActivated(oldWorkspaces, active);
}


void 
BackgroundsWindow::ProcessRefs(entry_ref dir, BMessage* refs)
{
	fBackgroundsView->ProcessRefs(dir, refs);
}


//	#pragma mark -


/*!
	\brief Tracker add-on entry
*/
void
process_refs(entry_ref dir, BMessage* refs, void* /*reserved*/)
{
	BackgroundsWindow* window = new BackgroundsWindow(BRect(100, 100, 570, 325), false);
	window->ProcessRefs(dir, refs);
	snooze(500);
	window->Show();

	status_t status;
	wait_for_thread(window->Thread(), &status);
}


int
main(int argc, char** argv)
{
	BApplication* app = new BackgroundsApplication;

	// This function doesn't return until the application quits
	app->Run();
	delete app;

	return 0;
}


