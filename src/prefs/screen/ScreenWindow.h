#ifndef SCREENWINDOW_H
#define SCREENWINDOW_H

#include <PopUpMenu.h>
#include <MenuField.h>
#include <Window.h>
#include <Screen.h>
#include <Box.h>

#include "ScreenView.h"
#include "ScreenDrawView.h"
#include "RefreshWindow.h"
#include "ScreenSettings.h"

class ScreenWindow : public BWindow
{

public:
						ScreenWindow(ScreenSettings *Settings);
	virtual				~ScreenWindow();
	virtual	bool		QuitRequested();
	virtual void 		MessageReceived(BMessage *message);
	virtual void		WorkspaceActivated(int32 ws, bool state);
	virtual void 		FrameMoved(BPoint position);
	virtual void		ScreenChanged(BRect frame, color_space mode);

private:
	void				CheckApplyEnabled();
	ScreenSettings		*fSettings;
	ScreenView			*fScreenView;
	ScreenDrawView		*fScreenDrawView;
	RefreshWindow		*fRefreshWindow;
	BPopUpMenu			*fWorkspaceMenu;
	BMenuField			*fWorkspaceField;
	BPopUpMenu			*fWorkspaceCountMenu;
	BMenuField			*fWorkspaceCountField;
	BPopUpMenu			*fResolutionMenu;
	BMenuField			*fResolutionField;
	BPopUpMenu			*fColorsMenu;
	BMenuField			*fColorsField;
	BPopUpMenu			*fRefreshMenu;
	BMenuField			*fRefreshField;
	BMenuItem			*fCurrentWorkspaceItem;
	BMenuItem			*fAllWorkspacesItem;
	BButton				*fDefaultsButton;
	BButton				*fApplyButton;
	BButton				*fRevertButton;
	BBox				*fScreenBox;
	BBox 				*fControlsBox;
	BMenuItem			*fInitialResolution;
	BMenuItem			*fInitialColors;
	BMenuItem			*fInitialRefresh;
	display_mode		fInitialMode;
	float				fCustomRefresh;
	float				fInitialRefreshN;
};

#endif