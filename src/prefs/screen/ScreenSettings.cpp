#include <StorageKit.h>
#include <Screen.h>

#include "ScreenSettings.h"

const char ScreenSettings::fScreenSettingsFile[] = "RRScreen_Data";

ScreenSettings::ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK)
	{
		path.Append(fScreenSettingsFile);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK && file.Read(&fWindowFrame, sizeof(BRect)) == sizeof(BRect))
		{
			BScreen Screen;
			if (Screen.Frame().right >= fWindowFrame.right
				&& Screen.Frame().bottom >= fWindowFrame.bottom)
				return;
		}
	}
	
	BScreen Screen;
	fWindowFrame = Screen.Frame();
	fWindowFrame.left = (Screen.Frame().right / 2) - 178;
	fWindowFrame.top = (Screen.Frame().right / 2) - 101;
	fWindowFrame.right = fWindowFrame.left + 356;
	fWindowFrame.bottom = fWindowFrame.top + 202;
}

ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(fScreenSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		file.Write(&fWindowFrame, sizeof(BRect));
}

void ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}
