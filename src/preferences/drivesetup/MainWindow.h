/*! \file MainWindow.h
    \brief Header for the MainWindow class.
    
*/

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

	#define MOUNT_MOUNT_ALL_MSG					'mall'
	#define MOUNT_MOUNT_SELECTED_MSG			'msel'
	#define UNMOUNT_UNMOUNT_SELECTED_MSG	'usel'
	#define SETUP_FORMAT_MSG							'sfor'
	#define SETUP_PARTITION_SELECTED_MSG		'spsl'
	#define SETUP_INITIALIZE_MSG							'sini'
	#define OPTIONS_EJECT_MSG							'oeje'
	#define OPTIONS_SURFACE_TEST_MSG			'osut'
	#define RESCAN_IDE_MSG								'ride'
	#define RESCAN_SCSI_MSG								'rscs'

	#include <interface/Window.h>
	#include <storage/DiskDeviceRoster.h>

	// Forward declarations
	class PosSettings;
	class BPartition;
	class BDiskDevice;
	class BColumnListView;
	
	/**
	 * The main window of the app.
	 *
	 * Sets up and displays everything you need for the app.
	 */

	class MainWindow : public BWindow
	{
		private:
			PosSettings*				fSettings;
			BDiskDeviceRoster		fDDRoster;
			BColumnListView*		fListView;
		public:
			MainWindow(BRect frame, PosSettings *fSettings);
			
			bool QuitRequested();
			void MessageReceived(BMessage *message);
			
			// These are public for visitor....
			void AddPartition(BPartition* partition, int32 level);
			void AddDrive(BDiskDevice* device);
	};
	
#endif
