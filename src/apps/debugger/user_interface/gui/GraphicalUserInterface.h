/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef GRAPHICAL_USER_INTERFACE_H
#define GRAPHICAL_USER_INTERFACE_H


#include "UserInterface.h"


class BFilePanel;
class BHandler;
class BMessenger;
class TeamWindow;


class GraphicalUserInterface : public UserInterface {
public:
								GraphicalUserInterface();
	virtual						~GraphicalUserInterface();

	virtual	const char*			ID() const;

	virtual	status_t			Init(Team* team,
									UserInterfaceListener* listener);
	virtual	void				Show();
	virtual	void				Terminate();
									// shut down the UI *now* -- no more user
									// feedback

	virtual	bool				IsInteractive() const;

	virtual status_t			LoadSettings(const TeamUiSettings* settings);
	virtual status_t			SaveSettings(TeamUiSettings*& settings)	const;

	virtual	void				NotifyUser(const char* title,
									const char* message,
									user_notification_type type);
	virtual	void				NotifyBackgroundWorkStatus(
									const char* message);
	virtual	int32				SynchronouslyAskUser(const char* title,
									const char* message, const char* choice1,
									const char* choice2, const char* choice3);
	virtual	status_t			SynchronouslyAskUserForFile(entry_ref* _ref);
private:
			class FilePanelHandler;

private:
			TeamWindow*			fTeamWindow;
			BMessenger*			fTeamWindowMessenger;
			FilePanelHandler*	fFilePanelHandler;
			BFilePanel*			fFilePanel;
};


#endif	// GRAPHICAL_USER_INTERFACE_H
