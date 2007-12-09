/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <DiskDeviceRoster.h>
#include <Window.h>


class BDiskDevice;
class BPartition;
class BMenu;
class DiskView;
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

private:
			void				_ScanDrives();
			void				_ScanFileSystems();
			void				_AdaptToSelectedPartition();


			BDiskDeviceRoster	fDDRoster;
			PartitionListView*	fListView;
			DiskView*			fDiskView;

			BMenu*				fInitMenu;
};


#endif // MAIN_WINDOW_H
