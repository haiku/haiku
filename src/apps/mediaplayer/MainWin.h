/*
 * MainWin.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __MAIN_WIN_H
#define __MAIN_WIN_H

#include <Window.h>
#include <Menu.h>
#include <Button.h>
#include <Slider.h>
#include "Controller.h"
#include "VideoView.h"

class MainWin : public BWindow
{
public:
						MainWin(BRect rect);
						~MainWin();

	void				FrameResized(float new_width, float new_height);
	void				Zoom(BPoint rec_position, float rec_width, float rec_height);
	void				DispatchMessage(BMessage *message, BHandler *handler);
	void				MessageReceived(BMessage *msg);
	bool				QuitRequested();

	void				MouseDown(BMessage *msg);
	void				MouseMoved(BMessage *msg);
	void				MouseUp(BMessage *msg);
	status_t			KeyDown(BMessage *msg);

	void				CreateMenu();
	
	void				VideoFormatChange(int width, int height, float width_scale, float height_scale);

	void				UpdateWindowTitle();
	
	void				AdjustFullscreenRenderer();
	void 				AdjustWindowedRenderer(bool user_resized);
	
	void				ToggleFullscreen();
	void				ToggleKeepAspectRatio();
	void				ToggleAlwaysOnTop();
	void				ToggleNoBorder();
	void				ToggleNoMenu();
	void				ToggleNoBorderNoMenu();
	
	void				ShowContextMenu(const BPoint &screen_point);
	
	BMenuBar *			fMenuBar;
	BView *				fBackground;
	VideoView *			fVideoView;

	BMenu *				fFileMenu;
	BMenu *				fChannelMenu;
	BMenu *				fInterfaceMenu;
	BMenu *				fSettingsMenu;
	BMenu *				fDebugMenu;

	Controller *		fController;
	volatile bool		fIsFullscreen;
	volatile bool		fKeepAspectRatio;
	volatile bool		fAlwaysOnTop;
	volatile bool		fNoMenu;
	volatile bool		fNoBorder;
	int					fSourceWidth;
	int					fSourceHeight;
	float				fWidthScale;
	float				fHeightScale;
	int					fMenuBarHeight;
	BRect				fSavedFrame;
	bool				fMouseDownTracking;
	BPoint				fMouseDownMousePos;
	BPoint				fMouseDownWindowPos;
	bool				fFrameResizedTriggeredAutomatically;
	bool				fIgnoreFrameResized;
	bool				fFrameResizedCalled;
};

#endif
