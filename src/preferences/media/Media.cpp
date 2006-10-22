/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Sikosis
 *		Jérôme Duval
 */


#include "Media.h"

#include <StorageKit.h>
#include <Roster.h>
#include <String.h>
#include <stdio.h>


Media::Media() 
	: BApplication("application/x-vnd.Haiku-Media")
{
	BRect rect(32,64,637,442);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(),B_READ_ONLY);
		if (file.InitCheck()==B_OK) {
			char buffer[255];
			ssize_t size = 0;
			while ((size = file.Read(buffer, 255)) > 0) {
				int32 i = 0;
				while (buffer[i] == '#') {
					while (i < size && buffer[i] != '\n')
						i++;
					i++;
				}
				int32 a, b, c, d;
				if (sscanf(&buffer[i], " rect = %li,%li,%li,%li", &a, &b, &c, &d) > 0) {
					if (c - a >= rect.IntegerWidth()) {
						rect.left = a;
						rect.right = c;
					}
					if (d - b >= rect.IntegerHeight()) {
						rect.top = b;
						rect.bottom = d;
					}
				}
			}		
		}
	}

	fWindow = new MediaWindow(rect);
	fWindow->SetSizeLimits(605.0, 10000.0, 378.0, 10000.0);

	be_roster->StartWatching(BMessenger(this));
}


Media::~Media()
{
	be_roster->StopWatching(BMessenger(this));
}


status_t
Media::InitCheck()
{
	if (fWindow)
		return fWindow->InitCheck();
	return B_OK;
}


void
Media::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
			fWindow->PostMessage(message);
			break;

	    default:
    	    BApplication::MessageReceived(message);
        	break;
    }
}


//	#pragma mark -


int
main(int, char**)
{
	Media app;
	if (app.InitCheck() == B_OK)
		app.Run();

   return 0;
}

