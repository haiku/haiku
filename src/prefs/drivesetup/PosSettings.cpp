#ifndef POS_SETTINGS_H
#include "PosSettings.h"
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

const char PosSettings::kVMSettingsFile[] = "DriveSetup_prefs";

PosSettings::PosSettings()
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
		if (file.Read(&brCorner, sizeof(BPoint)) != sizeof(BPoint))
			be_app->PostMessage(B_QUIT_REQUESTED);
	}
	printf("VM settings file read.\n");
	printf("=========================\n");
	printf("fcorner read in as ");
	fcorner.PrintToStream();
	printf("brCorner read in as ");
	brCorner.PrintToStream();

	fWindowFrame.left=fcorner.x;
	fWindowFrame.top=fcorner.y;
	fWindowFrame.right=brCorner.x;
	fWindowFrame.bottom=brCorner.y;
	
	//Check to see if the co-ords of the window are in the range of the Screen
	BScreen screen;
		if (screen.Frame().right >= fWindowFrame.right
			&& screen.Frame().bottom >= fWindowFrame.bottom)
		return;
	// If they are not, lets just stick the window in the middle
	// of the screen.
	
	//I don't want to deal with this now - MSM
	
	/*fWindowFrame = screen.Frame();
	fWindowFrame.left = (fWindowFrame.right-269)/2;
	fWindowFrame.right = fWindowFrame.left + 269;
	fWindowFrame.top = (fWindowFrame.bottom-172)/2;
	fWindowFrame.bottom = fWindowFrame.top + 172;
*/
}//VMSettings::VMSettings

PosSettings::~PosSettings()
{//VMSettings::~VMSettings
//printf("enter\n");
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;
//printf("find_directory\n");
	path.Append(kVMSettingsFile);
//printf("path.Append\n");
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
//printf("create file\n");
	if (file.InitCheck() == B_OK)
	{
		//printf("in if statement\n");
		file.Write(&fcorner, sizeof(BPoint));
		file.Write(&brCorner, sizeof(BPoint));
	}
//printf("exit\n");
}//MouseSettings::~MouseSettings

void PosSettings::SetWindowPosition(BRect f)
{//VMSettings::SetWindowFrame
	fcorner.x=f.left;
	fcorner.y=f.top;
	brCorner.x=f.right;
	brCorner.y=f.bottom;
brCorner.PrintToStream();
}//VMSettings::SetWindowFrame