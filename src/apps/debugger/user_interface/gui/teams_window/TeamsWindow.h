/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAMS_WINDOW_H
#define TEAMS_WINDOW_H


#include <Window.h>

#include <util/DoublyLinkedList.h>


class BButton;
class BListView;
class BFile;
class BFilePanel;
class BMenuField;
class BMessage;
class SettingsManager;
class TargetHostInterface;
class TeamsListView;


class TeamsWindow : public BWindow {
public:
	class Listener;

								TeamsWindow(SettingsManager* settingsManager);
	virtual						~TeamsWindow();

	static	TeamsWindow*		Create(SettingsManager* settingsManager);
									// throws

	virtual	void				Zoom(BPoint origin, float width, float height);
	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef DoublyLinkedList<Listener> ListenerList;

private:
			void				_Init();
			status_t			_OpenSettings(BFile& file, uint32 mode);
			status_t			_LoadSettings(BMessage& settings);
			status_t			_SaveSettings();

			void				_NotifySelectedInterfaceChanged(
									TargetHostInterface* interface);

private:
			team_id				fCurrentTeam;
			TargetHostInterface* fTargetHostInterface;
			TeamsListView*		fTeamsListView;
			BButton*			fAttachTeamButton;
			BButton*			fCreateTeamButton;
			BButton*			fCreateConnectionButton;
			BButton*			fLoadCoreButton;
			BMenuField*			fConnectionField;
			BFilePanel*			fCoreSelectionPanel;
			SettingsManager*	fSettingsManager;
			ListenerList		fListeners;
};


class TeamsWindow::Listener : public DoublyLinkedListLinkImpl<Listener>
{
public:
	virtual						~Listener();

	virtual	void				SelectedInterfaceChanged(
									TargetHostInterface* interface) = 0;
};


#endif	// TEAMS_WINDOW_H
