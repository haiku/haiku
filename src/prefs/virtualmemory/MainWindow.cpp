/*! \file MainWindow.cpp
 *  \brief Code for the MainWindow class.
 *  
 *  Displays the main window, the essence of the app.
 *
*/

#include "MainWindow.h"

/**
 * Constructor.
 * @param frame The size to make the window.
 * @param physMem The amount of physical memory in the machine.
 * @param currSwp The current swap file size.
 * @param minVal The minimum value of the swap file.
 * @param maxSwapVal The maximum value of the swap file.
 */	
MainWindow::MainWindow(BRect frame, int physMemVal, int currSwapVal, int minVal, int maxSwapVal, VMSettings *Settings)
			:BWindow(frame, "Virtual Memory", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE){

	BStringView *physMem;
	BStringView *currSwap;
	BButton *defaultButton;
	BBox *topLevelView;
	BBox *boxView;
	
	fSettings = Settings;
	
	/**
	 * Sets the fill color for the "used" portion of the slider.
	 */
	rgb_color fillColor;
	fillColor.red = 0;
	fillColor.blue = 152;
	fillColor.green = 102;
	
	/**
	 * This var sets the size of the visible box around the string views and
	 * the slider.
	 */
	BRect boxRect=Bounds();
	boxRect.left=boxRect.left+10;
	boxRect.top=boxRect.top+10;
	boxRect.bottom=boxRect.bottom-45;
	boxRect.right=boxRect.right-10;
	char labels[50];
	char sliderMinLabel[10];
	char sliderMaxLabel[10];
	
	origMemSize = currSwapVal;
	minSwapVal = minVal;
	
	/**
	 * Set up the "Physical Memory" label.
	 */
	sprintf(labels, "Physical Memory: %d MB", physMemVal);
	physMem = new BStringView(*(new BRect(10, 10, 210, 20)), "PhysicalMemory", labels, B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * Set up the "Current Swap File Size" label.
	 */
	sprintf(labels, "Current Swap File Size: %d MB", currSwapVal);
	currSwap = new BStringView(*(new BRect(10, 25, 210, 35)), "CurrentSwapSize", labels, B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * Set up the "Requested Swap File Size" label.
	 */
	sprintf(labels, "Requested Swap File Size: %d MB", currSwapVal);
	reqSwap = new BStringView(*(new BRect(10, 40, 210, 50)), "RequestedSwapSize", labels, B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * Set up the slider.
	 */
	sprintf(sliderMinLabel, "%d MB", minSwapVal);
	sprintf(sliderMaxLabel, "%d MB", maxSwapVal);
	reqSizeSlider = new BSlider(*(new BRect(10, 51, 240, 75)), "ReqSwapSizeSlider", "", new BMessage(MEMORY_SLIDER_MSG), minSwapVal, maxSwapVal, B_TRIANGLE_THUMB, B_FOLLOW_LEFT, B_WILL_DRAW);
	reqSizeSlider->SetLimitLabels(sliderMinLabel, sliderMaxLabel);
	reqSizeSlider->UseFillColor(TRUE, &fillColor);
	reqSizeSlider->SetModificationMessage(new BMessage(SLIDER_UPDATE_MSG));
	
	reqSizeSlider->SetValue(currSwapVal);
	
	/**
	 * Initializes the restart notice view.
	 */
	restart = new BStringView(*(new BRect(40, 100, 210, 110)), "RestartMessage", "", B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * This view holds the three labels and the slider.
	 */
	boxView = new BBox(boxRect, "BoxView", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	boxView->AddChild(reqSizeSlider);
	boxView->AddChild(physMem);
	boxView->AddChild(currSwap);
	boxView->AddChild(reqSwap);
	boxView->AddChild(restart);
		
	defaultButton = new BButton(*(new BRect(10, 138, 85, 158)), "DefaultButton", "Default", new BMessage(DEFAULT_BUTTON_MSG), B_FOLLOW_ALL, B_WILL_DRAW);
	revertButton = new BButton(*(new BRect(95, 138, 170, 158)), "RevertButton", "Revert", new BMessage(REVERT_BUTTON_MSG), B_FOLLOW_ALL, B_WILL_DRAW);
	revertButton->SetEnabled(false);
	
	topLevelView = new BBox(Bounds(), "TopLevelView", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER);
	topLevelView->AddChild(boxView);
	topLevelView->AddChild(defaultButton);
	topLevelView->AddChild(revertButton);
	
	AddChild(topLevelView);
	
}

/**
 * Displays the "Changes will take effect on restart" message.
 * @param setTo If true, displays the message  If false, un-displays it.
 */	
void MainWindow::toggleChangedMessage(bool setTo){

	char msg[100];
	if(setTo){
	
		revertButton->SetEnabled(true);
		sprintf(msg, "Changes will take effect on restart.");
		restart->SetText(msg);
	
	}//if
	else{
	
		restart->SetText("");
		
	}//else

}//toggleChangedMessage

/**
 * Handles messages.
 * @param message The message recieved by the window.
 */	
void MainWindow::MessageReceived(BMessage *message){

	char msg[100];
	
	switch(message->what){
	
		int currVal;
		
		/**
		 * Updates the requested swap file size during a drag.
		 */
		case SLIDER_UPDATE_MSG:
			
			currVal = int(reqSizeSlider->Value());
			sprintf(msg, "Requested Swap File Size: %d MB", currVal);
			reqSwap->SetText(msg);
			
			if(currVal != origMemSize){
			
				toggleChangedMessage(true);
				
			}//if
			else{
				
				revertButton->SetEnabled(false);
				toggleChangedMessage(false);
				
			}//else
			
			break;
		
		/**
		 * Case where the slider was moved.
		 * Resets the "Requested Swap File Size" label to the new value.
		 */
		case MEMORY_SLIDER_MSG:
		
			currVal = int(reqSizeSlider->Value());
			sprintf(msg, "Requested Swap File Size: %d MB", currVal);
			reqSwap->SetText(msg);
			
			if(currVal != origMemSize){
			
				toggleChangedMessage(true);
				
			}//if
			else{
				
				revertButton->SetEnabled(false);
				toggleChangedMessage(false);
				
			}//else
			
			break;
			
		/**
		 * Case where the default button was pressed.
		 * Eventually will set the swap file size to the optimum size, 
		 * as decided by this app (as soon as I can figure out how to 
		 * do that).
		 */
		case DEFAULT_BUTTON_MSG:
		
			reqSizeSlider->SetValue(minSwapVal);
			sprintf(msg, "Requested Swap File Size: %d MB", minSwapVal);
			reqSwap->SetText(msg);
			if(minSwapVal != origMemSize){
			
				toggleChangedMessage(true);
				
			}//if
			else{
				
				revertButton->SetEnabled(false);
				toggleChangedMessage(false);
				
			}//else
			break;
			
		/**
		 * Case where the revert button was pressed.
		 * Returns things to the way they were when the app was started, 
		 * which is not necessarily the default size.
		 */
		case REVERT_BUTTON_MSG:
		
			revertButton->SetEnabled(false);
			sprintf(msg, "Requested Swap File Size: %d MB", origMemSize);
			reqSwap->SetText(msg);
			reqSizeSlider->SetValue(origMemSize);
			toggleChangedMessage(false);
			break;
			
		/**
		 * Unhandled messages get passed to BWindow.
		 */
		default:
		
			BWindow::MessageReceived(message);
	
	}
	
}

/**
 * Quits and Saves.
 * Sets the swap size and turns the virtual memory on by writing to the
 * /boot/home/config/settings/kernel/drivers/virtual_memory file.
 */	
bool MainWindow::QuitRequested(){

    FILE *settingsFile =
    	fopen("/boot/home/config/settings/kernel/drivers/virtual_memory", "w");
    fprintf(settingsFile, "vm on\n");
    fprintf(settingsFile, "swap_size %d\n", (int(reqSizeSlider->Value()) * 1048576));
    fclose(settingsFile);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
	
}

void MainWindow::FrameMoved(BPoint origin)
{//MainWindow::FrameMoved
	fSettings->SetWindowPosition(Frame());
}//MainWindow::FrameMoved

