/*! \file MainWindow.h
    \brief Header for the MainWindow class.
    
*/

#ifndef MAIN_WINDOW_H

	
	#define MAIN_WINDOW_H
	
	/*!
	 *  Default button message.
	 */
	#define DEFAULT_BUTTON_MSG 'dflt'
	
	/*!
	 *  Revert button message.
	 */
	#define REVERT_BUTTON_MSG 'rvrt'
	
	/*!
	 *  Slider message.
	 */
	#define MEMORY_SLIDER_MSG 'sldr'
	
	/*!
	 *	Slider dragging message.
	 */
	#define SLIDER_UPDATE_MSG 'sldu'
	
	#ifndef _APPLICATION_H

		#include <Application.h>
	
	#endif
	#ifndef _WINDOW_H
	
		#include <Window.h>
		
	#endif
	#ifndef _STRING_VIEW_H
	
		#include <StringView.h>
		
	#endif
	#ifndef _BOX_H
	
		#include <Box.h>
		
	#endif
	#ifndef _SLIDER_H
	
		#include <Slider.h>
		
	#endif
	#ifndef _BUTTON_H
	
		#include <Button.h>
		
	#endif
	#ifndef _STDIO_H
	
		#include <stdio.h>
		
	#endif
	#ifndef VM_SETTINGS_H
	
		#include "VMSettings.h"

	#endif
	
	/**
	 * The main window of the app.
	 *
	 * Sets up and displays everything you need for the app.
	 */
	class MainWindow : public BWindow{
	
		private:
		
			/**
			 * Saves the size of the swap file when the app is started
			 * so that it can be restored later if need be.
			 */
			int origMemSize;
			
			/**
			 * Saves the minimum virtual memory value;
			 */
			int minSwapVal;
			
			/**
			 * The BStringView that shows the requested swap file 
			 * size.
			 */
			BStringView *reqSwap;
			
			/**
			 * The slider that lets you adjust the size of the swap file.
			 */
			BSlider *reqSizeSlider;
			
			/**
			 * The button that returns the swap file to the original 
			 * size.
			 */
			BButton *revertButton;
			
			/**
			 * The BStringView that informs you that you need to
			 * restart.
			 */
			BStringView *restart;
			
			VMSettings	*fSettings;
			
		public:
		
			MainWindow(BRect frame, int physMem, int currSwp, int sliderMin, int sliderMax, VMSettings *fSettings);
			virtual bool QuitRequested();
			virtual void MessageReceived(BMessage *message);
			virtual void FrameMoved(BPoint origin);
			virtual void toggleChangedMessage(bool setTo);
						
	};
	
#endif
