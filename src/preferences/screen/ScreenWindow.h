/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Thomas Kurschel
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_WINDOW_H
#define SCREEN_WINDOW_H


#include <Window.h>

#include "ScreenMode.h"

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
		virtual void ScreenChanged(BRect frame, color_space mode);

	private:
		void CheckApplyEnabled();
		void CheckResolutionMenu();
		void CheckColorMenu();
		void CheckRefreshMenu();

		void UpdateActiveMode();
		void UpdateRefreshControl();
		void UpdateMonitorView();
		void UpdateControls();
		void UpdateOriginal();

		void Apply();

		bool CanApply() const;
		bool CanRevert() const;

		status_t _WriteVesaModeFile(const screen_mode& mode) const;
		bool _IsVesa() const { return fIsVesa; }

		void LayoutControls(uint32 flags);
		BRect LayoutMenuFields(uint32 flags, bool sideBySide = false);

		ScreenSettings*	fSettings;
		bool			fIsVesa;
		bool			fVesaApplied;

		BBox*			fScreenBox;
		BBox*			fControlsBox;

		MonitorView*	fMonitorView;
		BMenuItem*		fAllWorkspacesItem;

		BMenuField*		fWorkspaceCountField;

		BPopUpMenu*		fResolutionMenu;
		BMenuField*		fResolutionField;
		BPopUpMenu*		fColorsMenu;
		BMenuField*		fColorsField;
		BPopUpMenu*		fRefreshMenu;
		BMenuField*		fRefreshField;
		BMenuItem*		fOtherRefresh;

		BPopUpMenu*		fCombineMenu;
		BMenuField*		fCombineField;
		BPopUpMenu*		fSwapDisplaysMenu;
		BMenuField*		fSwapDisplaysField;
		BPopUpMenu*		fUseLaptopPanelMenu;
		BMenuField*		fUseLaptopPanelField;
		BPopUpMenu*		fTVStandardMenu;
		BMenuField*		fTVStandardField;

		BButton*		fDefaultsButton;
		BButton*		fApplyButton;
		BButton*		fRevertButton;

		BButton*		fBackgroundsButton;

		ScreenMode		fScreenMode, fTempScreenMode;
			// screen modes for all workspaces
		int32			fOriginalWorkspaceCount;
		screen_mode		fActive, fSelected, fOriginal;
			// screen modes for the current workspace
		bool			fModified;
};

#endif	/* SCREEN_WINDOW_H */
