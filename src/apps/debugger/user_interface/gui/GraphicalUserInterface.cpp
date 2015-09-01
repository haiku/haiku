/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "GraphicalUserInterface.h"

#include <Alert.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <FilePanel.h>
#include <Locker.h>

#include "GuiTeamUiSettings.h"
#include "MessageCodes.h"
#include "TeamWindow.h"
#include "Tracing.h"


// #pragma mark - GraphicalUserInterface::FilePanelHandler


class GraphicalUserInterface::FilePanelHandler : public BHandler {
public:
								FilePanelHandler();
	virtual						~FilePanelHandler();

			status_t			Init();

	virtual	void				MessageReceived(BMessage* message);

			status_t			WaitForPanel();

			void				SetCurrentRef(entry_ref* ref);

			BLocker&			Locker()
									{ return fPanelLock; }

private:
			entry_ref*			fCurrentRef;
			BLocker				fPanelLock;
			sem_id				fPanelWaitSem;
};


GraphicalUserInterface::FilePanelHandler::FilePanelHandler()
	:
	BHandler("GuiPanelHandler"),
	fCurrentRef(NULL),
	fPanelLock(),
	fPanelWaitSem(-1)
{
}


GraphicalUserInterface::FilePanelHandler::~FilePanelHandler()
{
	if (fPanelWaitSem >= 0)
		delete_sem(fPanelWaitSem);
}


status_t
GraphicalUserInterface::FilePanelHandler::Init()
{
	fPanelWaitSem = create_sem(0, "FilePanelWaitSem");

	if (fPanelWaitSem < 0)
		return fPanelWaitSem;

	return B_OK;
}


void
GraphicalUserInterface::FilePanelHandler::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_USER_INTERFACE_FILE_CHOSEN:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK
				&& fCurrentRef != NULL) {
				*fCurrentRef = ref;
				fCurrentRef = NULL;
			}
			// fall through
		}

		case B_CANCEL:
		{
			release_sem(fPanelWaitSem);
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


status_t
GraphicalUserInterface::FilePanelHandler::WaitForPanel()
{
	status_t result = B_OK;
	do {
		result = acquire_sem(fPanelWaitSem);
	} while (result == B_INTERRUPTED);

	return result;
}


void
GraphicalUserInterface::FilePanelHandler::SetCurrentRef(entry_ref* ref)
{
	fCurrentRef = ref;
}


// #pragma mark - GraphicalUserInterface


GraphicalUserInterface::GraphicalUserInterface()
	:
	fTeamWindow(NULL),
	fTeamWindowMessenger(NULL),
	fFilePanelHandler(NULL),
	fFilePanel(NULL)
{
}


GraphicalUserInterface::~GraphicalUserInterface()
{
	delete fTeamWindowMessenger;
	delete fFilePanel;
}


const char*
GraphicalUserInterface::ID() const
{
	return "GraphicalUserInterface";
}


status_t
GraphicalUserInterface::Init(Team* team, UserInterfaceListener* listener)
{
	try {
		fTeamWindow = TeamWindow::Create(team, listener);
		fTeamWindowMessenger = new BMessenger(fTeamWindow);
		fFilePanelHandler = new FilePanelHandler();
		status_t error = fFilePanelHandler->Init();
		if (error != B_OK) {
			ERROR("Error: Failed to create file panel semaphore!\n");
			return error;
		}
		fTeamWindow->AddHandler(fFilePanelHandler);

		// start the message loop
		fTeamWindow->Hide();
		fTeamWindow->Show();
	} catch (...) {
		// TODO: Notify the user!
		ERROR("Error: Failed to create team window!\n");
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
GraphicalUserInterface::Show()
{
	fTeamWindow->Show();
}


void
GraphicalUserInterface::Terminate()
{
	// quit window
	if (fTeamWindowMessenger && fTeamWindowMessenger->LockTarget())
		fTeamWindow->Quit();
}


bool
GraphicalUserInterface::IsInteractive() const
{
	return true;
}


status_t
GraphicalUserInterface::LoadSettings(const TeamUiSettings* settings)
{
	status_t result = fTeamWindow->LoadSettings((GuiTeamUiSettings*)settings);

	return result;
}


status_t
GraphicalUserInterface::SaveSettings(TeamUiSettings*& settings) const
{
	settings = new(std::nothrow) GuiTeamUiSettings(ID());
	if (settings == NULL)
		return B_NO_MEMORY;

	fTeamWindow->SaveSettings((GuiTeamUiSettings*)settings);

	return B_OK;
}


void
GraphicalUserInterface::NotifyUser(const char* title, const char* message,
	user_notification_type type)
{
	// convert notification type to alert type
	alert_type alertType;
	switch (type) {
		case USER_NOTIFICATION_INFO:
			alertType = B_INFO_ALERT;
			break;
		case USER_NOTIFICATION_WARNING:
		case USER_NOTIFICATION_ERROR:
		default:
			alertType = B_WARNING_ALERT;
			break;
	}

	BAlert* alert = new(std::nothrow) BAlert(title, message, "OK",
		NULL, NULL, B_WIDTH_AS_USUAL, alertType);
	if (alert != NULL)
		alert->Go(NULL);

	// TODO: We need to let the alert run asynchronously, but we shouldn't just
	// create it and don't care anymore. Maybe an error window, which can
	// display a list of errors would be the better choice.
}


void
GraphicalUserInterface::NotifyBackgroundWorkStatus(const char* message)
{
	fTeamWindow->DisplayBackgroundStatus(message);
}


int32
GraphicalUserInterface::SynchronouslyAskUser(const char* title,
	const char* message, const char* choice1, const char* choice2,
	const char* choice3)
{
	BAlert* alert = new(std::nothrow) BAlert(title, message,
		choice1, choice2, choice3);
	if (alert == NULL)
		return 0;
	return alert->Go();
}


status_t
GraphicalUserInterface::SynchronouslyAskUserForFile(entry_ref* _ref)
{
	BAutolock lock(&fFilePanelHandler->Locker());

	if (fFilePanel == NULL) {
		BMessenger messenger(fFilePanelHandler);
		BMessage* message = new(std::nothrow) BMessage(
			MSG_USER_INTERFACE_FILE_CHOSEN);
		if (message == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<BMessage> messageDeleter(message);
		fFilePanel = new(std::nothrow) BFilePanel(B_OPEN_PANEL,
			&messenger, NULL, B_FILE_NODE, false, message);
		if (fFilePanel == NULL)
			return B_NO_MEMORY;
		messageDeleter.Detach();
	}

	fFilePanelHandler->SetCurrentRef(_ref);
	fFilePanel->Show();
	return fFilePanelHandler->WaitForPanel();
}
