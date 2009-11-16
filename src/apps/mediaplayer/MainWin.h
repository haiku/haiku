/*
 * MainWin.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2009 Stephan Aßmus <superstippi@gmx.de> (MIT ok)
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
#include "ControllerView.h"
#include "InfoWin.h"
#include "ListenerAdapter.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "VideoView.h"


class ControllerObserver;
class PlaylistObserver;
class PlaylistWindow;


class MainWin : public BWindow {
public:
								MainWin(bool isFirstWindow);
	virtual						~MainWin();

	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				Zoom(BPoint rectPosition, float rectWidth,
									float rectHeight);
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				WindowActivated(bool active);
	virtual	bool				QuitRequested();
	virtual	void				MenusBeginning();

			void				OpenPlaylistItem(const PlaylistItemRef& item);

			void				ShowFileInfo();
			void				ShowPlaylistWindow();
			void				ShowSettingsWindow();

			void				VideoAspectChange(int forcedWidth,
									int forcedHeight, float widthScale);
			void				VideoAspectChange(float widthScale);
			void				VideoAspectChange(int widthAspect,
									int heightAspect);
			void				VideoFormatChange(int width, int height,
									int widthAspect, int heightAspect);

private:
			void				_RefsReceived(BMessage* message);

			void				_SetupWindow();
			void				_CreateMenu();
			void				_SetupVideoAspectItems(BMenu* menu);
			void				_SetupTrackMenus(BMenu* audioTrackMenu,
									BMenu* videoTrackMenu);

			void				_GetMinimumWindowSize(int& width,
										int& height) const;
			void				_SetWindowSizeLimits();
			void				_GetUnscaledVideoSize(int& videoWidth,
									int& videoHeight) const;
			int					_CurrentVideoSizeInPercent() const;
			void				_ResizeWindow(int percent,
									bool useNoVideoWidth = false,
									bool stayOnScreen = false);
			void				_ResizeVideoView(int x, int y, int width,
									int height);

			void				_MouseDown(BMessage* message,
									BView* originalHandler);
			void				_MouseMoved(BMessage* message,
									BView* originalHandler);
			void				_MouseUp(BMessage* message);
			void				_ShowContextMenu(const BPoint& screenPoint);
			bool				_KeyDown(BMessage* message);

			void				_ToggleFullscreen();
			void				_ToggleAlwaysOnTop();
			void				_ToggleNoInterface();

			void				_SetFileAttributes();
			void				_UpdateControlsEnabledStatus();
			void				_UpdatePlaylistMenu();
			void				_AddPlaylistItem(PlaylistItem* item,
									int32 index);
			void				_RemovePlaylistItem(int32 index);
			void				_MarkPlaylistItem(int32 index);
			void				_MarkItem(BMenu* menu, uint32 command,
									bool mark);

			void				_AdoptGlobalSettings();

			bigtime_t			fCreationTime;

			BMenuBar*			fMenuBar;
			BView*				fBackground;
			VideoView*			fVideoView;
			ControllerView*		fControls;
			InfoWin*			fInfoWin;
			PlaylistWindow*		fPlaylistWindow;

			BMenu*				fFileMenu;
			BMenu*				fAudioMenu;
			BMenu*				fVideoMenu;
			BMenu*				fVideoAspectMenu;
			BMenu*				fAudioTrackMenu;
			BMenu*				fVideoTrackMenu;
			BMenu*				fSettingsMenu;
			BMenuItem*			fNoInterfaceMenuItem;
			BMenu*				fPlaylistMenu;

			bool				fHasFile;
			bool				fHasVideo;
			bool				fHasAudio;

			Playlist*			fPlaylist;
			PlaylistObserver*	fPlaylistObserver;
			Controller*			fController;
			ControllerObserver*	fControllerObserver;
	volatile bool				fIsFullscreen;
	volatile bool				fAlwaysOnTop;
	volatile bool				fNoInterface;
			int					fSourceWidth;
			int					fSourceHeight;
			int					fWidthAspect;
			int					fHeightAspect;
			int					fMenuBarWidth;
			int					fMenuBarHeight;
			int					fControlsHeight;
			int					fControlsWidth;
			int					fNoVideoWidth;
			BRect				fSavedFrame;
			bool				fMouseDownTracking;
			BPoint				fMouseDownMousePos;
			BPoint				fMouseDownWindowPos;

			ListenerAdapter		fGlobalSettingsListener;
			bool				fCloseWhenDonePlayingMovie;
			bool				fCloseWhenDonePlayingSound;

	static	int					sNoVideoWidth;
};

#endif // __MAIN_WIN_H
