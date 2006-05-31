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
class ServerFont;


static const int32 kMaxWorkspaces = 32;

enum {
	kAllSettings		= 0xff,
	kWorkspacesSettings	= 0x01,
	kFontSettings		= 0x02,
	kAppearanceSettings	= 0x04,
	kMouseSettings		= 0x08,
};

class DesktopSettings {
	public:
		DesktopSettings(Desktop* desktop);
		~DesktopSettings();

		status_t		Save(uint32 mask = kAllSettings);

		void			SetDefaultPlainFont(const ServerFont& font);
		void			GetDefaultPlainFont(ServerFont& font) const;

		void			SetDefaultBoldFont(const ServerFont& font);
		void			GetDefaultBoldFont(ServerFont& font) const;

		void			SetDefaultFixedFont(const ServerFont& font);
		void			GetDefaultFixedFont(ServerFont& font) const;

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
