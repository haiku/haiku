#include <StorageKit.h>
#include <Screen.h>

#include "ScreenSettings.h"

const char ScreenSettings::fScreenSettingsFile[] = "Screen_data";

ScreenSettings::ScreenSettings()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid())
		; //Debugger() ?
	
	fWindowFrame.Set( 0, 0, 356, 202 );
	
	BRect screenFrame = screen.Frame();
		
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
	{
		path.Append(fScreenSettingsFile);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK)
		{
			BPoint point;
			file.Read(&point, sizeof(BPoint));
			fWindowFrame.OffsetTo(point);
			
			if (screen.Frame().right >= fWindowFrame.right
				&& screen.Frame().bottom >= fWindowFrame.bottom)
				return;
		}
	}
	fWindowFrame.OffsetTo((screenFrame.Width() - fWindowFrame.Width()) / 2,
		(screenFrame.Height() - fWindowFrame.Height()) /2 );
}


ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(fScreenSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		BPoint point(fWindowFrame.LeftTop());
		file.Write(&point, sizeof(BPoint));
	}
}


void
ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}
