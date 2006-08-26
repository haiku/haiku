/*
 * Copyright 2005-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DesktopSettings.h"
#include "DesktopSettingsPrivate.h"
#include "Desktop.h"
#include "FontManager.h"
#include "ServerConfig.h"

#include <DefaultColors.h>
#include <ServerReadOnlyMemory.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


DesktopSettingsPrivate::DesktopSettingsPrivate(server_read_only_memory* shared)
	:
	fShared(*shared)
{
	// if the on-disk settings are not complete, the defaults will be kept
	_SetDefaults();
	_Load();
}


DesktopSettingsPrivate::~DesktopSettingsPrivate()
{
}


void
DesktopSettingsPrivate::_SetDefaults()
{
	fPlainFont = *gFontManager->DefaultPlainFont();
	fBoldFont = *gFontManager->DefaultBoldFont();
	fFixedFont = *gFontManager->DefaultFixedFont();

	fMouseMode = B_NORMAL_MOUSE;

	// init scrollbar info
	fScrollBarInfo.proportional = true;
	fScrollBarInfo.double_arrows = false;
	// look of the knob (R5: (0, 1, 2), 1 = default)
	fScrollBarInfo.knob = 1;
	fScrollBarInfo.min_knob_size = 15;

	// init menu info
	strlcpy(fMenuInfo.f_family, fPlainFont.Family(), B_FONT_FAMILY_LENGTH);
	strlcpy(fMenuInfo.f_style, fPlainFont.Style(), B_FONT_STYLE_LENGTH);
	fMenuInfo.font_size = fPlainFont.Size();
	fMenuInfo.background_color.set_to(216, 216, 216);
	
	// look of the separator (R5: (0, 1, 2), default 0)
	// TODO: we could just choose a nice one and remove the others
	fMenuInfo.separator = 0;
	fMenuInfo.click_to_open = true; // always true
	fMenuInfo.triggers_always_shown = false;

	fWorkspacesCount = 4;

	memcpy(fShared.colors, BPrivate::kDefaultColors, sizeof(rgb_color) * kNumColors);
}


status_t
DesktopSettingsPrivate::_GetPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	status = path.Append("system/app_server");
	if (status < B_OK)
		return status;

	return create_directory(path.Path(), 0755);
}


status_t
DesktopSettingsPrivate::_Load()
{
	// TODO: add support for old app_server_settings file as well

	BPath basePath;
	status_t status = _GetPath(basePath);
	if (status < B_OK)
		return status;

	// read workspaces settings

	BPath path(basePath);
	path.Append("workspaces");
	
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			int32 count;
			if (settings.FindInt32("count", &count) == B_OK) {
				fWorkspacesCount = count;
				if (fWorkspacesCount < 1 || fWorkspacesCount > 32)
					fWorkspacesCount = 4;
			}

			int32 i = 0;
			while (i < kMaxWorkspaces
				&& settings.FindMessage("workspace", i, &fWorkspaceMessages[i]) == B_OK) {
				i++;
			}
		}
	}

	// read font settings

	path = basePath;
	path.Append("fonts");
	
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK && gFontManager->Lock()) {
			const char* family;
			const char* style;
			float size;
			if (settings.FindString("plain family", &family) == B_OK
				&& settings.FindString("plain style", &style) == B_OK
				&& settings.FindFloat("plain size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				fPlainFont.SetStyle(fontStyle);
				fPlainFont.SetSize(size);
			}
			if (settings.FindString("bold family", &family) == B_OK
				&& settings.FindString("bold style", &style) == B_OK
				&& settings.FindFloat("bold size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				fBoldFont.SetStyle(fontStyle);
				fBoldFont.SetSize(size);
			}
			if (settings.FindString("fixed family", &family) == B_OK
				&& settings.FindString("fixed style", &style) == B_OK
				&& settings.FindFloat("fixed size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				if (fontStyle->IsFixedWidth())
					fFixedFont.SetStyle(fontStyle);
				fFixedFont.SetSize(size);
			}
			gFontManager->Unlock();
		}
	}

	// read mouse settings

	path = basePath;
	path.Append("mouse");
	
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			int32 mode;
			if (settings.FindInt32("mode", &mode) == B_OK) {
				fMouseMode = (mode_mouse)mode;
			}
		}
	}

	// read appearance settings

	path = basePath;
	path.Append("appearance");
	
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			float fontSize;
			if (settings.FindFloat("font size", &fontSize) == B_OK)
				fMenuInfo.font_size = fontSize;
				
			const char* fontFamily;
			if (settings.FindString("font family", &fontFamily) == B_OK)
				strlcpy(fMenuInfo.f_family, fontFamily, B_FONT_FAMILY_LENGTH);

			const char* fontStyle;
			if (settings.FindString("font style", &fontStyle) == B_OK)
				strlcpy(fMenuInfo.f_style, fontStyle, B_FONT_STYLE_LENGTH);

			rgb_color bgColor;
			if (settings.FindInt32("bg color", (int32*)&bgColor) == B_OK)
				fMenuInfo.background_color = bgColor;

			int32 separator;
			if (settings.FindInt32("separator", &separator) == B_OK)
				fMenuInfo.separator = separator;

			bool clickToOpen;
			if (settings.FindBool("click to open", &clickToOpen) == B_OK)
				fMenuInfo.click_to_open = clickToOpen;

			bool triggersAlwaysShown;
			if (settings.FindBool("triggers always shown", &triggersAlwaysShown) == B_OK)
				fMenuInfo.triggers_always_shown = triggersAlwaysShown;
		}
	}

	return B_OK;
}


status_t
DesktopSettingsPrivate::Save(uint32 mask)
{
	BPath basePath;
	status_t status = _GetPath(basePath);
	if (status < B_OK)
		return status;

	if (mask & kWorkspacesSettings) {
		BPath path(basePath);
		if (path.Append("workspaces") == B_OK) {
			BMessage settings('asws');
			settings.AddInt32("count", fWorkspacesCount);

			for (int32 i = 0; i < kMaxWorkspaces; i++) {
				settings.AddMessage("workspace", &fWorkspaceMessages[i]);
			}

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & kFontSettings) {
		BPath path(basePath);
		if (path.Append("fonts") == B_OK) {
			BMessage settings('asfn');

			settings.AddString("plain family", fPlainFont.Family());
			settings.AddString("plain style", fPlainFont.Style());
			settings.AddFloat("plain size", fPlainFont.Size());

			settings.AddString("bold family", fBoldFont.Family());
			settings.AddString("bold style", fBoldFont.Style());
			settings.AddFloat("bold size", fBoldFont.Size());

			settings.AddString("fixed family", fFixedFont.Family());
			settings.AddString("fixed style", fFixedFont.Style());
			settings.AddFloat("fixed size", fFixedFont.Size());

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & kMouseSettings) {
		BPath path(basePath);
		if (path.Append("mouse") == B_OK) {
			BMessage settings('asms');
			settings.AddInt32("mode", (int32)fMouseMode);

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & kAppearanceSettings) {
		BPath path(basePath);
		if (path.Append("appearance") == B_OK) {
			BMessage settings('aslk');
			settings.AddFloat("font size", fMenuInfo.font_size);
			settings.AddString("font family", fMenuInfo.f_family);
			settings.AddString("font style", fMenuInfo.f_style);
			settings.AddInt32("bg color", (const int32&)fMenuInfo.background_color);
			settings.AddInt32("separator", fMenuInfo.separator);
			settings.AddBool("click to open", fMenuInfo.click_to_open);
			settings.AddBool("triggers always shown", fMenuInfo.triggers_always_shown);
			// TODO: more appearance settings

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	return status;
}


void
DesktopSettingsPrivate::SetDefaultPlainFont(const ServerFont &font)
{
	fPlainFont = font;
	Save(kFontSettings);
}


const ServerFont &
DesktopSettingsPrivate::DefaultPlainFont() const
{
	return fPlainFont;
}


void
DesktopSettingsPrivate::SetDefaultBoldFont(const ServerFont &font)
{
	fBoldFont = font;
	Save(kFontSettings);
}


const ServerFont &
DesktopSettingsPrivate::DefaultBoldFont() const
{
	return fBoldFont;
}


void
DesktopSettingsPrivate::SetDefaultFixedFont(const ServerFont &font)
{
	fFixedFont = font;
	Save(kFontSettings);
}


const ServerFont &
DesktopSettingsPrivate::DefaultFixedFont() const
{
	return fFixedFont;
}


void
DesktopSettingsPrivate::SetScrollBarInfo(const scroll_bar_info& info)
{
	fScrollBarInfo = info;
	Save(kAppearanceSettings);
}


const scroll_bar_info&
DesktopSettingsPrivate::ScrollBarInfo() const
{
	return fScrollBarInfo;
}


void
DesktopSettingsPrivate::SetMenuInfo(const menu_info& info)
{
	fMenuInfo = info;
	Save(kAppearanceSettings);
}


const menu_info&
DesktopSettingsPrivate::MenuInfo() const
{
	return fMenuInfo;
}


void
DesktopSettingsPrivate::SetMouseMode(const mode_mouse mode)
{
	fMouseMode = mode;
	Save(kMouseSettings);
}


mode_mouse
DesktopSettingsPrivate::MouseMode() const
{
	return fMouseMode;
}


bool
DesktopSettingsPrivate::FocusFollowsMouse() const
{
	return MouseMode() != B_NORMAL_MOUSE;
}


void
DesktopSettingsPrivate::SetWorkspacesCount(int32 number)
{
	if (number < 1)
		number = 1;
	else if (number > kMaxWorkspaces)
		number = kMaxWorkspaces;

	fWorkspacesCount = number;
}


int32
DesktopSettingsPrivate::WorkspacesCount() const
{
	return fWorkspacesCount;
}


void
DesktopSettingsPrivate::SetWorkspacesMessage(int32 index, BMessage& message)
{
	if (index < 0 || index > kMaxWorkspaces)
		return;

	fWorkspaceMessages[index] = message;
}


const BMessage*
DesktopSettingsPrivate::WorkspacesMessage(int32 index) const
{
	if (index < 0 || index > kMaxWorkspaces)
		return NULL;

	return &fWorkspaceMessages[index];
}


//	#pragma mark - read access


DesktopSettings::DesktopSettings(Desktop* desktop)
	:
	fSettings(desktop->fSettings)
{
	if (!desktop->fWindowLock.IsReadLocked() && !desktop->fWindowLock.IsWriteLocked())
		debugger("desktop not locked when trying to access settings");
}


void
DesktopSettings::GetDefaultPlainFont(ServerFont &font) const
{
	font = fSettings->DefaultPlainFont();
}


void
DesktopSettings::GetDefaultBoldFont(ServerFont &font) const
{
	font = fSettings->DefaultBoldFont();
}


void
DesktopSettings::GetDefaultFixedFont(ServerFont &font) const
{
	font = fSettings->DefaultFixedFont();
}


void
DesktopSettings::GetScrollBarInfo(scroll_bar_info& info) const
{
	info = fSettings->ScrollBarInfo();
}


void
DesktopSettings::GetMenuInfo(menu_info& info) const
{
	info = fSettings->MenuInfo();
}


mode_mouse
DesktopSettings::MouseMode() const
{
	return fSettings->MouseMode();
}


bool
DesktopSettings::FocusFollowsMouse() const
{
	return fSettings->FocusFollowsMouse();
}


int32
DesktopSettings::WorkspacesCount() const
{
	return fSettings->WorkspacesCount();
}


const BMessage*
DesktopSettings::WorkspacesMessage(int32 index) const
{
	return fSettings->WorkspacesMessage(index);
}


//	#pragma mark - write access


LockedDesktopSettings::LockedDesktopSettings(Desktop* desktop)
	:
	fSettings(desktop->fSettings),
	fDesktop(desktop)
{
	// TODO: this only works in MultiLocker's DEBUG mode
#if 0
	if (desktop->fWindowLock.IsReadLocked())
		debugger("desktop read locked when trying to change settings");
#endif

	fDesktop->LockAllWindows();
}


LockedDesktopSettings::~LockedDesktopSettings()
{
	fDesktop->UnlockAllWindows();
}


void
LockedDesktopSettings::SetDefaultPlainFont(const ServerFont &font)
{
	fSettings->SetDefaultPlainFont(font);
}


void
LockedDesktopSettings::SetDefaultBoldFont(const ServerFont &font)
{
	fSettings->SetDefaultBoldFont(font);
}


void
LockedDesktopSettings::SetDefaultFixedFont(const ServerFont &font)
{
	fSettings->SetDefaultFixedFont(font);
}


void
LockedDesktopSettings::SetScrollBarInfo(const scroll_bar_info& info)
{
	fSettings->SetScrollBarInfo(info);
}


void
LockedDesktopSettings::SetMenuInfo(const menu_info& info)
{
	fSettings->SetMenuInfo(info);
}


void
LockedDesktopSettings::SetMouseMode(const mode_mouse mode)
{
	fSettings->SetMouseMode(mode);
}


