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

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Slider.h>
#include <stdio.h>
#include <StringView.h>
#include <Window.h>

#include "VMSettings.h"

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
		int forigMemSize;
		
		/**
		 * Saves the minimum virtual memory value;
		 */
		int fminSwapVal;
		
		/**
		 * The slider that lets you adjust the size of the swap file.
		 */
		BSlider *freqSizeSlider;
		
		/**
		 * The button that returns the swap file to the original 
		 * size.
		 */
		BButton *frevertButton;
		
		/**
		 * The BStringView that informs you that you need to
		 * restart.
		 */
		BStringView *frestart;
		
		VMSettings	*fSettings;
		
	public:
		MainWindow(BRect frame, int physMem, int currSwp, int sliderMin, int sliderMax, VMSettings *fSettings);
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *message);
		virtual void FrameMoved(BPoint origin);
		virtual void toggleChangedMessage(bool setTo);
					
};

#endif
