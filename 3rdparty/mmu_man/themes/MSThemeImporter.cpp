/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * MSThemeImporter class
 */

#include <Debug.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Message.h>
#include <String.h>
#include <File.h>
#include <Path.h>
#include <UTF8.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <be_apps/Tracker/Background.h>
#include <fs_info.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "UITheme.h"
#include "Utils.h"
#include "MSThemeImporter.h"


// references:
// http://www.virtualplastic.net/html/desk_reg.html
// http://blogs.technet.com/josebda/archive/2007/08/06/personalizing-the-desktop-on-a-windows-server-2008-server-core-install.aspx
// http://msdn.microsoft.com/en-us/library/bb773190(VS.85).aspx
// color schemes:
// http://www.debbiesthemes.com/utilities/color_scheme.html
// http://www.endolith.com/wordpress/2008/08/03/wine-colors/


#define DEBUG_TI
#ifdef DEBUG_TI
#define FENTRY PRINT(("ThemeImporter[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif

static struct ui_color_map {
	const char *wname;
	const char *name1;
	const char *name2;
} sUIColorMap[] = {
/*
	{ "ActiveBorder" },
	{ "ActiveTitle" },
	{ "AppWorkspace", NOT B_UI_PANEL_BACKGROUND_COLOR, NULL },
	{ "Background" },
	{ "ButtonAlternateFace" },
	{ "ButtonDkShadow" },
*/
	{ "ButtonFace", B_UI_CONTROL_BACKGROUND_COLOR, B_UI_PANEL_BACKGROUND_COLOR },
	{ "ButtonHilight", B_UI_CONTROL_HIGHLIGHT_COLOR, NULL },

	{ "ButtonLight", B_UI_SHINE_COLOR, NULL },
	{ "ButtonShadow", B_UI_SHADOW_COLOR, NULL },
	{ "ButtonText", B_UI_CONTROL_TEXT_COLOR, B_UI_PANEL_TEXT_COLOR },
/*
	{ "GradientActiveTitle" },
	{ "GradientInactiveTitle" },
	{ "GrayText" },
	{ "Hilight" },
	{ "HilightText" },
*/
	{ "HotTrackingColor", B_UI_DOCUMENT_LINK_COLOR, NULL },
/*
	{ "InactiveBorder" },
	{ "InactiveTitle" },
	{ "InactiveTitleText" },
*/
	{ "InfoText", B_UI_TOOLTIP_TEXT_COLOR, NULL },
	{ "InfoWindow", B_UI_TOOLTIP_BACKGROUND_COLOR, NULL },
	{ "Menu", B_UI_MENU_BACKGROUND_COLOR, NULL },
	{ "MenuBar", B_UI_MENU_BACKGROUND_COLOR, NULL },
	{ "MenuHilight", B_UI_MENU_SELECTED_BACKGROUND_COLOR, NULL },
	{ "MenuText", B_UI_MENU_ITEM_TEXT_COLOR, NULL },
/*
	{ "Scrollbar" },
	{ "TitleText" },
*/
	{ "Window", B_UI_DOCUMENT_BACKGROUND_COLOR, NULL },
	{ "WindowFrame", B_UI_NAVIGATION_BASE_COLOR, B_UI_CONTROL_HIGHLIGHT_COLOR },
	{ "WindowText", B_UI_DOCUMENT_TEXT_COLOR, NULL },
	{ NULL, NULL, NULL },
};

static struct screensaver_map {
	const char *wname;
	const char *name;
} sScreensaverMap[] = {
	//{ "", "Blackness" }
	//{ "", "BSOD" }
	{ "system32\\logon.scr", "Lissart" },
	{ "system32\\scrnsave.scr", "Lissart" },
	{ "system32\\ss3dfo.scr", "Lissart" },
	{ "system32\\ssbezier.scr", "Lissart" },
	{ "system32\\ssflwbox.scr", "Lissart" },
	{ "system32\\ssmarque.scr", "Message" },
	{ "system32\\ssmypics.scr", "Lissart" },
	{ "system32\\ssmyst.scr", "Lissart" },
	{ "system32\\sspipes.scr", "Lissart" },
	{ "system32\\ssstars.scr", /*"Stars"*/"Nebula" },
	{ "system32\\sstext3d.scr", "Lissart" },
	{ NULL, NULL }
};


/*
'[' <REGPATH> [|.A|.W] ']'
left to map:
AppEvents\\Schemes\\Apps\\.Default\\AppGPFault\\.Current
AppEvents\\Schemes\\Apps\\.Default\\DeviceConnect\\.Current
AppEvents\\Schemes\\Apps\\.Default\\DeviceDisconnect\\.Current
AppEvents\\Schemes\\Apps\\.Default\\DeviceFail\\.Current
AppEvents\\Schemes\\Apps\\.Default\\LowBatteryAlarm\\.Current
AppEvents\\Schemes\\Apps\\.Default\\MenuCommand\\.Current
AppEvents\\Schemes\\Apps\\.Default\\MenuPopup\\.Current
AppEvents\\Schemes\\Apps\\.Default\\PrintComplete\\.Current
AppEvents\\Schemes\\Apps\\.Default\\RingIn\\.Current
AppEvents\\Schemes\\Apps\\.Default\\Ringout\\.Current
AppEvents\\Schemes\\Apps\\.Default\\SystemHand\\.Current
AppEvents\\Schemes\\Apps\\.Default\\SystemQuestion\\.Current
AppEvents\\Schemes\\Apps\\.Default\\SystemStartMenu\\.Current
AppEvents\\Schemes\\Apps\\.Default\\WindowsLogoff\\.Current
AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon\\.Current
AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current
AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current
*/

static struct sounds_map {
	const char *wname;
	const char *name;
} sSoundsMap[] = {
	//{ "", "BeShare-DLFinished" },
	//{ "", "BeShare-InactivChat" },
	//{ "", "BeShare-Name Said" },
	//{ "", "BeShare-NoComplete" },
	//{ "", "BeShare-Private Msg" },
	//{ "", "BeShare-PrivateWndw" },
	//{ "", "BeShare-ULFinished" },
	//{ "", "BeShare-ULStarted" },
	//{ "", "BeShare-WatchedUser" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current", "Beep" },
	//{ "", "Capture" },
	//{ "", "IM Connected" },
	//{ "", "IM Disconnected" },
	//{ "", "IM Message Received" },
	//{ "", "IM Status: Available" },
	//{ "", "IM Status: Away" },
	//{ "", "IM Status: Offline" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\SystemExclamation\\.Current", "InfoPopper: Error Message" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\SystemAsterisk\\.Current", "InfoPopper: Important Message" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\SystemNotification\\.Current", "InfoPopper: Information Message" },
	//{ "", "InfoPopper: Progress Message" },
	//{ "", "Key Down" },
	//{ "", "Key Repeat" },
	//{ "", "Key Up" },
	//{ "", "Mouse Down" },
	//{ "", "Mouse Up" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\MailBeep\\.Current", "New E-mail" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\SystemStart\\.Current", "Startup" },
	//{ "", "Vision Nick Notification" },
	//{ "AppEvents\\Schemes\\Apps\\.Default\\RestoreDown\\.Current", "Window Activated" },
	// actually *app* close/open...
	{ "AppEvents\\Schemes\\Apps\\.Default\\Close\\.Current", "Window Close" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\Minimize\\.Current", "Window Minimized" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\Open\\.Current", "Window Open" },
	// or Activated ?
	{ "AppEvents\\Schemes\\Apps\\.Default\\RestoreUp\\.Current", "Window Restored" },
	{ "AppEvents\\Schemes\\Apps\\.Default\\Maximize\\.Current", "Window Zoomed" },
	//{ "", "Yahoo: Buzz" },
	// non official sounds yet
	{ "AppEvents\\Schemes\\Apps\\.Default\\SystemExit\\.Current", "Shutdown" },
	{ NULL, NULL }
};


// #pragma mark - 


MSThemeImporter::MSThemeImporter()
	:ThemeImporter("MSTheme")
{
	FENTRY;
}


MSThemeImporter::~MSThemeImporter()
{
	FENTRY;
}


const char *
MSThemeImporter::Description()
{
	FENTRY;
	return "Imports MSTheme themes";
}
	

status_t
MSThemeImporter::FetchThemes()
{
	FENTRY;
	status_t err = ENOENT;
	BVolumeRoster r;
	BVolume v;

	r.Rewind();
	while (r.GetNextVolume(&v) == B_OK) {
		fs_info info;
		if (fs_stat_dev(v.Device(), &info) != B_OK)
			continue;
		if (strcmp(info.fsh_name, "ntfs"))
			continue;
		BDirectory dir;
		if (v.GetRootDirectory(&dir) != B_OK)
			continue;
		if (dir.SetTo(&dir, "WINDOWS/Resources/Themes") == B_OK) {
			if (ScanDirectory(dir))
				err = B_OK;
		}
#if 0
		if (v.GetRootDirectory(&dir) != B_OK)
			continue;
		if (dir.SetTo(&dir, "Documents and Settings/revol/Mes documents") == B_OK) {
			if (ScanDirectory(dir, 1))
				err = B_OK;
		}
#endif
#if 1
		// that's slow...
		if (v.GetRootDirectory(&dir) != B_OK)
			continue;
		if (dir.SetTo(&dir, "Documents and Settings") == B_OK) {
			if (ScanDirectory(dir, 3))
				err = B_OK;
		}
#endif
	}

	return err;
}


status_t
MSThemeImporter::ImportNextTheme(BMessage **theme)
{
	FENTRY;
	status_t err;

	BEntry *entry = (BEntry *)fEntryList.RemoveItem(0L);
	// no more
	if (!entry)
		return ENOENT;
	BPath path;
	err = entry->GetPath(&path);
	delete entry;
	if (err < B_OK)
		return err;
	BFile file;
	err = file.SetTo(path.Path(), B_READ_ONLY);
	if (err < B_OK)
		return err;
	BVolume vol;
	err = file.GetVolume(&vol);
	if (err < B_OK)
		return err;

	PRINT(("MSThemeImporter: importing from '%s'\n", path.Path()));

	// get some standard folders
	BDirectory rootDir;
	err = vol.GetRootDirectory(&rootDir);
	if (err < B_OK)
		return err;

	BString encodedContent;
	off_t size;
	char *buf;
	err = file.GetSize(&size);
	if (err < B_OK)
		return err;
	buf = encodedContent.LockBuffer((size_t)size+1);
	memset(buf, 0, (size_t)size+1);
	if (file.Read(buf, (size_t)size) < size)
		return EIO;
	encodedContent.UnlockBuffer();
	int32 encodedLength = (int32)size;
	BString content;
	int32 contentLength = (int32)size * 2;
	int32 state = 0;
	buf = content.LockBuffer(contentLength+1);
	memset(buf, 0, contentLength+1);
	err = convert_to_utf8(B_MS_WINDOWS_CONVERSION, 
		encodedContent.String(), &encodedLength, 
		buf, &contentLength,
		&state);
	content.UnlockBuffer();
	if (err < B_OK)
		return err;
	//PRINT(("'%s'\n", content.String()));
	// strip DOS CR
	// RemoveSet() seems to be buggy on BeOS (strips UTF-8 chars ???)
	content.RemoveAll("\r");

	// is it really a theme file ?
	if (content.IFindFirst("[Theme]") < 0)
		return ENOENT;

	BMessage global(Z_THEME_MESSAGE_WHAT);
	BMessage info;
	BMessage decor;
	BMessage backgrounds;
	BMessage deskbar;
	BMessage ui_settings;
	BMessage screensaver;
	BMessage sounds;

	BString leaf(path.Leaf());
	leaf.RemoveLast(".theme");
	info.AddString(Z_THEME_NAME, leaf);
	global.AddString(Z_THEME_LOCATION, path.Path());

	BString themeScreensaver("Blackness");

	BString themeWindowDecor("Win2k");
	int32 themeWindowDecorR5 = R5_DECOR_WIN95;
	
	BPath themeWallpaperPath;
	bool themeWallpaperTiled = false;
	int32 themeWallpaperStyle = 0;
	BPoint themeWallpaperOrigin(0, 0);

	BString section;

	// on each line
	while (content.Length()) {
		BString line;
		content.MoveInto(line, 0, content.FindFirst('\n'));
		content.RemoveFirst("\n");
		//PRINT((":'%s'\n", line.String()));
		if (line.Length() < 1)
			continue;
		if (line[0] == ';')
			continue;
		if (line[0] == '[') {
			section = line;
			continue;
		}
		BString key;
		BString value;
		line.MoveInto(key, 0, line.FindFirst('='));
		line.RemoveFirst("=");
		value = line;
		if (section == "[Theme]") {
		} else if (section == "[boot]") {
			if (!key.ICompare("SCRNSAVE.EXE")) {
				for (int i = 0; sScreensaverMap[i].wname; i++) {
					if (value.IFindFirst(sScreensaverMap[i].wname) > -1) {
						themeScreensaver = sScreensaverMap[i].name;
						PRINT(("screensaver: %s -> %s\n", sScreensaverMap[i].wname, sScreensaverMap[i].name));
					}
				}
			}
		// My Computer
		} else if (section == "[CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon]") {
			//DefaultValue=%WinDir%explorer.exe,0
		// My Documents
		} else if (section == "[CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon]") {
			//DefaultValue=%WinDir%SYSTEM32\mydocs.dll,0
		// My Network Places
		} else if (section == "[CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon]") {
			//DefaultValue=%WinDir%SYSTEM32\shell32.dll,17
		// Recycle Bin
		} else if (section == "[CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon]") {
			//full=%WinDir%SYSTEM32\shell32.dll,32
			//empty=%WinDir%SYSTEM32\shell32.dll,31
		} else if (section == "[Control Panel\\Colors]") {
			int r, g, b;
			int nColors = sscanf(value.String(), "%d %d %d", &r, &g, &b);
			if (nColors < 3) {
				PRINT(("nColors %d < 3\n", nColors));
				continue;
			}
			rgb_color color = { r, g, b, 255 };

			if (!key.ICompare("Background")) {
				AddRGBColor(backgrounds, "be:desktop_color", color);
				continue;
			}
			for (int i = 0; sUIColorMap[i].wname; i++) {
				if (!key.ICompare(sUIColorMap[i].wname)) {
					if (sUIColorMap[i].name1)
						AddRGBColor(ui_settings, sUIColorMap[i].name1, color);
					if (sUIColorMap[i].name2)
						AddRGBColor(ui_settings, sUIColorMap[i].name2, color);
					break;
				}
			}
		} else if (section == "[Control Panel\\Desktop]") {
			if (!key.ICompare("Wallpaper")) {
				BPath p;
				if (value.Length() < 1)
					continue;
				
				err = ParseWinPath(rootDir, value.String(), p);
				if (err < B_OK)
					continue;
				PRINT(("Wallpaper at '%s'\n", p.Path()));
				themeWallpaperPath = p;
			} else if (!key.ICompare("TileWallpaper")) {
				if (value != 0)
					themeWallpaperTiled = true;
			} else if (!key.ICompare("WallpaperStyle")) {
				int v;
				int n = sscanf(value.String(), "%d", &v);
				if (n > 0)
					themeWallpaperStyle = v;
			} else if (!key.ICompare("WallpaperOriginX")) {
				int v;
				int n = sscanf(value.String(), "%d", &v);
				if (n > 0)
					themeWallpaperOrigin.x = v;
			} else if (!key.ICompare("WallpaperOriginY")) {
				int v;
				int n = sscanf(value.String(), "%d", &v);
				if (n > 0)
					themeWallpaperOrigin.y = v;
			} else if (!key.ICompare("Pattern")) {
				;
			} else if (!key.ICompare("ScreenSaveActive")) {
				// force back to none
				if (value == "0")
					themeScreensaver = "Blackness";
			}
		} else if (section == "[Metrics]") {
			;
		} else if (section == "[VisualStyles]") {
			if (!key.ICompare("Path")) {
				if (value.IFindFirst("luna.msstyles") > -1) {
					// map Zeta decor
					themeWindowDecor = "Redmond";
				} else if (value.IFindFirst("aero.msstyles") > -1) {
					;
				} // else use Win2k
			}
			if (!key.ICompare("ColorStyle")) {
					// use the greyish verison
					if (value == "Metallic" && themeWindowDecor == "Redmond")
						themeWindowDecor = "Seattle";
			}
		} else if (section.IFindFirst("AppEvents\\Schemes\\Apps") > -1) {
			if (key != "DefaultValue")
				continue;
			// system sound...
			for (int i = 0; sSoundsMap[i].wname; i++) {
				if (section.IFindFirst(sSoundsMap[i].wname) > -1) {
					BString encodedPath;
					if (section.IFindFirst(".W]") > -1) {
						// Wide Char weirdly encoded path...
						// skip for now.
						break;
						// someone knows this +AOk- stuff ??
					} else {
						// ANSI path (.A or none specified)
						// but it's been converted to UTF-8 already
						// so we just use that one.
						encodedPath = value;
					}
					PRINT(("Sound: '%s' -> '%s' (%s)\n", section.String(), sSoundsMap[i].name, value.String()));
					if (encodedPath.Length() == 0)
						break;
					BPath soundPath;
					if (ParseWinPath(rootDir, encodedPath.String(), soundPath) < B_OK)
						break;
					BMessage sound('SndI');
					sound.AddString("sounds:file", soundPath.Path());
					sound.AddFloat("sounds:volume", 1.0);
					if (sounds.ReplaceMessage(sSoundsMap[i].name, &sound) < B_OK)
						sounds.AddMessage(sSoundsMap[i].name, &sound);
				}
			}
		} else {
			;//PRINT(("MSThemeImporter: unknown section '%s', line '%s'\n", section.String(), line.String()));
		}
	}

	// apply settings now we that have everything
	
	// force Deskbar at the bottom... it's Windows, right?
	// XXX: I couldn't find a reg key for that in any theme,
	// but likely there is one.
	deskbar_location loc = B_DESKBAR_BOTTOM;
	bool expanded = true;
	deskbar.AddInt32("db:location", (int32)loc);
	deskbar.AddBool("db:expanded", expanded);

	// window decor
	decor.AddString("window:decor", themeWindowDecor.String());
	// fallbacks
	decor.AddString("window:decor", "Win95");
	decor.AddString("window:decor", "Win2k");
	decor.AddString("window:decor", "Win");
	decor.AddString("window:decor", "Redmond");
	decor.AddString("window:decor", "Seattle");
	decor.AddString("window:decor", "WinDecorator");
	decor.AddInt32("window:R5:decor", themeWindowDecorR5);

	// wallpaper
	if (themeWallpaperPath.InitCheck() == B_OK) {
		backgrounds.AddString(B_BACKGROUND_IMAGE, themeWallpaperPath.Path());
		backgrounds.AddInt32(B_BACKGROUND_WORKSPACES, 0xffffffff);
		int32 wallpaperMode = B_BACKGROUND_MODE_CENTERED;
		switch (themeWallpaperStyle) {
			case 0: // centered or tiled
				if (themeWallpaperTiled)
					wallpaperMode = B_BACKGROUND_MODE_TILED;
				break;
			case 2: // stretched
				wallpaperMode = B_BACKGROUND_MODE_SCALED;
				break;
			case 3: // resized with correct ratio
				// let's keep centered
				break;
			case 4: // resized and cropped with correct ratio
				break;
		}
		backgrounds.AddInt32(B_BACKGROUND_MODE, wallpaperMode);
		backgrounds.AddPoint(B_BACKGROUND_ORIGIN, themeWallpaperOrigin);
		//XXX: there is a reg key for that, is it used ?
		//backgrounds.AddBool(B_BACKGROUND_ERASE_TEXT, erasetext);
	}

	// ui settings: disable Z-Snake
	ui_settings.AddBool(B_UI_MENU_ZSNAKE, false);

	screensaver.AddString("screensaver:modulename", themeScreensaver.String());

	global.AddMessage(Z_THEME_INFO_MESSAGE, &info);
	global.AddMessage(Z_THEME_WINDOW_DECORATIONS, &decor);
	global.AddMessage(Z_THEME_BACKGROUND_SETTINGS, &backgrounds);
	global.AddMessage(Z_THEME_DESKBAR_SETTINGS, &deskbar);
	global.AddMessage(Z_THEME_UI_SETTINGS, &ui_settings);
	global.AddMessage(Z_THEME_SCREENSAVER_SETTINGS, &screensaver);
	global.AddMessage(Z_THEME_SOUNDS_SETTINGS, &sounds);
	*theme = new BMessage(global);
	//global.PrintToStream();

	return B_OK;
}


status_t
MSThemeImporter::EndImports()
{
	FENTRY;
	return B_OK;
}


bool
MSThemeImporter::ScanDirectory(BDirectory &dir, int depth)
{
	//FENTRY;
	BEntry ent;
	bool found = false;

	if (depth == 0)
		return false;

	while (dir.GetNextEntry(&ent) == B_OK) {
		if (ent.IsDirectory()) {
			BDirectory subdir(&ent);
			if (ScanDirectory(subdir, depth - 1))
				found = true;
			continue;
		}
		if (!ent.IsFile())
			continue;
		char name[B_FILE_NAME_LENGTH];
		if (ent.GetName(name) != B_OK)
			continue;
		if (!strstr(name, ".theme"))
			continue;
		printf("found '%s'\n", name);
		fEntryList.AddItem(new BEntry(ent));
		found = true;
	}

	return found;
}


status_t
MSThemeImporter::ParseWinPath(BDirectory &rootDir, const char *from, BPath &to)
{
	status_t err;
	//FENTRY;
	//return ENOENT;
	//BDirectory winDir(&rootDir, "WINDOWS");
	//BDirectory resourceDir(&winDir, "Resources");
	PRINT(("ParseWinPath(, %s, )\n", from));
	BString p(from);
	//p.ReplaceAll('\\', '/');
	to.SetTo(&rootDir, ".");
	
	if (p.IFindFirst("%WinDir%") == 0) {
		to.Append("WINDOWS");
		p.IReplaceFirst("%WinDir%", "");
	}
	if (p.IFindFirst("%ResourceDir%") == 0) {
		to.Append("WINDOWS/Resources");
		p.IReplaceFirst("%ResourceDir%", "");
	}
	//TODO: %ThemeDir%
	
	//TODO: split and check for correct case
	BDirectory dir;
	while (p.Length()) {
		BString component;
		//PRINT(("ParseWinPath: p.L %d p '%s'\n", p.Length(), p.String()));
		int32 len = p.FindFirst('\\');
		if (len < 0)
			len = p.Length();
		p.MoveInto(component, 0, len);
		p.RemoveFirst("\\");
		err = dir.SetTo(to.Path());
		if (err < B_OK)
			return err;
		//PRINT(("ParseWinPath: at '%s'\n", to.Path()));
		//PRINT(("ParseWinPath: testing '%s'\n", component.String()));
		if (dir.Contains(component.String())) {
			to.Append(component.String());
			continue;
		}
		// can't find as is, try various capitalizations
		// (caseless fs SUXOR)
		component.Capitalize();
		//PRINT(("ParseWinPath: testing '%s'\n", component.String()));
		if (dir.Contains(component.String())) {
			to.Append(component.String());
			continue;
		}
		component.CapitalizeEachWord();
		//PRINT(("ParseWinPath: testing '%s'\n", component.String()));
		if (dir.Contains(component.String())) {
			to.Append(component.String());
			continue;
		}
		component.ToLower();
		//PRINT(("ParseWinPath: testing '%s'\n", component.String()));
		if (dir.Contains(component.String())) {
			to.Append(component.String());
			continue;
		}
		component.ToUpper();
		//PRINT(("ParseWinPath: testing '%s'\n", component.String()));
		if (dir.Contains(component.String())) {
			to.Append(component.String());
			continue;
		}
		PRINT(("ParseWinPath: failed\n"));
		// TODO: loop on GetNextEntry and ICompare ?
		return ENOENT;
	}
	PRINT(("ParseWinPath(, %s, ) -> %s\n", from, to.Path()));
	return B_OK;
}

