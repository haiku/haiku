#include <StorageKit.h>
#include <Screen.h>

#include "ScreenSettings.h"

const char ScreenSettings::fScreenSettingsFile[] = "OBOS_Screen_data";

ScreenSettings::ScreenSettings()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid())
		; //Debugger() ?
		
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
	{
		path.Append(fScreenSettingsFile);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK)
		{
			file.Read(&fWindowFrame, sizeof(BRect));
			
			if (screen.Frame().right >= fWindowFrame.right
				&& screen.Frame().bottom >= fWindowFrame.bottom)
				return;
		}
	}
	
	fWindowFrame = screen.Frame();
	fWindowFrame.left = (fWindowFrame.right / 2) - 178;
	fWindowFrame.top = (fWindowFrame.right / 2) - 101;
	fWindowFrame.right = fWindowFrame.left + 356;
	fWindowFrame.bottom = fWindowFrame.top + 202;
}


ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(fScreenSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		file.Write(&fWindowFrame, sizeof(BRect));
}


void
ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}
