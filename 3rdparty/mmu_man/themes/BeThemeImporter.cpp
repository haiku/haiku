/*
 * BeThemeImporter class
 */

#include <Debug.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Message.h>
#include <String.h>
#include <File.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <be_apps/Tracker/Background.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "UITheme.h"
#include "Utils.h"
#include "BeThemeImporter.h"


#define DEBUG_TI
#ifdef DEBUG_TI
#define FENTRY PRINT(("ThemeImporter[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif

#define QSTR "(name==\"TrackerTheme\")"

static const deskbar_location kDeskbarLocationMap[] = {
	//2:top 1:topright 5:bottom
	B_DESKBAR_LEFT_TOP, B_DESKBAR_RIGHT_TOP, B_DESKBAR_TOP,
	B_DESKBAR_LEFT_BOTTOM, B_DESKBAR_RIGHT_BOTTOM, B_DESKBAR_BOTTOM
};


// #pragma mark - 


BeThemeImporter::BeThemeImporter()
	:ThemeImporter("BeTheme")
{
	FENTRY;
	BVolumeRoster r;
	BVolume v;
	// TODO: query all volumes ?
	r.GetBootVolume(&v);
	fQuery.Clear();
	fQuery.SetVolume(&v);
	fQuery.SetPredicate(QSTR);
}


BeThemeImporter::~BeThemeImporter()
{
	FENTRY;
	fQuery.Clear();
}


const char *
BeThemeImporter::Description()
{
	FENTRY;
	return "Imports BeTheme themes";
}
	

status_t
BeThemeImporter::FetchThemes()
{
	FENTRY;
	return fQuery.Fetch();
}


status_t
BeThemeImporter::ImportNextTheme(BMessage **theme)
{
	FENTRY;
	status_t err;
	BEntry ent;
	int i, j;
	
	err = fQuery.GetNextEntry(&ent);
	if (err < B_OK)
		return err;
	BDirectory dir;
	BDirectory settingsDir;
	BPath path;
	err = ent.GetParent(&dir);
	ent.GetParent(&ent);
	ent.GetPath(&path);
	if (err < 0)
		return err;
	err = settingsDir.SetTo(&dir, "Settings");
	if (err < 0)
		return B_ERROR;
	PRINT(("BeThemeImporter: importing from '%s'\n", path.Path()));

	BMessage global(Z_THEME_MESSAGE_WHAT);
	BMessage info;
	BMessage decor;
	BMessage backgrounds;
	BMessage deskbar;
	info.AddString(Z_THEME_NAME, path.Leaf());
	global.AddString(Z_THEME_LOCATION, path.Path());

	BFile file;
	int32 value;
	int64 llvalue;
	int32 first;

	err = file.SetTo(&settingsDir, "Description", B_READ_ONLY);
	if (err < B_OK)
		return B_ERROR;
	BString str;
	char *buff;
	buff = str.LockBuffer(1024);
	memset(buff, 0, 1024);
	file.Read(buff, 1024-1);
	if (err < B_OK)
		return err;
	str.UnlockBuffer();
	info.AddString(Z_THEME_DESCRIPTION, str.String());
	info.AddString(Z_THEME_SCREENSHOT_FILENAME, "Settings/Preview.jpg");
	
	
	err = file.SetTo(&settingsDir, "Deskbar", B_READ_ONLY);
	if (err < B_OK)
		return B_ERROR;
	str = "";
	buff = str.LockBuffer(1024);
	memset(buff, 0, 1024);
	file.Read(buff, 1024-1);
	if (err < B_OK)
		return err;
	str.UnlockBuffer();
	str.ReplaceSet("\r\n", " ");
	for (i = 0; str.Length() > 0; i++) {
		BString token;
		first = str.FindFirst(' ');
		first = first < 0 ? str.Length() : first + 1;
		str.MoveInto(token, 0, first);
		if (token.Length() < 2)
			continue;
		if (sscanf(token.String(), "POS:%ld", &value) >= 1) {
			value = (value < 6) ? kDeskbarLocationMap[value] : B_DESKBAR_RIGHT_TOP;
			deskbar.AddInt32("db:location", value);
		}
		if (sscanf(token.String(), "EXP:%ld", &value) >= 1) {
			deskbar.AddBool("db:expanded", value != 0);
		}
		if (sscanf(token.String(), "DECOR:%ld", &value) >= 1) {
			value = (value < 4) ? value : 0;
			decor.AddInt32("window:R5:decor", value);
			switch (value) {
				case R5_DECOR_BEOS:
				default:
					decor.AddString("window:decor", "R5");
					// fallbacks
					decor.AddString("window:decor", "Default");
					decor.AddString("window:decor", "ClassicBe");
					decor.AddString("window:decor", "BeDecorator");
					break;
				case R5_DECOR_WIN95:
					decor.AddString("window:decor", "Win95");
					// fallbacks
					decor.AddString("window:decor", "Win2k");
					decor.AddString("window:decor", "Win");
					decor.AddString("window:decor", "Redmond");
					decor.AddString("window:decor", "Seattle");
					decor.AddString("window:decor", "WinDecorator");
					break;
					
				case R5_DECOR_AMIGA:
					decor.AddString("window:decor", "Amiga");
					// fallbacks
					decor.AddString("window:decor", "AmigaOS");
					decor.AddString("window:decor", "AmigaOS4");
					break;
				case R5_DECOR_MAC:
					decor.AddString("window:decor", "Mac");
					// fallbacks
					decor.AddString("window:decor", "MacOS");
					decor.AddString("window:decor", "Baqua");
					decor.AddString("window:decor", "Cupertino");
					decor.AddString("window:decor", "MacDecorator");
					break;
			}
		}
	}
	
	err = file.SetTo(&settingsDir, "WorkSpaces", B_READ_ONLY);
	if (err < B_OK)
		return B_ERROR;
	buff = str.LockBuffer(1024);
	memset(buff, 0, 1024);
	file.Read(buff, 1024-1);
	if (err < B_OK)
		return err;
	str.UnlockBuffer();
	BString line;
	for (i = 0; str.Length() > 0; i++) {
		first = str.FindFirst('\n');
		first = first < 0 ? str.Length() : first + 1;
		str.MoveInto(line, 0, first);
		if (line.Length() < 2)
			break;

		rgb_color color;
		bool erasetext = false;
		int32 mode = B_BACKGROUND_MODE_SCALED;
		//int32 set = 0; // ?
		BPoint offset;
		uint32 workspaces = 0;

		for (j = 0; line.Length() > 0; j++) {
			BString token;
			first = line.FindFirst(' ');
			first = first < 0 ? line.Length() : first + 1;
			line.MoveInto(token, 0, first);
			if (token.Length() < 2)
				break;

			if (sscanf(token.String(), "R:%ld", &value) >= 1) {
				color.red = (int8) value;
			}
			if (sscanf(token.String(), "G:%ld", &value) >= 1) {
				color.green = (int8) value;
			}
			if (sscanf(token.String(), "B:%ld", &value) >= 1) {
				color.blue = (int8) value;
			}
			color.alpha = 255;
			if (sscanf(token.String(), "BGM:%ld", &value) >= 1) {
				mode = value;
			}
			if (sscanf(token.String(), "ERASETEXT:%ld", &value) >= 1) {
				erasetext = value != 0;
			}
			if (sscanf(token.String(), "x:%ld", &value) >= 1) {
				offset.x = value;
			}
			if (sscanf(token.String(), "y:%ld", &value) >= 1) {
				offset.y = value;
			}
			
			if (sscanf(token.String(), "WORKSPACE:%Lx", &llvalue) >= 1) {
				workspaces = (uint32)llvalue;
			}

		}

		BString image(path.Path());
		image << "/Settings/";
		image << "Background" << i << ".jpg";
		
		backgrounds.AddString(B_BACKGROUND_IMAGE, image.String());
		backgrounds.AddInt32(B_BACKGROUND_MODE, mode);
		backgrounds.AddPoint(B_BACKGROUND_ORIGIN, offset);
		backgrounds.AddBool(B_BACKGROUND_ERASE_TEXT, erasetext);
		backgrounds.AddInt32(B_BACKGROUND_WORKSPACES, workspaces);
		AddRGBColor(backgrounds, "be:desktop_color", color);
	}
	
	
	global.AddMessage(Z_THEME_INFO_MESSAGE, &info);
	global.AddMessage(Z_THEME_WINDOW_DECORATIONS, &decor);
	global.AddMessage(Z_THEME_BACKGROUND_SETTINGS, &backgrounds);
	global.AddMessage(Z_THEME_DESKBAR_SETTINGS, &deskbar);
	*theme = new BMessage(global);
	//global.PrintToStream();
	
	return B_OK;
}


status_t
BeThemeImporter::EndImports()
{
	FENTRY;
	fQuery.Clear();
	return B_OK;
}

