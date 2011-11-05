/**App window for the FileSoundWindow test
	@author Tim de Jong
	@date 21/09/2002
	@version 1.0
 */


#include "FileSoundWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <Path.h>
#include <TextControl.h>

#include <stdio.h>
#include <stdlib.h>


FileSoundWindow::FileSoundWindow(BRect windowBounds)
					:BWindow(windowBounds,"BFileGameSound test",B_TITLED_WINDOW,B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	//init private vars
	loop = false;
	preload = false;
	playing = false;
	paused = false;
	rampTime = 250000; // 250 ms default
	fileSound = 0;
	//make openPanel and let it send its messages to this window
	openPanel = new BFilePanel();
	openPanel -> SetTarget(this);
	//get appBounds
	BRect appBounds = Bounds();
	//make a cosmetic box
	BBox *box = new BBox(appBounds);
	//textcontrol to display the chosen file
	BRect textBounds(appBounds.left + 10, appBounds.top + 10, appBounds.right - 70, appBounds.top + 20);
	textControl = new BTextControl(textBounds,"filechosen","Play file:","No file chosen yet.", NULL);
	textControl -> SetEnabled(false);
	
	float x1 = textControl -> Bounds().left;
	float x2 = textControl -> Bounds().right;
	float dividerX = 0.20 * (x2 - x1);
	textControl -> SetDivider(dividerX);
	
	box -> AddChild(textControl);
	//button to choose file
	BRect browseBounds(textBounds.right + 5,textBounds.top, appBounds.right - 5, textBounds.bottom);
	BMessage *browseMessage = new BMessage(BROWSE_MSG);
	BButton *browseButton = new BButton(browseBounds,"browseButton","Browse" B_UTF8_ELLIPSIS,browseMessage);
	box -> AddChild(browseButton);
	//make play button
	BRect playBounds(textBounds.left,textBounds.bottom + 15, textBounds.left + 50, textBounds.bottom + 25);
	BMessage *playMessage = new BMessage(PLAY_MSG);
	playButton = new BButton(playBounds,"playbutton","Play", playMessage);
	box -> AddChild(playButton);
	//make pause button
	BRect pauseBounds(playBounds.right + 10,playBounds.top, playBounds.right + 60, playBounds.bottom);
	BMessage *pauseMessage = new BMessage(PAUSE_MSG);
	pauseButton = new BButton(pauseBounds,"pausebutton","Pause",pauseMessage);
	box -> AddChild(pauseButton);
	//make textcontrol to enter delay for pausing/resuming
	BRect delayBounds(pauseBounds.right + 10, pauseBounds.top,pauseBounds.right + 150, pauseBounds.bottom);
	delayControl = new BTextControl(delayBounds,"delay","Ramp time (ms)","250", new BMessage(DELAY_MSG));
	delayControl -> SetDivider(90);
	delayControl -> SetModificationMessage(new BMessage(DELAY_MSG));
	box -> AddChild(delayControl);
	//make loop checkbox
	BRect loopBounds(playBounds.left,playBounds.bottom + 20, playBounds.left + 150, playBounds.bottom + 30);
	BMessage *loopMessage = new BMessage(LOOP_MSG);
	loopCheck = new BCheckBox(loopBounds,"loopcheckbox","Loop the sound file",loopMessage);
	box -> AddChild(loopCheck);
	//make preload checkbox
	BRect preloadBounds(loopBounds.right + 10,loopBounds.top, loopBounds.right + 150, loopBounds.bottom);
	BMessage *preloadMessage = new BMessage(PRELOAD_MSG);
	preloadCheck = new BCheckBox(preloadBounds,"loopcheckbox","Preload the sound file",preloadMessage);
	box -> AddChild(preloadCheck); 	
	//add background box to window
	AddChild(box);
}

FileSoundWindow::~FileSoundWindow()
{
}

void FileSoundWindow::MessageReceived(BMessage *message)
{
	switch (message -> what)
	{
		case BROWSE_MSG:
			openPanel -> Show();
		break;
		
		case B_REFS_RECEIVED:
		{
			message->FindRef("refs", 0, &fileref);
			BPath path(new BEntry(&fileref));
			textControl -> SetText(path.Path());
		}	
		break;
		
		case PLAY_MSG:
		{
			if (!playing)
			{
				fileSound = new BFileGameSound(&fileref,loop);
				status_t error = fileSound -> InitCheck();
				if (error != B_OK)
				{
					delete fileSound;
					fileSound = 0;
					if (error == B_NO_MEMORY)
					{
						BAlert *alert = new BAlert("alert","Not enough memory.","OK");
						alert -> Go();
					}
					else if (error == B_ERROR)
					{
						BAlert *alert = new BAlert("alert","Unable to create a sound player.","OK");
						alert -> Go();
					}
					else
					{
						BAlert *alert = new BAlert("alert","Other kind of error!","OK");
						alert -> Go();
					}				
					break;
				}
				paused = false;
				pauseButton -> SetLabel("Pause");
				//preload sound file?
				if (preload)
				{
					status_t preloadError = fileSound -> Preload();
					if (preloadError != B_OK)
					{
						BAlert *alert = new BAlert("alert","Port errors. Unable to communicate with the streaming sound port.","OK");
						alert -> Go();
					}
				}	
				//play it!
				status_t playerror = fileSound -> StartPlaying();
				if (playerror != B_OK)
				{
					if (playerror == EALREADY)
					{
						BAlert *alert = new BAlert("alert","Sound is already playing","OK");
						alert -> Go();
					}
					else
					{
						BAlert *alert = new BAlert("alert","Error playing sound","OK");
						alert -> Go();
					}
				}
				else 
				{
					playButton -> SetLabel("Stop");
					playing = true;
				}
			}
			else
			{
				//stop it!
				status_t stoperror = fileSound -> StopPlaying();
				if (stoperror != B_OK)
				{
					if (stoperror == EALREADY)
					{
						BAlert *alert = new BAlert("alert","Sound is already stopped","OK");
						alert -> Go();
					}
					else
					{
						BAlert *alert = new BAlert("alert","Error stopping sound","OK");
						alert -> Go();
					}
				}
				else
				{
					playButton -> SetLabel("Play");
					playing = false;
				}
				delete fileSound;
				fileSound = 0;
			}
		}
		break;
		
		case PAUSE_MSG:
		{
			if (!fileSound)
				break;

			int32 pausedValue = fileSound -> IsPaused();
			if (pausedValue != BFileGameSound::B_PAUSE_IN_PROGRESS)
			{
				status_t error;
				const char *label;
				if (paused)
				{
					paused = false;
					label = "Pause";
					error = fileSound ->SetPaused(paused, rampTime);
				}
				else
				{
					paused = true;
					label = "Resume";
					error = fileSound ->SetPaused(paused, rampTime);
				}
				
				if (error != B_OK)
				{
					if (error == EALREADY)
					{
						BAlert *alert = new BAlert("alert","Already in requested state","OK");
						alert -> Go();
					}
					else
					{
						BAlert *alert = new BAlert("alert","Error!","OK");
						alert -> Go();
					}
				} 
				else
				{
					pauseButton -> SetLabel(label);
				}
			}
		}
		break;
		
		case LOOP_MSG:
		{
			int32 checkValue = loopCheck -> Value();
			if (checkValue == B_CONTROL_ON)
				loop = true;
			else loop = false;		
		}
		break;
		
		case PRELOAD_MSG:
		{
			int32 checkValue = preloadCheck -> Value();
			if (checkValue == B_CONTROL_ON)
				preload = true;
			else preload = false;
		}
		break;
		
		case DELAY_MSG:
		{
			//set delay time for resuming/pausing
			rampTime = atol(delayControl -> Text()) * 1000;
		}
		break;
	}

}

bool FileSoundWindow::QuitRequested() 
{
	
	delete openPanel;
	if (fileSound != 0 && fileSound -> IsPlaying())
		fileSound -> StopPlaying();
	delete fileSound;
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
