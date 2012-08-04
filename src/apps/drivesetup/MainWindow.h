/*
 * Copyright 2002-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <DiskDeviceRoster.h>
#include <Window.h>

#include "Support.h"


class BDiskDevice;
class BPartition;
class BMenu;
class BMenuItem;
class DiskView;
class PartitionListView;


enum {
	MSG_SELECTED_PARTITION_ID	= 'spid'
};


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
			void				ApplyDefaultSettings();

private:
			void				_ScanDrives();

			void				_AdaptToSelectedPartition();
			void				_SetToDiskAndPartition(partition_id diskID,
									partition_id partitionID,
									partition_id parentID);
			void				_UpdateMenus(BDiskDevice* disk,
									partition_id selectedPartition,
									partition_id parentID);

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
			void				_Create(BDiskDevice* disk,
									partition_id selectedPartition);
			void				_Delete(BDiskDevice* disk,
									partition_id selectedPartition);


			BDiskDeviceRoster	fDDRoster;
			BDiskDevice*		fCurrentDisk;
			partition_id		fCurrentPartitionID;

			PartitionListView*	fListView;
			DiskView*			fDiskView;

			SpaceIDMap			fSpaceIDMap;

			BMenu*				fDiskMenu;
			BMenu*				fDiskInitMenu;

			BMenu*				fPartitionMenu;
			BMenu*				fFormatMenu;

			BMenuItem*			fWipeMI;
			BMenuItem*			fEjectMI;
			BMenuItem*			fSurfaceTestMI;
			BMenuItem*			fRescanMI;

			BMenuItem*			fCreateMI;
			BMenuItem*			fDeleteMI;
			BMenuItem*			fMountMI;
			BMenuItem*			fUnmountMI;
			BMenuItem*			fMountAllMI;
};


#endif // MAIN_WINDOW_H
