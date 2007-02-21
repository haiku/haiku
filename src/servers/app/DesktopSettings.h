/*
 * Copyright 2001-2007, Haiku.
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
class DesktopSettingsPrivate;
class ServerFont;


static const int32 kMaxWorkspaces = 32;

enum {
	kAllSettings		= 0xff,
	kWorkspacesSettings	= 0x01,
	kFontSettings		= 0x02,
	kAppearanceSettings	= 0x04,
	kMouseSettings		= 0x08,
	kDraggerSettings	= 0x10,
};

class DesktopSettings {
	public:
		DesktopSettings(Desktop* desktop);

		status_t		Save(uint32 mask = kAllSettings);

		void			GetDefaultPlainFont(ServerFont& font) const;
		void			GetDefaultBoldFont(ServerFont& font) const;
		void			GetDefaultFixedFont(ServerFont& font) const;

		void			GetScrollBarInfo(scroll_bar_info& info) const;
		void			GetMenuInfo(menu_info& info) const;

		mode_mouse		MouseMode() const;
		bool			FocusFollowsMouse() const;

		bool			ShowAllDraggers() const;

		int32			WorkspacesCount() const;
		const BMessage*	WorkspacesMessage(int32 index) const;

	protected:
		DesktopSettingsPrivate*	fSettings;
};

class LockedDesktopSettings : public DesktopSettings {
	public:
		LockedDesktopSettings(Desktop* desktop);
		~LockedDesktopSettings();

		void			SetDefaultPlainFont(const ServerFont& font);
		void			SetDefaultBoldFont(const ServerFont& font);
		void			SetDefaultFixedFont(const ServerFont& font);

		void			SetScrollBarInfo(const scroll_bar_info& info);
		void			SetMenuInfo(const menu_info& info);

		void			SetMouseMode(mode_mouse mode);

		void			SetShowAllDraggers(bool show);

	private:
		Desktop*		fDesktop;
};

#endif	/* DESKTOP_SETTINGS_H */
