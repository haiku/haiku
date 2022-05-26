/*
 * MainWin.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2010 Stephan Aßmus <superstippi@gmx.de> (MIT ok)
 *
 * Released under the terms of the MIT license.
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
								MainWin(bool isFirstWindow,
									BMessage* message = NULL);
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

			void				OpenPlaylist(const BMessage* playlistArchive);
			void				OpenPlaylistItem(const PlaylistItemRef& item);
			void				Eject();
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

			void				GetQuitMessage(BMessage* message);

	virtual	BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 what,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

private:
			void				_RefsReceived(BMessage* message);
			void				_PlaylistItemOpened(
									const PlaylistItemRef& item,
									status_t result);

			void				_SetupWindow();
			void				_CreateMenu();
			void				_SetupVideoAspectItems(BMenu* menu);
			void				_SetupTrackMenus(BMenu* audioTrackMenu,
									BMenu* videoTrackMenu,
									BMenu* subTitleTrackMenu);
			void				_UpdateAudioChannelCount(int32 audioTrackIndex);

			void				_GetMinimumWindowSize(int& width,
										int& height) const;
			void				_SetWindowSizeLimits();
			void				_GetUnscaledVideoSize(int& videoWidth,
									int& videoHeight) const;
			int					_CurrentVideoSizeInPercent() const;
			void				_ZoomVideoView(int percentDiff);
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
			void				_ShowIfNeeded();
			void				_ShowFullscreenControls(bool show,
									bool animate = true);

			void				_Wind(bigtime_t howMuch, int64 frames);

			void				_UpdatePlaylistItemFile();
			void				_UpdateAttributesMenu(const BNode& node);
			void				_SetRating(int32 rating);

			void				_UpdateControlsEnabledStatus();
			void				_UpdatePlaylistMenu();
			void				_AddPlaylistItem(PlaylistItem* item,
									int32 index);
			void				_RemovePlaylistItem(int32 index);
			void				_MarkPlaylistItem(int32 index);
			void				_MarkItem(BMenu* menu, uint32 command,
									bool mark);

			void				_AdoptGlobalSettings();

private:
			bigtime_t			fCreationTime;

			BMenuBar*			fMenuBar;
			BView*				fBackground;
			VideoView*			fVideoView;
			ControllerView*		fControls;
			InfoWin*			fInfoWin;
			PlaylistWindow*		fPlaylistWindow;

			BMenu*				fFileMenu;
			BMenu*				fPlaylistMenu;
			BMenu*				fAudioMenu;
			BMenu*				fVideoMenu;
			BMenu*				fVideoAspectMenu;
			BMenu*				fAudioTrackMenu;
			BMenu*				fVideoTrackMenu;
			BMenu*				fSubTitleTrackMenu;
			BMenuItem*			fNoInterfaceMenuItem;
			BMenu*				fAttributesMenu;
			BMenu*				fRatingMenu;

			bool				fHasFile;
			bool				fHasVideo;
			bool				fHasAudio;

			Playlist*			fPlaylist;
			PlaylistObserver*	fPlaylistObserver;
			Controller*			fController;
			ControllerObserver*	fControllerObserver;

			bool				fIsFullscreen;
			bool				fAlwaysOnTop;
			bool				fNoInterface;
			bool				fShowsFullscreenControls;

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
			BRect				fNoVideoFrame;

			bool				fMouseDownTracking;
			BPoint				fMouseDownMousePos;
			BPoint				fMouseDownWindowPos;
			BPoint				fLastMousePos;
			bigtime_t			fLastMouseMovedTime;
			float				fMouseMoveDist;

			ListenerAdapter		fGlobalSettingsListener;
			bool				fCloseWhenDonePlayingMovie;
			bool				fCloseWhenDonePlayingSound;
			bool				fLoopMovies;
			bool				fLoopSounds;
			bool				fScaleFullscreenControls;
			bigtime_t			fInitialSeekPosition;
			bool				fAllowWinding;

	static	int					sNoVideoWidth;
};

#endif // __MAIN_WIN_H
