/*
 * Copyright 2001-2009, Haiku.
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
class BStringView;
class BTextControl;

class RefreshWindow;
class MonitorView;
class ScreenSettings;


class ScreenWindow : public BWindow {
public:
							ScreenWindow(ScreenSettings *settings);
	virtual					~ScreenWindow();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage *message);
	virtual	void			WorkspaceActivated(int32 ws, bool state);
	virtual	void			ScreenChanged(BRect frame, color_space mode);

private:
			BButton*		_CreateColumnRowButton(bool columns, bool plus);
			BButton*		_GetColumnRowButton(bool columns, bool plus);

			void			_BuildSupportedColorSpaces();

			void			_CheckApplyEnabled();
			void			_CheckResolutionMenu();
			void			_CheckColorMenu();
			void			_CheckRefreshMenu();

			void			_UpdateActiveMode();
			void			_UpdateActiveMode(int32 workspace);
			void			_UpdateWorkspaceButtons();
			void			_UpdateRefreshControl();
			void			_UpdateMonitorView();
			void			_UpdateControls();
			void			_UpdateOriginal();
			void			_UpdateMonitor();
			void			_UpdateColorLabel();

			void			_Apply();

			status_t		_WriteVesaModeFile(const screen_mode& mode) const;
			bool			_IsVesa() const { return fIsVesa; }

private:
			ScreenSettings*	fSettings;
			bool			fIsVesa;
			bool			fBootWorkspaceApplied;

			BStringView*	fMonitorInfo;
			MonitorView*	fMonitorView;
			BMenuItem*		fAllWorkspacesItem;

			BTextControl*	fColumnsControl;
			BTextControl*	fRowsControl;
			BButton*		fWorkspacesButtons[4];

			uint32			fSupportedColorSpaces;
			BMenuItem*		fUserSelectedColorSpace;

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

			ScreenMode		fScreenMode;
			ScreenMode		fUndoScreenMode;
				// screen modes for all workspaces

			screen_mode		fActive, fSelected, fOriginal;
				// screen modes for the current workspace

			uint32			fOriginalWorkspacesColumns;
			uint32			fOriginalWorkspacesRows;
			bool			fModified;
};

#endif	/* SCREEN_WINDOW_H */
