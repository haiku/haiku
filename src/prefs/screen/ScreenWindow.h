/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 */
#ifndef SCREEN_WINDOW_H
#define SCREEN_WINDOW_H


#include <Screen.h>
#include <Window.h>

class BBox;
class BPopUpMenu;
class BMenuField;

class RefreshWindow;
class MonitorView;
class ScreenSettings;


class ScreenWindow : public BWindow {
	public:
		ScreenWindow(ScreenSettings *settings);
		virtual ~ScreenWindow();

		virtual	bool QuitRequested();
		virtual void MessageReceived(BMessage *message);
		virtual void WorkspaceActivated(int32 ws, bool state);
		virtual void FrameMoved(BPoint position);
		virtual void ScreenChanged(BRect frame, color_space mode);

	private:
		void SetStateByMode();
		void CheckApplyEnabled();
		void CheckUpdateDisplayModes();
		void CheckModesByResolution(const char*);
		void ApplyMode();
	
		ScreenSettings *fSettings;

		MonitorView *fMonitorView;
		BPopUpMenu *fWorkspaceMenu;
		BMenuField *fWorkspaceField;
		BPopUpMenu *fWorkspaceCountMenu;
		BMenuField *fWorkspaceCountField;
		BPopUpMenu *fResolutionMenu;
		BMenuField *fResolutionField;
		BPopUpMenu *fColorsMenu;
		BMenuField *fColorsField;
		BPopUpMenu *fRefreshMenu;
		BMenuField *fRefreshField;
		BMenuItem *fCurrentWorkspaceItem;
		BMenuItem *fAllWorkspacesItem;
		BButton	*fDefaultsButton;
		BButton	*fApplyButton;
		BButton	*fRevertButton;

		BMenuItem *fInitialResolution;
		BMenuItem *fInitialColors;
		BMenuItem *fInitialRefresh;
		BMenuItem *fOtherRefresh;

		display_mode fInitialMode;
		display_mode *fSupportedModes;
		uint32 fTotalModes;

		float fMinRefresh;
		float fMaxRefresh;
		float fCustomRefresh;
		float fInitialRefreshN;
};

#endif	/* SCREEN_WINDOW_H */
