/*
 * MainWin.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
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
#include <FilePanel.h>
#include "Controller.h"
#include "ControllerView.h"
#include "InfoWin.h"
#include "VideoView.h"
#include "Playlist.h"

class ControllerObserver;
class PlaylistObserver;
class PlaylistWindow;

class MainWin : public BWindow {
public:
						MainWin();
						~MainWin();

	void				FrameResized(float new_width, float new_height);
	void				Zoom(BPoint rec_position, float rec_width, float rec_height);
	void				RefsReceived(BMessage *msg);
	void				DispatchMessage(BMessage *message, BHandler *handler);
	void				MessageReceived(BMessage *msg);
	bool				QuitRequested();

	void				MouseDown(BMessage *msg, BView* originalHandler);
	void				MouseMoved(BMessage *msg, BView* originalHandler);
	void				MouseUp(BMessage *msg);
	status_t			KeyDown(BMessage *msg);

	void				CreateMenu();
	void				SetupWindow();
	void				SetupTrackMenus();
	void				SetWindowSizeLimits();
	void				ResizeWindow(int percent);
	void				ResizeVideoView(int x, int y, int width, int height);
	
	void				ShowFileInfo();
	void				ShowPlaylistWindow();
	void				MaybeUpdateFileInfo(uint32 which=INFO_ALL);

	void				OpenFile(const entry_ref &ref);
	
	void				VideoFormatChange(int width, int height, float width_scale, float height_scale);
	
	void				ToggleFullscreen();
	void				ToggleKeepAspectRatio();
	void				ToggleAlwaysOnTop();
	void				ToggleNoBorder();
	void				ToggleNoMenu();
	void				ToggleNoControls();
	void				ToggleNoBorderNoMenu();
	
	void				ShowContextMenu(const BPoint &screen_point);

	void				UpdateControlsEnabledStatus();
	void				_UpdatePlaylistMenu();
	void				_AddPlaylistItem(const entry_ref& ref, int32 index);
	void				_RemovePlaylistItem(int32 index);
	void				_MarkPlaylistItem(int32 index);

	BMenuBar *			fMenuBar;
	BView *				fBackground;
	VideoView *			fVideoView;
	BFilePanel *		fFilePanel;
	ControllerView *	fControls;
	InfoWin *			fInfoWin;
	PlaylistWindow*		fPlaylistWindow;
	bool				fInfoWinShowing;

	BMenu *				fFileMenu;
	BMenu *				fAudioMenu;
	BMenu *				fVideoMenu;
	BMenu *				fAudioTrackMenu;
	BMenu *				fVideoTrackMenu;
	BMenu *				fSettingsMenu;
	BMenu *				fPlaylistMenu;
	BMenu *				fDebugMenu;
	
	bool				fHasFile;
	bool				fHasVideo;
	bool				fHasAudio;

	Playlist *			fPlaylist;
	PlaylistObserver*	fPlaylistObserver;
	Controller *		fController;
	ControllerObserver*	fControllerObserver;
	volatile bool		fIsFullscreen;
	volatile bool		fKeepAspectRatio;
	volatile bool		fAlwaysOnTop;
	volatile bool		fNoMenu;
	volatile bool		fNoBorder;
	volatile bool		fNoControls;
	int					fSourceWidth;
	int					fSourceHeight;
	float				fWidthScale;
	float				fHeightScale;
	int					fMenuBarWidth;
	int					fMenuBarHeight;
	int					fControlsHeight;
	int					fControlsWidth;
	BRect				fSavedFrame;
	bool				fMouseDownTracking;
	BPoint				fMouseDownMousePos;
	BPoint				fMouseDownWindowPos;

private:
	void				_AppendToPlaylist(const entry_ref& ref,
							Playlist* playlist);
};

#endif
