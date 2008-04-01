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
class SettingsWindow;

class MainWin : public BWindow {
public:
								MainWin();
	virtual						~MainWin();

	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				Zoom(BPoint rectPosition, float rectWidth,
									float rectHeight);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				OpenFile(const entry_ref& ref);
			
			void				ShowFileInfo();
			void				ShowPlaylistWindow();
			void				ShowSettingsWindow();
		
			void				VideoFormatChange(int width, int height,
									float widthScale, float heightScale);

private:
			void				_RefsReceived(BMessage *message);
		
			void				_SetupWindow();
			void				_CreateMenu();
			void				_SetupTrackMenus();
			void				_SetWindowSizeLimits();
			void				_ResizeWindow(int percent);
			void				_ResizeVideoView(int x, int y, int width,
									int height);
			
			void				_MouseDown(BMessage* message,
									BView* originalHandler);
			void				_MouseMoved(BMessage* message,
									BView* originalHandler);
			void				_MouseUp(BMessage* message);
			void				_ShowContextMenu(const BPoint& screenPoint);
			status_t			_KeyDown(BMessage* message);
				
			void				_ToggleFullscreen();
			void				_ToggleKeepAspectRatio();
			void				_ToggleAlwaysOnTop();
			void				_ToggleNoBorder();
			void				_ToggleNoMenu();
			void				_ToggleNoControls();
			void				_ToggleNoBorderNoMenu();
			
			void				_UpdateControlsEnabledStatus();
			void				_UpdatePlaylistMenu();
			void				_AddPlaylistItem(const entry_ref& ref,
									int32 index);
			void				_RemovePlaylistItem(int32 index);
			void				_MarkPlaylistItem(int32 index);
			void				_MarkSettingsItem(uint32 command, bool mark);
		
			BMenuBar*			fMenuBar;
			BView*				fBackground;
			VideoView*			fVideoView;
			BFilePanel*			fFilePanel;
			ControllerView*		fControls;
			InfoWin*			fInfoWin;
			PlaylistWindow*		fPlaylistWindow;
			SettingsWindow*		fSettingsWindow;
		
			BMenu*				fFileMenu;
			BMenu*				fAudioMenu;
			BMenu*				fVideoMenu;
			BMenu*				fAudioTrackMenu;
			BMenu*				fVideoTrackMenu;
			BMenu*				fSettingsMenu;
			BMenu*				fPlaylistMenu;
			BMenu*				fDebugMenu;
			
			bool				fHasFile;
			bool				fHasVideo;
			bool				fHasAudio;
		
			Playlist*			fPlaylist;
			PlaylistObserver*	fPlaylistObserver;
			Controller*			fController;
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
};

#endif // __MAIN_WIN_H
