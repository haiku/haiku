/*
 * Copyright 2001-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini, burton666@libero.it
 *		Axel Dörfler, axeld@pinc-software.de
 *		Thomas Kurschel
 *		Rafael Romo
 *		John Scipione, jscipione@gmail.com
 */
#ifndef SCREEN_WINDOW_H
#define SCREEN_WINDOW_H


#include <Window.h>

#include "ScreenMode.h"


class BBox;
class BPopUpMenu;
class BMenuField;
class BSlider;
class BSpinner;
class BStringView;

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

			BBox*			fScreenBox;
			BStringView*	fDeviceInfo;
			MonitorView*	fMonitorView;
			BMenuItem*		fAllWorkspacesItem;

			BSpinner*		fColumnsControl;
			BSpinner*		fRowsControl;

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

			BSlider*		fBrightnessSlider;

			BButton*		fDefaultsButton;
			BButton*		fApplyButton;
			BButton*		fRevertButton;

			ScreenMode		fScreenMode;
			ScreenMode		fUndoScreenMode;
				// screen modes for all workspaces

			screen_mode		fActive, fSelected, fOriginal;
				// screen modes for the current workspace

			uint32			fOriginalWorkspacesColumns;
			uint32			fOriginalWorkspacesRows;
			float			fOriginalBrightness;
			bool			fModified;
};

#endif	/* SCREEN_WINDOW_H */
