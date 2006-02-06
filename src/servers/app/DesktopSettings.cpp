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

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
//#include <DataIO./h>
#include <Path.h>


DesktopSettings::Private::Private()
	: BLocker("DesktopSettings_Private")
{
	// if the on-disk settings are not complete, the defaults will be kept
	_SetDefaults();
	_Load();
}


DesktopSettings::Private::~Private()
{
}


void
DesktopSettings::Private::_SetDefaults()
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
	fMenuInfo.font_size = 12.0;
// TODO: ...
//	fMenuInfo.f_family;
//	fMenuInfo.f_style;
//	fMenuInfo.background_color = gColorSet->menu_background;
	// look of the separator (R5: (0, 1, 2), default ?)
	fMenuInfo.separator = 0;
	fMenuInfo.click_to_open = true;
	fMenuInfo.triggers_always_shown = false;

	fWorkspacesCount = 4;
}


status_t
DesktopSettings::Private::_GetPath(BPath& path)
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
DesktopSettings::Private::_Load()
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
			while (i < 32
				&& settings.FindMessage("workspace", i, &fWorkspaceMessages[i]) == B_OK) {
				i++;
			}
		}
	}

	return B_ERROR;
}


status_t
DesktopSettings::Private::Save(uint32 mask)
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

	return status;
}


void
DesktopSettings::Private::SetDefaultPlainFont(const ServerFont &font)
{
	fPlainFont = font;
}


const ServerFont &
DesktopSettings::Private::DefaultPlainFont() const
{
	return fPlainFont;
}


void
DesktopSettings::Private::SetDefaultBoldFont(const ServerFont &font)
{
	fBoldFont = font;
}


const ServerFont &
DesktopSettings::Private::DefaultBoldFont() const
{
	return fBoldFont;
}


void
DesktopSettings::Private::SetDefaultFixedFont(const ServerFont &font)
{
	fFixedFont = font;
}


const ServerFont &
DesktopSettings::Private::DefaultFixedFont() const
{
	return fFixedFont;
}


void
DesktopSettings::Private::SetScrollBarInfo(const scroll_bar_info& info)
{
	fScrollBarInfo = info;
}


const scroll_bar_info&
DesktopSettings::Private::ScrollBarInfo() const
{
	return fScrollBarInfo;
}


void
DesktopSettings::Private::SetMenuInfo(const menu_info& info)
{
	fMenuInfo = info;
}


const menu_info&
DesktopSettings::Private::MenuInfo() const
{
	return fMenuInfo;
}


void
DesktopSettings::Private::SetMouseMode(const mode_mouse mode)
{
	fMouseMode = mode;
}


mode_mouse
DesktopSettings::Private::MouseMode() const
{
	return fMouseMode;
}


bool
DesktopSettings::Private::FocusFollowsMouse() const
{
	return MouseMode() != B_NORMAL_MOUSE;
}


void
DesktopSettings::Private::SetWorkspacesCount(int32 number)
{
	if (number < 1)
		number = 1;
	else if (number > kMaxWorkspaces)
		number = kMaxWorkspaces;

	fWorkspacesCount = number;
}


int32
DesktopSettings::Private::WorkspacesCount() const
{
	return fWorkspacesCount;
}


void
DesktopSettings::Private::SetWorkspacesMessage(int32 index, BMessage& message)
{
	if (index < 0 || index > kMaxWorkspaces)
		return;

	fWorkspaceMessages[index] = message;
}


const BMessage*
DesktopSettings::Private::WorkspacesMessage(int32 index) const
{
	if (index < 0 || index > kMaxWorkspaces)
		return NULL;

	return &fWorkspaceMessages[index];
}



//	#pragma mark -


DesktopSettings::DesktopSettings(Desktop* desktop)
{
	fSettings = desktop->fSettings;
	fSettings->Lock();
}


DesktopSettings::~DesktopSettings()
{
	fSettings->Unlock();
}


void
DesktopSettings::SetDefaultPlainFont(const ServerFont &font)
{
	fSettings->SetDefaultPlainFont(font);
}


void
DesktopSettings::GetDefaultPlainFont(ServerFont &font) const
{
	font = fSettings->DefaultPlainFont();
}


void
DesktopSettings::SetDefaultBoldFont(const ServerFont &font)
{
	fSettings->SetDefaultBoldFont(font);
}


void
DesktopSettings::GetDefaultBoldFont(ServerFont &font) const
{
	font = fSettings->DefaultBoldFont();
}


void
DesktopSettings::SetDefaultFixedFont(const ServerFont &font)
{
	fSettings->SetDefaultFixedFont(font);
}


void
DesktopSettings::GetDefaultFixedFont(ServerFont &font) const
{
	font = fSettings->DefaultFixedFont();
}


void
DesktopSettings::SetScrollBarInfo(const scroll_bar_info& info)
{
	fSettings->SetScrollBarInfo(info);
}


void
DesktopSettings::GetScrollBarInfo(scroll_bar_info& info) const
{
	info = fSettings->ScrollBarInfo();
}


void
DesktopSettings::SetMenuInfo(const menu_info& info)
{
	fSettings->SetMenuInfo(info);
}


void
DesktopSettings::GetMenuInfo(menu_info& info) const
{
	info = fSettings->MenuInfo();
}


void
DesktopSettings::SetMouseMode(const mode_mouse mode)
{
	fSettings->SetMouseMode(mode);
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


void
DesktopSettings::SetWorkspacesCount(int32 number)
{
	fSettings->SetWorkspacesCount(number);
}


int32
DesktopSettings::WorkspacesCount() const
{
	return fSettings->WorkspacesCount();
}


void
DesktopSettings::SetWorkspacesMessage(int32 index, BMessage& message)
{
	fSettings->SetWorkspacesMessage(index, message);
}


const BMessage*
DesktopSettings::WorkspacesMessage(int32 index) const
{
	return fSettings->WorkspacesMessage(index);
}

