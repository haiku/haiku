/*! \file MainWindow.cpp
 *  \brief Code for the MainWindow class.
 *  
 *  Displays the main window, the essence of the app.
 *
*/

#include "MainWindow.h"
#include "Pref_Utils.h"

#include <String.h>

const char *kRequestStr = "Requested swap file size: ";
/**
 * Constructor.
 * @param frame The size to make the window.
 * @param physMem The amount of physical memory in the machine.
 * @param currSwp The current swap file size.
 * @param minVal The minimum value of the swap file.
 * @param maxSwapVal The maximum value of the swap file.
 */	
MainWindow::MainWindow(BRect frame, int physMemVal, int currSwapVal, int minVal, int maxSwapVal, VMSettings *Settings)
	:BWindow(frame, "VirtualMemory", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE){

	fSettings = Settings;
	
	/**
	 * Sets the fill color for the "used" portion of the slider.
	 */
	rgb_color fillColor = { 0, 102, 152, 255 };

	/**
	 * Set up variables to help handle font sensitivity
	 */
	float fontheight = FontHeight(true, NULL);
	
	/**
	 * boxRect sets the size of the visible box around the string views and
	 * the slider.
	 */
	BRect boxRect = Bounds();
	boxRect.InsetBy(11, 11);
	boxRect.bottom -= 25;
	
	BString labels;
	forigMemSize = currSwapVal;
	fminSwapVal = minVal;
	
	BRect rect(0.0, 0.0, boxRect.Width() -20.0, fontheight);
	rect.OffsetTo(10.0, 10.0);
	/**
	 * Set up the "Physical Memory" label.
	 */
	BStringView *physMem;
	 labels << "Physical memory: " << physMemVal << " MB";
	physMem = new BStringView(rect, "PhysicalMemory", labels.String(), B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * Set up the "Current Swap File Size" label.
	 */
	BStringView *currSwap;
	rect.OffsetBy(0, rect.Height() +5);
	labels = "Current swap file size: ";
	labels << currSwapVal << " MB";
	currSwap = new BStringView(rect, "CurrentSwapSize", labels.String(), B_FOLLOW_ALL, B_WILL_DRAW);
	
	/**
	 * Set up the "Requested Swap File Size" label.
	 */
	rect.OffsetBy(0, rect.Height() +5);
	labels = kRequestStr;
	labels << currSwapVal << " MB";
	/**
	 * Set up the slider.
	 */
	BString sliderMinLabel;
	BString sliderMaxLabel;
		sliderMinLabel << fminSwapVal << " MB";
		sliderMaxLabel << maxSwapVal << " MB";

	rect.bottom = rect.top+2;
	freqSizeSlider = new BSlider(rect, "ReqSwapSizeSlider", labels.String(), 
		new BMessage(MEMORY_SLIDER_MSG), fminSwapVal, maxSwapVal, B_TRIANGLE_THUMB, B_FOLLOW_LEFT, B_WILL_DRAW);

	freqSizeSlider->SetLimitLabels(sliderMinLabel.String(), sliderMaxLabel.String());
	freqSizeSlider->UseFillColor(true, &fillColor);
	freqSizeSlider->SetModificationMessage(new BMessage(SLIDER_UPDATE_MSG));
	
	freqSizeSlider->SetValue(currSwapVal);
	
	/**
	 * Initializes the restart notice view.
	 */
	 rect = freqSizeSlider->Frame();
	rect.top = rect.bottom +2;
	rect.bottom = rect.top +fontheight;
	frestart = new BStringView(rect, "RestartMessage", B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW);
	frestart->SetAlignment(B_ALIGN_CENTER);
	
	/**
	 * This view holds the three labels and the slider.
	 */

	BBox *boxView;
	boxView = new BBox(boxRect, "BoxView", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	boxView->AddChild(freqSizeSlider);
	boxView->AddChild(physMem);
	boxView->AddChild(currSwap);
	boxView->AddChild(frestart);
	
	rect.Set(0.0, 0.0, 75.0, 20.0);
	BButton *defaultButton;
	rect.OffsetTo(10, boxRect.bottom +5);
	defaultButton = new BButton(rect, "DefaultButton", "Default", 
		new BMessage(DEFAULT_BUTTON_MSG), B_FOLLOW_ALL, B_WILL_DRAW);
	rect.OffsetBy(85, 0);
	frevertButton = new BButton(rect, "RevertButton", "Revert", 
		new BMessage(REVERT_BUTTON_MSG), B_FOLLOW_ALL, B_WILL_DRAW);
	frevertButton->SetEnabled(false);
	
	BBox *topLevelView;
	topLevelView = new BBox(Bounds(), "TopLevelView", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	topLevelView->AddChild(boxView);
	topLevelView->AddChild(defaultButton);
	topLevelView->AddChild(frevertButton);
	
	AddChild(topLevelView);
}

/**
 * Displays the "Changes will take effect on restart" message.
 * @param setTo If true, displays the message  If false, un-displays it.
 */	
void MainWindow::toggleChangedMessage(bool setTo){
	
	BString message = B_EMPTY_STRING;
	if (setTo) {
		frevertButton->SetEnabled(true);
		message << "Changes will take effect on restart.";
	} else {
		frevertButton->SetEnabled(false);
	}

	frestart->SetText(message.String());
}//toggleChangedMessage

/**
 * Handles messages.
 * @param message The message recieved by the window.
 */	
void MainWindow::MessageReceived(BMessage *message){

	switch(message->what){

		/**
		 * Updates the requested swap file size during a drag.
		 */
		case SLIDER_UPDATE_MSG:
		{
			int32 currVal = freqSizeSlider->Value();
			BString label(kRequestStr);
			label << currVal << " MB";
			freqSizeSlider->SetLabel(label.String());
			
			if (currVal != forigMemSize) 
				toggleChangedMessage(true);
			else 
				toggleChangedMessage(false);

			break;
		}	
		
		/**
		 * Case where the slider was moved.
		 * Resets the "Requested Swap File Size" label to the new value.
		 */
		case MEMORY_SLIDER_MSG:
		{
			int32 currVal = freqSizeSlider->Value();
			BString label(kRequestStr);
			label << currVal << " MB";
			freqSizeSlider->SetLabel(label.String());
			
			if (currVal != forigMemSize)
				toggleChangedMessage(true);
			else
				toggleChangedMessage(false);
			
			break;
		}	
			
		/**
		 * Case where the default button was pressed.
		 * Eventually will set the swap file size to the optimum size, 
		 * as decided by this app (as soon as I can figure out how to 
		 * do that).
		 */
		case DEFAULT_BUTTON_MSG:
		{
			freqSizeSlider->SetValue(fminSwapVal);
			BString label(kRequestStr);
			label << fminSwapVal << " MB";
			freqSizeSlider->SetLabel(label.String());

			if(fminSwapVal != forigMemSize)
				toggleChangedMessage(true);				
			else
				toggleChangedMessage(false);
				
			break;
		}	
		/**
		 * Case where the revert button was pressed.
		 * Returns things to the way they were when the app was started, 
		 * which is not necessarily the default size.
		 */
		case REVERT_BUTTON_MSG:
		{
			frevertButton->SetEnabled(false);
			BString label(kRequestStr);
			label << forigMemSize << " MB";
			freqSizeSlider->SetLabel(label.String());
			freqSizeSlider->SetValue(forigMemSize);
			toggleChangedMessage(false);
			
			break;
		}	
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

	if (freqSizeSlider->Value() != forigMemSize) {
		FILE *settingsFile = fopen("/boot/home/config/settings/kernel/drivers/virtual_memory", "w");
		fprintf(settingsFile, "vm on\n");
		fprintf(settingsFile, "swap_size %d\n", (int(freqSizeSlider->Value()) * 1048576));
		fclose(settingsFile);
	}
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
	
}

void MainWindow::FrameMoved(BPoint origin)
{
	fSettings->SetWindowPosition(Frame());
}

