/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <TrackerAddOnAppLaunch.h>
#include <Window.h>

#include "BackgroundsView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main Window"


static const char* kSignature = "application/x-vnd.Haiku-Backgrounds";


class BackgroundsWindow : public BWindow {
public:
							BackgroundsWindow();

			void			RefsReceived(BMessage* message);

protected:
	virtual	bool			QuitRequested();
	virtual	void			WorkspaceActivated(int32 oldWorkspaces,
								bool active);

			BackgroundsView*	fBackgroundsView;
};


class BackgroundsApplication : public BApplication {
public:
							BackgroundsApplication();
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			RefsReceived(BMessage* message);

private:
			BackgroundsWindow*	fWindow;
};


//	#pragma mark -


BackgroundsApplication::BackgroundsApplication()
	:
	BApplication(kSignature),
	fWindow(NULL)
{
	fWindow = new BackgroundsWindow();
	fWindow->Show();
}


void
BackgroundsApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			fWindow->Activate();
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
BackgroundsApplication::RefsReceived(BMessage* message)
{
	fWindow->RefsReceived(message);
}


//	#pragma mark -


BackgroundsWindow::BackgroundsWindow()
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Backgrounds"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES)
{
	fBackgroundsView = new BackgroundsView();

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fBackgroundsView)
			.End()
		.End();

	if (!fBackgroundsView->FoundPositionSetting())
		CenterOnScreen();
}


void
BackgroundsWindow::RefsReceived(BMessage* message)
{
	fBackgroundsView->RefsReceived(message);
	Activate();
}


bool
BackgroundsWindow::QuitRequested()
{
	fBackgroundsView->SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
BackgroundsWindow::WorkspaceActivated(int32 oldWorkspaces, bool active)
{
	fBackgroundsView->WorkspaceActivated(oldWorkspaces, active);
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	BackgroundsApplication app;
	app.Run();
	return 0;
}

