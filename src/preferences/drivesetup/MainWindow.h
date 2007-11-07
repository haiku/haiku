/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <DiskDeviceRoster.h>
#include <Window.h>


class PosSettings;
class BPartition;
class BDiskDevice;
class PartitionListView;


class MainWindow : public BWindow {
public:
								MainWindow(BRect frame);

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

	// MainWindow
			status_t			StoreSettings(BMessage* archive) const;
			status_t			RestoreSettings(BMessage* archive);

	// These are public for visitor....
			void				AddPartition(BPartition* partition,
									int32 level);
			void				AddDrive(BDiskDevice* device);

private:
			void				_ScanDrives();


			BDiskDeviceRoster	fDDRoster;
			PartitionListView*	fListView;
};


#endif // MAIN_WINDOW_H
