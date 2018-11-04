/*
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "StartTeamWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "AppMessageCodes.h"
#include "UserInterface.h"


enum {
	MSG_BROWSE_TEAM		= 'brte',
	MSG_SET_TEAM_PATH	= 'setp'
};


StartTeamWindow::StartTeamWindow(TargetHostInterface* hostInterface)
	:
	BWindow(BRect(), "Start new team", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fGuideText(NULL),
	fTeamTextControl(NULL),
	fArgumentsTextControl(NULL),
	fBrowseTeamButton(NULL),
	fBrowseTeamPanel(NULL),
	fStartButton(NULL),
	fCancelButton(NULL),
	fTargetHostInterface(hostInterface)
{
}


StartTeamWindow::~StartTeamWindow()
{
	delete fBrowseTeamPanel;
}


StartTeamWindow*
StartTeamWindow::Create(TargetHostInterface* hostInterface)
{
	StartTeamWindow* self = new StartTeamWindow(hostInterface);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
StartTeamWindow::_Init()
{
	fGuideText = new BStringView("guide", "Set new team parameters below.");
	fTeamTextControl = new BTextControl("Path: ", NULL, NULL);
	fArgumentsTextControl = new BTextControl("Arguments: ", NULL, NULL);
	fBrowseTeamButton = new BButton("Browse" B_UTF8_ELLIPSIS, new BMessage(
			MSG_BROWSE_TEAM));
	fStartButton = new BButton("Start team", new BMessage(MSG_START_NEW_TEAM));
	fCancelButton = new BButton("Cancel", new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fGuideText)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fTeamTextControl)
			.Add(fBrowseTeamButton)
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fArgumentsTextControl)
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fStartButton)
		.End();

	fTeamTextControl->SetExplicitMinSize(BSize(300.0, B_SIZE_UNSET));
	fGuideText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fStartButton->SetTarget(this);
	fCancelButton->SetTarget(this);
}


void
StartTeamWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
StartTeamWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BROWSE_TEAM:
		{
			if (fBrowseTeamPanel == NULL) {
				fBrowseTeamPanel = new(std::nothrow) BFilePanel(B_OPEN_PANEL,
					new BMessenger(this));
				if (fBrowseTeamPanel == NULL)
					break;
				BMessage* message = new(std::nothrow) BMessage(
					MSG_SET_TEAM_PATH);
				if (message == NULL) {
					delete fBrowseTeamPanel;
					fBrowseTeamPanel = NULL;
					break;
				}
				fBrowseTeamPanel->SetMessage(message);
			}

			fBrowseTeamPanel->SetPanelDirectory(fTeamTextControl->TextView()
					->Text());

			fBrowseTeamPanel->Show();
			break;
		}
		case MSG_SET_TEAM_PATH:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				fTeamTextControl->TextView()->SetText(path.Path());
			}
			break;
		}
		case MSG_START_NEW_TEAM:
		{
			BMessage appMessage(MSG_START_NEW_TEAM);
			appMessage.AddString("path", fTeamTextControl->TextView()->Text());
			appMessage.AddString("arguments", fArgumentsTextControl->TextView()
					->Text());
			appMessage.AddPointer("interface", fTargetHostInterface);
			BMessage reply;
			be_app_messenger.SendMessage(&appMessage, &reply);
			status_t error = reply.FindInt32("status");
			if (error != B_OK) {
				BString messageString;
				messageString.SetToFormat("Failed to start team: %s.",
					strerror(error));
				BAlert* alert = new(std::nothrow) BAlert("Start team failed",
					messageString.String(), "Close");
				if (alert != NULL)
					alert->Go();
			} else {
				be_app->PostMessage(MSG_START_TEAM_WINDOW_CLOSED);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}

}
