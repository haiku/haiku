/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

class PadView;

enum {
	MSG_SHOW_BORDER				= 'shbr',
	MSG_HIDE_BORDER				= 'hdbr',

	MSG_TOGGLE_AUTORAISE		= 'tgar',
	MSG_SHOW_ON_ALL_WORKSPACES	= 'awrk',

	MSG_SET_DESCRIPTION			= 'dscr',

	MSG_ADD_WINDOW				= 'addw',
};

class MainWindow : public BWindow {
 public:
								MainWindow(const char* name,
										   BRect frame); 
								MainWindow(const char* name,
										   BRect frame, BMessage* settings); 
	virtual						~MainWindow();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();
	virtual	void				ScreenChanged(BRect frame, color_space format);
	virtual void				WorkspaceActivated(int32 workspace, bool active);
	virtual	void				FrameMoved(BPoint origin);
	virtual	void				FrameResized(float width, 
											 float height);

	// MainWindow
			void				ToggleAutoRaise();
			bool				AutoRaise() const
									{ return fAutoRaise; }
			bool				ShowOnAllWorkspaces() const
									{ return fShowOnAllWorkspaces; }

			BPoint				ScreenPosition() const
									{ return fScreenPosition; }

			bool				LoadSettings(const BMessage* message);
			void				SaveSettings(BMessage* message);
			BMessage*			Settings() const
									{ return fSettings; }

 private:
 			void				_GetLocation();
			void				_AdjustLocation(BRect frame);

	BMessage*					fSettings;
	PadView*					fPadView;
	int32						fLastID;

	float						fBorderDist;
	BPoint						fScreenPosition;	// not really the position, 0...1 = left...right

	BRect						fNamePanelFrame;

	bool						fAutoRaise;
	bool						fShowOnAllWorkspaces;
};

#endif // MAIN_WINDOW_H
