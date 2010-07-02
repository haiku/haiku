/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef DRIVE_SETUP_H
#define DRIVE_SETUP_H

	
#include <Application.h>
#include <Catalog.h>
#include <Message.h>


class BFile;
class MainWindow;

	
class DriveSetup : public BApplication {	
public:
								DriveSetup();
	virtual						~DriveSetup();

	virtual	void				ReadyToRun();
	virtual	bool				QuitRequested();

private:
			status_t			_StoreSettings();
			status_t			_RestoreSettings();
			status_t			_GetSettingsFile(BFile& file,
									bool forWriting) const;

			MainWindow*			fWindow;

			BMessage			fSettings;
};


#endif // DRIVE_SETUP_H
