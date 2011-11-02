/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "GraphicalUserInterface.h"

#include <Alert.h>

#include "GUITeamUISettings.h"
#include "TeamWindow.h"
#include "Tracing.h"


GraphicalUserInterface::GraphicalUserInterface()
	:
	fTeamWindow(NULL),
	fTeamWindowMessenger(NULL)
{
}


GraphicalUserInterface::~GraphicalUserInterface()
{
	delete fTeamWindowMessenger;
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


status_t
GraphicalUserInterface::LoadSettings(const TeamUISettings* settings)
{
	// TODO: restore settings

	return B_OK;
}


status_t
GraphicalUserInterface::SaveSettings(TeamUISettings*& settings) const
{
	settings = new(std::nothrow) GUITeamUISettings(ID());
	if (settings == NULL)
		return B_NO_MEMORY;

	// TODO: fill in settings

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
