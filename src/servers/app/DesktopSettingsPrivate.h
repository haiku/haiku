/*
 * Copyright 2005-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef DESKTOP_SETTINGS_PRIVATE_H
#define DESKTOP_SETTINGS_PRIVATE_H


#include "DesktopSettings.h"

#include <Locker.h>

#include "ServerFont.h"


struct server_read_only_memory;


class DesktopSettingsPrivate {
public:
								DesktopSettingsPrivate(
									server_read_only_memory* shared);
								~DesktopSettingsPrivate();

			status_t			Save(uint32 mask = kAllSettings);

			void				SetDefaultPlainFont(const ServerFont& font);
			const ServerFont&	DefaultPlainFont() const;

			void				SetDefaultBoldFont(const ServerFont& font);
			const ServerFont&	DefaultBoldFont() const;

			void				SetDefaultFixedFont(const ServerFont& font);
			const ServerFont&	DefaultFixedFont() const;

			void				SetScrollBarInfo(const scroll_bar_info &info);
			const scroll_bar_info& ScrollBarInfo() const;

			void				SetMenuInfo(const menu_info &info);
			const menu_info&	MenuInfo() const;

			void				SetMouseMode(mode_mouse mode);
			mode_mouse			MouseMode() const;
			void				SetFocusFollowsMouseMode(
									mode_focus_follows_mouse mode);
			mode_focus_follows_mouse FocusFollowsMouseMode() const;
			bool				NormalMouse() const
									{ return MouseMode() == B_NORMAL_MOUSE; }
			bool				FocusFollowsMouse() const
									{ return MouseMode()
										== B_FOCUS_FOLLOWS_MOUSE; }
			bool				ClickToFocusMouse() const
									{ return MouseMode()
										== B_CLICK_TO_FOCUS_MOUSE; }
			void				SetAcceptFirstClick(bool acceptFirstClick);
			bool				AcceptFirstClick() const;

			void				SetShowAllDraggers(bool show);
			bool				ShowAllDraggers() const;

			void				SetWorkspacesLayout(int32 columns, int32 rows);
			int32				WorkspacesCount() const;
			int32				WorkspacesColumns() const;
			int32				WorkspacesRows() const;

			void				SetWorkspacesMessage(int32 index,
									BMessage& message);
			const BMessage*		WorkspacesMessage(int32 index) const;

			void				SetUIColor(color_which which,
									const rgb_color color,
									bool* changed = NULL);
			void				SetUIColors(const BMessage& colors,
									bool* changed = NULL);
									// changed must be boolean array equal in
									// size to colors' size

			rgb_color			UIColor(color_which which) const;

			void				SetSubpixelAntialiasing(bool subpix);
			bool				SubpixelAntialiasing() const;
			void				SetHinting(uint8 hinting);
			uint8				Hinting() const;
			void				SetSubpixelAverageWeight(uint8 averageWeight);
			uint8				SubpixelAverageWeight() const;
			void				SetSubpixelOrderingRegular(
									bool subpixelOrdering);
			bool				IsSubpixelOrderingRegular() const;

			status_t			SetControlLook(const char* path);
			const BString&		ControlLook() const;

private:
			void				_SetDefaults();
			status_t			_Load();
			status_t			_GetPath(BPath& path);
			void				_ValidateWorkspacesLayout(int32& columns,
									int32& rows) const;

			ServerFont			fPlainFont;
			ServerFont			fBoldFont;
			ServerFont			fFixedFont;

			scroll_bar_info		fScrollBarInfo;
			menu_info			fMenuInfo;
			mode_mouse			fMouseMode;
			mode_focus_follows_mouse	fFocusFollowsMouseMode;
			bool				fAcceptFirstClick;
			bool				fShowAllDraggers;
			int32				fWorkspacesColumns;
			int32				fWorkspacesRows;
			BMessage			fWorkspaceMessages[kMaxWorkspaces];
			BString				fControlLook;

			server_read_only_memory& fShared;
};

#endif	/* DESKTOP_SETTINGS_PRIVATE_H */
