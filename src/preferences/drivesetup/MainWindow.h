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
class BMenuItem;
class DiskView;
class PartitionListView;


class MainWindow : public BWindow {
public:
								MainWindow(BRect frame);
	virtual						~MainWindow();

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
			void				_EnabledDisableMenuItems(BDiskDevice* disk,
									partition_id selectedPartition);

			void				_DisplayPartitionError(BString message,
									const BPartition* partition = NULL,
									status_t error = B_OK) const;

			void				_Mount(BDiskDevice* disk,
									partition_id selectedPartition);
			void				_Unmount(BDiskDevice* disk,
									partition_id selectedPartition);
			void				_MountAll();

			void				_Initialize(BDiskDevice* disk,
									partition_id selectedPartition,
									const BString& diskSystemName);


			BDiskDeviceRoster	fDDRoster;
			BDiskDevice*		fCurrentDisk;
			partition_id		fCurrentPartitionID;

			PartitionListView*	fListView;
			DiskView*			fDiskView;

			BMenu*				fDiskMenu;
			BMenu*				fPartitionMenu;
			BMenu*				fInitMenu;
			BMenu*				fCreateMenu;

			BMenuItem*			fFormatMI;
			BMenuItem*			fEjectMI;
			BMenuItem*			fSurfaceTestMI;
			BMenuItem*			fRescanMI;

			BMenuItem*			fCreatePrimaryMI;
			BMenuItem*			fCreateExtendedMI;
			BMenuItem*			fCreateLogicalMI;
			BMenuItem*			fMountMI;
			BMenuItem*			fUnmountMI;
			BMenuItem*			fMountAllMI;
};


#endif // MAIN_WINDOW_H
