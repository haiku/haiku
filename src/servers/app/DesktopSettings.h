/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DESKTOP_SETTINGS_H
#define DESKTOP_SETTINGS_H


#include <InterfaceDefs.h>
#include <Menu.h>
#include <Message.h>

class Desktop;


static const int32 kMaxWorkspaces = 32;


class DesktopSettings {
	public:
		DesktopSettings(Desktop* desktop);
		~DesktopSettings();

		status_t		Save();

		void			SetScrollBarInfo(const scroll_bar_info& info);
		void			GetScrollBarInfo(scroll_bar_info& info) const;

		void			SetMenuInfo(const menu_info& info);
		void			GetMenuInfo(menu_info& info) const;

		void			SetMouseMode(mode_mouse mode);
		mode_mouse		MouseMode() const;
		bool			FocusFollowsMouse() const;

		void			SetWorkspacesCount(int32 number);
		int32			WorkspacesCount() const;

		void			SetWorkspacesMessage(int32 index, BMessage& message);
		const BMessage*	WorkspacesMessage(int32 index) const;

	private:
		class Private;
		friend class Desktop;

		Private*		fSettings;
};

#endif	/* DESKTOP_SETTINGS_H */
