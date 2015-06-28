/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_SETTINGS_WINDOW_H
#define TEAM_SETTINGS_WINDOW_H


#include <Window.h>


class BButton;
class ExceptionStopConfigView;
class ImageStopConfigView;
class Team;
class UserInterfaceListener;


class TeamSettingsWindow : public BWindow {
public:
								TeamSettingsWindow(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);

								~TeamSettingsWindow();

	static	TeamSettingsWindow* Create(::Team* team,
									UserInterfaceListener* listener,
									BHandler* target);
									// throws

	virtual	void				Show();

private:
			void	 			_Init();

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BButton*			fCloseButton;
			BHandler*			fTarget;
};


#endif // TEAM_SETTINGS_WINDOW_H
