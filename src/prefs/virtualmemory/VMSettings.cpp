#ifndef VM_SETTINGS_H
#include "VMSettings.h"
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _PATH_H
#include <Path.h>
#endif
#ifndef _FINDDIRECTORY_H
#include <FindDirectory.h>
#endif

#include <stdio.h>

const char VMSettings::kVMSettingsFile[] = "VM_data";

VMSettings::VMSettings()
{//VMSettings::VMSettings

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK)
	{
		path.Append(kVMSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			be_app->PostMessage(B_QUIT_REQUESTED);
		// Now read in the data
		if (file.Read(&fcorner, sizeof(BPoint)) != sizeof(BPoint))
			be_app->PostMessage(B_QUIT_REQUESTED);
	}
	printf("VM settings file read.\n");
	printf("=========================\n");
	printf("fcorner read in as ");
	fcorner.PrintToStream();

	fWindowFrame.left=fcorner.x;
	fWindowFrame.top=fcorner.y;
	fWindowFrame.right=fWindowFrame.left+269;
	fWindowFrame.bottom=fWindowFrame.top+172;
	
	//Check to see if the co-ords of the window are in the range of the Screen
	BScreen screen;
		if (screen.Frame().right >= fWindowFrame.right
			&& screen.Frame().bottom >= fWindowFrame.bottom)
		return;
	// If they are not, lets just stick the window in the middle
	// of the screen.
	fWindowFrame = screen.Frame();
	fWindowFrame.left = (fWindowFrame.right-269)/2;
	fWindowFrame.right = fWindowFrame.left + 269;
	fWindowFrame.top = (fWindowFrame.bottom-172)/2;
	fWindowFrame.bottom = fWindowFrame.top + 172;

}//VMSettings::VMSettings

VMSettings::~VMSettings()
{//VMSettings::~VMSettings
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kVMSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
	{
		file.Write(&fcorner, sizeof(BPoint));
	}
}//MouseSettings::~MouseSettings

void VMSettings::SetWindowPosition(BRect f)
{//VMSettings::SetWindowFrame
	fcorner.x=f.left;
	fcorner.y=f.top;
}//VMSettings::SetWindowFrame