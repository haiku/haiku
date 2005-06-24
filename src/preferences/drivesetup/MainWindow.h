/*! \file MainWindow.h
    \brief Header for the MainWindow class.
    
*/

#ifndef MAIN_WINDOW_H
	
	#define MAIN_WINDOW_H
	
	#define MOUNT_MOUNT_ALL_MSG 'mall'
	#define MOUNT_MOUNT_SELECTED_MSG 'msel'
	#define UNMOUNT_UNMOUNT_SELECTED_MSG 'usel'
	#define SETUP_FORMAT_MSG 'sfor'
	#define SETUP_PARTITION_APPLE_MSG 'spaa'
	#define SETUP_PARTITION_INTEL_MSG 'spai'
	#define SETUP_INITIALIZE_MSG 'sini'
	#define OPTIONS_EJECT_MSG 'oeje'
	#define OPTIONS_SURFACE_TEST_MSG 'osut'
	#define RESCAN_IDE_MSG 'ride'
	#define RESCAN_SCSI_MSG 'rscs'
	
	#ifndef _APPLICATION_H

		#include <Application.h>
	
	#endif
	#ifndef _WINDOW_H
	
		#include <Window.h>
		
	#endif
	#ifndef _MENU_BAR_H
	
		#include <MenuBar.h>
		
	#endif
	#ifndef _MENU_ITEM_H
	
		#include <MenuItem.h>
		
	#endif
	#ifndef _RECT_H
	
		#include <Rect.h>
		
	#endif
	#ifndef _STDIO_H
	
		#include <stdio.h>
		
	#endif
	#ifndef POS_SETTINGS_H
	
		#include "PosSettings.h"

	#endif
	#ifndef STRUCTS_H
	
		#include "structs.h"
		
	#endif
	
	/**
	 * The main window of the app.
	 *
	 * Sets up and displays everything you need for the app.
	 */
	class MainWindow : public BWindow{
	
		private:
		
			PosSettings	*fSettings;
			BMenuBar *rootMenu;
			
		public:
		
			MainWindow(BRect frame, PosSettings *fSettings);
			virtual bool QuitRequested();
			virtual void MessageReceived(BMessage *message);
			virtual void FrameMoved(BPoint origin);
			void AddDeviceInfo(dev_info d);
						
	};
	
#endif
