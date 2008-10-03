/*
Author:	Jerome LEVEQUE
Email:	jerl1@caramail.com
*/
#include "MidiPlayerWindow.h"
#include "MidiDelay.h"
#include "Activity.h"

#include <Application.h>
#include <Roster.h>
#include <Resources.h>
#include <Alert.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <StringView.h>
#include <String.h>
#include <File.h>
#include <MidiStore.h>
#include <MidiPort.h>
#include <MidiSynth.h>
#include <MidiSynthFile.h>

//----------------------------------------------------------

class MidiPlayerApp : public BApplication
{
public:
	MidiPlayerApp(void)
		: BApplication("application/x-vnd.MidiPlayer")
	{
	app_info info;
	BFile file;
	BResources resource;
	size_t len;
		GetAppInfo(&info);
		if (file.SetTo(&(info.ref), B_READ_ONLY) != B_OK) return;
		resource.SetTo(&file);
		BPoint *origin((BPoint*)resource.LoadResource('BPNT', "origin", &len));
		if (origin == NULL)
			fWindows = new MidiPlayerWindow(BPoint(100.0, 100.0));
		else
			fWindows = new MidiPlayerWindow(*origin);
	}

//--------------

	void ReadyToRun(void)
	{
		fWindows->Show();
	}

//--------------

	void SetOrigin(BPoint origin)
	{
	app_info info;
	BFile file;
	BResources resource;
	int32 temp;
		GetAppInfo(&info);
		if (file.SetTo(&(info.ref), B_READ_WRITE) != B_OK) return;
		resource.SetTo(&file);
		temp = resource.RemoveResource('BPNT', 1);
		temp = resource.AddResource('BPNT', 1, &origin, sizeof(origin), "origin");
	}

//--------------

	virtual void RefsReceived(BMessage *message)
	{
		message->what = B_SIMPLE_DATA;
		fWindows->PostMessage(message);
	}

//--------------
//--------------
//--------------
//--------------
//--------------

private:
	MidiPlayerWindow *fWindows;

//--------------

};

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

MidiPlayerWindow::MidiPlayerWindow(BPoint start)
				:	BWindow(BRect(start, start + BPoint(660, 360)), "Midi Player",
							B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
					fMidiInput(NULL),
					fMidiOutput(NULL),
					fMidiDisplay(NULL),
					fInputType(0),
					fOutputType(0),
					fDisplayType(0),
					fInputFilePanel(NULL),
					fOutputFilePanel(NULL),
					fInputCurrentEvent(0),
					fOutputCurrentEvent(0)
{
	AddChild(fStandartView = new MidiPlayerView());
	fMidiDelay = new MidiDelay();
}

//----------------------------------------------------------

MidiPlayerWindow::~MidiPlayerWindow(void)
{
	Disconnect();
	delete fMidiInput;
	delete fMidiOutput;
}

//----------------------------------------------------------

bool MidiPlayerWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//----------------------------------------------------------

void MidiPlayerWindow::FrameMoved(BPoint origin)
{
	((MidiPlayerApp*)be_app)->SetOrigin(origin);
}

//----------------------------------------------------------

void MidiPlayerWindow::MessageReceived(BMessage *msg)
{
uint32 temp = 0;
BPopUpMenu *MidiPortMenu = NULL;
BMenuItem *item = NULL;
BMessage message;
entry_ref ref;
	switch (msg->what)
	{
//--------------
//For The Input
//--------------
//--------------
		case INPUT_CHANGE_TO_FILE :
				if (fInputType == FILE)
					break;
				fInputType = FILE;
				Disconnect();
				delete fMidiInput;
				fMidiInput = new BMidiStore();
				if (!fMidiInput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiStore object", "OK"))->Go();
					break;
				}
				Connect();
				PostMessage(msg, fStandartView);
				break;
//--------------
		case INPUT_CHANGE_TO_MIDIPORT :
				if (fInputType == MIDIPORT)
					break;
				fInputType = MIDIPORT;
				Disconnect();
				delete fMidiInput;
				fMidiInput = new BMidiPort();
				if (!fMidiInput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiPort object", "OK"))->Go();
					break;
				}
				MidiPortMenu = new BPopUpMenu("Midi Port");
				for(int i = ((BMidiPort*)fMidiInput)->CountDevices(); i > 0; i--)
				{
				char name[B_OS_NAME_LENGTH];
					((BMidiPort*)fMidiInput)->GetDeviceName(i - 1, name);
					MidiPortMenu->AddItem(new BMenuItem(name, new BMessage(CHANGE_INPUT_MIDIPORT)));
				}
				msg->AddPointer("Menu", MidiPortMenu);
				Connect();
				PostMessage(msg, fStandartView);
				break;
//--------------
//For the Output
//--------------
//--------------
		case OUTPUT_CHANGE_TO_FILE :
				if (fOutputType == FILE)
					break;
				fOutputType = FILE;
				Disconnect();
				delete fMidiOutput;
				fMidiOutput = new BMidiStore();
				if (!fMidiOutput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiStore object", "OK"))->Go();
					break;
				}
				Connect();
				PostMessage(msg, fStandartView);
				break;
//--------------
		case OUTPUT_CHANGE_TO_BEOS_SYNTH :
				if (fOutputType == BEOS_SYNTH)
					break;
				fOutputType = BEOS_SYNTH;
				Disconnect();
				delete fMidiOutput;
				fMidiOutput = new BMidiSynth();
				if (!fMidiOutput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiSynth object", "OK"))->Go();
					break;
				}
				Connect();
				((BMidiSynth*)fMidiOutput)->EnableInput(true, true);
				PostMessage(msg, fStandartView);
				break;
//--------------
		case OUTPUT_CHANGE_TO_BEOS_SYNTH_FILE :
				if (fOutputType == BEOS_SYNTH_FILE)
					break;
				fOutputType = BEOS_SYNTH_FILE;
				Disconnect();
				delete fMidiOutput;
				fMidiOutput = new BMidiSynthFile();
				if (!fMidiOutput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiSynth object", "OK"))->Go();
					break;
				}
				Connect();
				((BMidiSynthFile*)fMidiOutput)->EnableInput(true, true);
				PostMessage(msg, fStandartView);
				break;
//--------------
		case OUTPUT_CHANGE_TO_MIDIPORT :
				if (fOutputType == MIDIPORT)
					break;
				fOutputType = MIDIPORT;
				Disconnect();
				delete fMidiOutput;
				fMidiOutput = new BMidiPort();
				if (!fMidiOutput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiPort object", "OK"))->Go();
					break;
				}
				MidiPortMenu = new BPopUpMenu("Midi Port");
				for(int i = ((BMidiPort*)fMidiOutput)->CountDevices(); i > 0; i--)
				{
				char name[B_OS_NAME_LENGTH];
					((BMidiPort*)fMidiOutput)->GetDeviceName(i - 1, name);
					MidiPortMenu->AddItem(new BMenuItem(name, new BMessage(CHANGE_OUTPUT_MIDIPORT)));
				}
				msg->AddPointer("Menu", MidiPortMenu);
				Connect();
				PostMessage(msg, fStandartView);
				break;
//--------------
//For the display view
//--------------
//--------------
		case VIEW_CHANGE_TO_NONE :
				if (fDisplayType == 0)
					break;
				fDisplayType = 0;
				if (fMidiDisplay != NULL)
					fMidiDelay->Disconnect(fMidiDisplay); //Object will be deleted in MidiPlayerView::RemoveAll
				PostMessage(msg, fStandartView);
				break;
//--------------
		case VIEW_CHANGE_TO_SCOPE :
				if (fDisplayType == SCOPE)
					break;
				fDisplayType = SCOPE;
				PostMessage(msg, fStandartView);
				break;
//--------------
		case VIEW_CHANGE_TO_ACTIVITY :
				if (fDisplayType == ACTIVITY)
					break;
				fDisplayType = ACTIVITY;
				fMidiDisplay = (BMidi*)new Activity(BRect(10, 25, 340, 340));
				fMidiDelay->Connect(fMidiDisplay);
				((Activity*)fMidiDisplay)->Start();
				msg->AddPointer("View", fMidiDisplay);
				PostMessage(msg, fStandartView);
				break;
//--------------
//--------------
//Message from Input file
//--------------
		case CHANGE_INPUT_FILE :
				if (fInputFilePanel)
				{
				entry_ref File;
					if (msg->FindRef("refs", &File) != B_OK)
						break;
					Disconnect();
					delete fMidiInput;
					fMidiInput = new BMidiStore();
					if (!fMidiInput)
					{
						(new BAlert(NULL, "Can't initialise a BMidiPort object", "OK"))->Go();
						return;
					}
					Connect();
					if (((BMidiStore*)fMidiInput)->Import(&File) == B_OK)
					{
					BStringView *aStringView = fStandartView->GetInputStringView();
						aStringView->SetText(File.name);
					}
					delete fInputFilePanel;
					fInputFilePanel = NULL;
				}
				else
				{
					BMessenger messenger(this)
					fInputFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger,
						NULL, B_FILE_NODE, false, msg);
					fInputFilePanel->Show();
				}
				break;
//--------------
		case REWIND_INPUT_FILE :
				((BMidiStore*)fMidiInput)->Stop();
				if (fMidiOutput) fMidiOutput->AllNotesOff();
				fInputCurrentEvent = 0;
				return;
//--------------
		case PLAY_INPUT_FILE :
				((BMidiStore*)fMidiInput)->SetCurrentEvent(fInputCurrentEvent);
				((BMidiStore*)fMidiInput)->Start();
				break;
//--------------
		case PAUSE_INPUT_FILE :
				fInputCurrentEvent = ((BMidiStore*)fMidiInput)->CurrentEvent();
				((BMidiStore*)fMidiInput)->Stop();
				if (fMidiOutput) fMidiOutput->AllNotesOff();
				break;
//--------------
//--------------
//Message from Output file
//--------------
		case CHANGE_OUTPUT_FILE :
				if (fOutputFilePanel)
				{
					const char *filename;
					msg->FindRef("directory", &fOutputFile);
					msg->FindString("name", &filename);
					BDirectory path(&fOutputFile);
					BFile fichier(&path, filename, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
					if (fichier.InitCheck() == B_OK)
					{
					BStringView *aStringView = fStandartView->GetOutputStringView();
						BEntry entry(&path, filename);
						entry.GetRef(&fOutputFile);
						aStringView->SetText(fOutputFile.name);
					}
					else
					{
						(new BAlert(NULL, "Can't Create File", "OK"))->Go();
					}
					delete fOutputFilePanel;
					fOutputFilePanel = NULL;
				}
				else
				{
					BMessenger messenger(this)
					fOutputFilePanel = new BFilePanel(B_SAVE_PANEL, &messenger,
						NULL, B_FILE_NODE, false, msg);
					fOutputFilePanel->Show();
				}
				break;
//--------------
		case REWIND_OUTPUT_FILE :
				Disconnect();
				delete fMidiOutput;
				fMidiOutput = new BMidiPort();
				if (!fMidiOutput)
				{
					(new BAlert(NULL, "Can't initialise a BMidiPort object", "OK"))->Go();
					return;
				}
				Connect();
				fOutputCurrentEvent = 0;
				return;
//--------------
		case SAVE_OUTPUT_FILE :
				((BMidiStore*)fMidiOutput)->Export(&fOutputFile, 0);
				break;
//--------------
//--------------
//Message from the input Midiport
//--------------
		case CHANGE_INPUT_MIDIPORT :
				if (msg->FindPointer("source", (void**)&item) == B_OK)
				{
					((BMidiPort*)fMidiInput)->Open(item->Label());
					((BMidiPort*)fMidiInput)->Start();
				}
				break;
//--------------
//--------------
//Message from the output Midiport
//--------------
		case CHANGE_OUTPUT_MIDIPORT :
				if (msg->FindPointer("source", (void**)&item) == B_OK)
				{
					((BMidiPort*)fMidiOutput)->Open(item->Label());
					((BMidiPort*)fMidiOutput)->Start();
				}
				break;
//--------------
//--------------
//Message from the BeOS Synth
//--------------
		case CHANGE_BEOS_SYNTH_FILE:
				if (fOutputFilePanel)
				{
					msg->FindRef("refs", &fOutputFile);
					((BMidiSynthFile*)fMidiOutput)->UnloadFile();
					if (((BMidiSynthFile*)fMidiOutput)->LoadFile(&fOutputFile) == B_OK)
					{
					BStringView *aStringView = fStandartView->GetOutputStringView();
						aStringView->SetText(fOutputFile.name);
					}
					delete fOutputFilePanel;
					fOutputFilePanel = NULL;
				}
				else
				{
					BMessenger messenger(this)
					fOutputFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger,
						NULL, B_FILE_NODE, false, msg);
					fOutputFilePanel->Show();
				}
				break;
//--------------
		case REWIND_BEOS_SYNTH_FILE :
				((BMidiSynthFile*)fMidiOutput)->Stop();
				fOutputCurrentEvent = 0;
				return;
//--------------
		case PLAY_BEOS_SYNTH_FILE :
				if (fOutputCurrentEvent == 1)
					((BMidiSynthFile*)fMidiOutput)->Resume();
				else
					((BMidiSynthFile*)fMidiOutput)->Start();
				fOutputCurrentEvent = 0;
				break;
//--------------
		case PAUSE_BEOS_SYNTH_FILE :
				fOutputCurrentEvent = 1;
				((BMidiSynthFile*)fMidiOutput)->Pause();
				break;
//--------------
		case CHANGE_SAMPLE_RATE_SYNTH :
				msg->FindInt32("index", (int32*)&temp);
				switch(temp)
				{
					case 0 : be_synth->SetSamplingRate(11025); break;
					case 1 : be_synth->SetSamplingRate(22050); break;
					case 2 : be_synth->SetSamplingRate(44100); break;
				}
				break;
//--------------
		case CHANGE_INTERPOLATION_TYPE_SYNTH :
				msg->FindInt32("index", (int32*)&temp);
				switch(temp)
				{
					case 0 : be_synth->SetInterpolation(B_DROP_SAMPLE); break;
					case 1 : be_synth->SetInterpolation(B_2_POINT_INTERPOLATION); break;
					case 2 : be_synth->SetInterpolation(B_LINEAR_INTERPOLATION); break;
				}
				break;
//--------------
		case CHANGE_REVERB_TYPE_SYNTH :
				msg->FindInt32("index", (int32*)&temp);
				switch(temp)
				{
					case 0 :
						be_synth->EnableReverb(false);
						break;
					case 1 :
						be_synth->EnableReverb(true);
						be_synth->SetReverb(B_REVERB_CLOSET);
						break;
					case 2 :
						be_synth->EnableReverb(true);
						be_synth->SetReverb(B_REVERB_GARAGE);
						break;
					case 3 :
						be_synth->EnableReverb(true);
						be_synth->SetReverb(B_REVERB_BALLROOM);
						break;
					case 4 :
						be_synth->EnableReverb(true);
						be_synth->SetReverb(B_REVERB_CAVERN);
						break;
					case 5 :
						be_synth->EnableReverb(true);
						be_synth->SetReverb(B_REVERB_DUNGEON);
						break;
				}
				break;
//--------------
		case CHANGE_FILE_SAMPLE_SYNTH :
				if (msg->FindPointer("source", (void**)&item) == B_OK)
				{
				BString path = BString(BEOS_SYNTH_DIRECTORY);
					path += item->Label();
				BEntry entry = BEntry(path.String());
					if (entry.InitCheck() == B_OK)
					{
					entry_ref ref;
						be_synth->Unload();
						entry.GetRef(&ref);
						be_synth->LoadSynthData(&ref);
						((BMidiSynth*)fMidiOutput)->EnableInput(true, true);
						if (!be_synth->IsLoaded())
						{
							(new BAlert(NULL, "Instrument File can't be loaded correctly", "OK"))->Go();
						}
						((BMidiSynthFile*)fMidiOutput)->LoadFile(&fOutputFile);
						((BMidiSynthFile*)fMidiOutput)->Start();
					}
					else
					{
						(new BAlert(NULL, "Can't initialise instrument File", "OK"))->Go();
					}
				}
				break;
//--------------
		case CHANGE_VOLUME_SYNTH :
				msg->FindInt32("be:value", (int32*)&temp);
				be_synth->SetSynthVolume(temp / 1000.0);
				break;
//--------------
//--------------
//For the drag and drop function
//--------------
		case B_SIMPLE_DATA : //A file had been dropped into application
				if (msg->FindRef("refs", &ref) == B_OK)
				{
					message = BMessage(INPUT_CHANGE_TO_FILE);
					PostMessage(&message);
					message = BMessage(*msg);
					message.what = CHANGE_INPUT_FILE;
					BMessenger messenger(this)
					fInputFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger,
						NULL, B_FILE_NODE, false, msg);;
					PostMessage(&message);
					message = BMessage(OUTPUT_CHANGE_TO_BEOS_SYNTH);
					PostMessage(&message);
					message = BMessage(PLAY_INPUT_FILE);
					PostMessage(&message);
				}
				break;
//--------------
		case B_CANCEL :
				msg->FindInt32("old_what", (int32*)&temp);
				if (temp == CHANGE_INPUT_FILE)
				{
					delete fInputFilePanel;
					fInputFilePanel = NULL;
				}
				if (temp == CHANGE_OUTPUT_FILE)
				{
					delete fOutputFilePanel;
					fOutputFilePanel = NULL;
				}
				break;
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
//--------------
	default : BWindow::MessageReceived(msg);
	}
}


//----------------------------------------------------------

void MidiPlayerWindow::Disconnect(void)
{//This function must be rewrited for a better disconnection
	if (fMidiInput != NULL)
	{
		fMidiInput->AllNotesOff(false);
		fMidiInput->Disconnect(fMidiDelay);
	}
	if (fMidiOutput != NULL)
	{
		fMidiOutput->AllNotesOff(false);
		fMidiDelay->Disconnect(fMidiOutput);
	}
}

//----------------------------------------------------------

void MidiPlayerWindow::Connect(void)
{//This function must be rewrited for a better connection
	if (fMidiInput != NULL)
		fMidiInput->Connect(fMidiDelay);
	if (fMidiOutput != NULL)
		fMidiDelay->Connect(fMidiOutput);
}

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

int main(int, char**)
{
	MidiPlayerApp *App = new MidiPlayerApp();
	App->Run();
	delete App;
	return B_NO_ERROR;
};

//----------------------------------------------------------
