/*
Author:	Jerome LEVEQUE
Email:	jerl1@caramail.com
*/
#include "MidiPlayerView.h"
#include "Scope.h"
#include "Activity.h"

#include <StringView.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Button.h>
#include <Window.h>
#include <Synth.h>
#include <Directory.h>
#include <Slider.h>

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

MidiPlayerView::MidiPlayerView(void)
				: BView(BRect(0, 0, 660, 360), "StandartView",
					B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fInputStringView = new BStringView(BRect(10, 20, 200, 40), NULL, "None");
	fOutputStringView = new BStringView(BRect(10, 20, 200, 40), NULL, "None");
}

//----------------------------------------------------------

MidiPlayerView::~MidiPlayerView(void)
{
	delete fInputStringView;
	delete fOutputStringView;
}

//----------------------------------------------------------

void MidiPlayerView::AttachedToWindow(void)
{
BPopUpMenu *Menu = NULL;
BMenuField *Field = NULL;
BMessage *msg = NULL;

	AddChild(fInputBox = new BBox(BRect(10, 0, 290, 170), "InputBox"));
	Menu = new BPopUpMenu("Select Input");
	msg = new BMessage(INPUT_CHANGE_TO_FILE);
	Menu->AddItem(new BMenuItem("From File", msg));
	msg = new BMessage(INPUT_CHANGE_TO_MIDIPORT);
	Menu->AddItem(new BMenuItem("From MidiPort", msg));
	Field = new BMenuField(BRect(0, 0, 150, 20), NULL, NULL, Menu);
	fInputBox->SetLabel((BView*)Field);
	
	AddChild(fOutputBox = new BBox(BRect(10, 180, 290, 350), "OutputBox"));
	Menu = new BPopUpMenu("Select Output");
	msg = new BMessage(OUTPUT_CHANGE_TO_FILE);
	Menu->AddItem(new BMenuItem("To File", msg));
	msg = new BMessage(OUTPUT_CHANGE_TO_BEOS_SYNTH);
	Menu->AddItem(new BMenuItem("To BeOS Synth", msg));
	msg = new BMessage(OUTPUT_CHANGE_TO_BEOS_SYNTH_FILE);
	Menu->AddItem(new BMenuItem("To Midi Synth File", msg));
	msg = new BMessage(OUTPUT_CHANGE_TO_MIDIPORT);
	Menu->AddItem(new BMenuItem("To MidiPort", msg));
	Field = new BMenuField(BRect(0, 0, 150, 20), NULL, NULL, Menu);
	fOutputBox->SetLabel((BView*)Field);

	AddChild(fViewBox = new BBox(BRect(300, 0, 650, 350), "OutputBox")); //Width = 350, High = 350
	Menu = new BPopUpMenu("Select View");
	msg = new BMessage(VIEW_CHANGE_TO_NONE);
	Menu->AddItem(new BMenuItem("None", msg));
	msg = new BMessage(VIEW_CHANGE_TO_SCOPE);
	Menu->AddItem(new BMenuItem("Scope", msg));
	msg = new BMessage(VIEW_CHANGE_TO_ACTIVITY);
	Menu->AddItem(new BMenuItem("Midi activity", msg));
	Field = new BMenuField(BRect(0, 0, 150, 20), NULL, NULL, Menu);
	fViewBox->SetLabel((BView*)Field);
	
}

//----------------------------------------------------------

void MidiPlayerView::MessageReceived(BMessage *msg)
{
BPopUpMenu *popupmenu = NULL;
BMenuField *Field = NULL;
Activity *view = NULL;
	switch (msg->what)
	{
//--------------
//--------------
//For The Input
//--------------
		case INPUT_CHANGE_TO_FILE :
				RemoveAll(fInputBox);
				fInputBox->AddChild(fInputStringView);
				fInputBox->AddChild(new BButton(BRect(220, 20, 270, 40), NULL, "Select", new BMessage(CHANGE_INPUT_FILE)));
				fInputBox->AddChild(new BButton(BRect(50, 140, 100, 160), NULL, "Rewind", new BMessage(REWIND_INPUT_FILE)));
				fInputBox->AddChild(new BButton(BRect(110, 140, 160, 160), NULL, "Play", new BMessage(PLAY_INPUT_FILE)));
				fInputBox->AddChild(new BButton(BRect(170, 140, 220, 160), NULL, "Pause", new BMessage(PAUSE_INPUT_FILE)));
				break;
//--------------
		case INPUT_CHANGE_TO_MIDIPORT :
				RemoveAll(fInputBox);
				msg->FindPointer("Menu", (void**)&popupmenu);
				fInputBox->AddChild(Field = new BMenuField(BRect(10, 25, 280, 45), NULL, "Selected", popupmenu));
				Field->SetDivider(60);
				break;
//--------------
//--------------
//For the Output
//--------------
		case OUTPUT_CHANGE_TO_FILE :
				RemoveAll(fOutputBox);
				fOutputBox->AddChild(fOutputStringView);
				fOutputBox->AddChild(new BButton(BRect(220, 20, 270, 40), NULL, "Select", new BMessage(CHANGE_OUTPUT_FILE)));
				fOutputBox->AddChild(new BButton(BRect(50, 140, 100, 160), NULL, "Rewind", new BMessage(REWIND_OUTPUT_FILE)));
				fOutputBox->AddChild(new BButton(BRect(170, 140, 220, 160), NULL, "Save", new BMessage(SAVE_OUTPUT_FILE)));
				break;
//--------------
		case OUTPUT_CHANGE_TO_BEOS_SYNTH :
				RemoveAll(fOutputBox);
				SetBeOSSynthView(fOutputBox);
				break;
//--------------
		case OUTPUT_CHANGE_TO_BEOS_SYNTH_FILE :
				RemoveAll(fOutputBox);
				fOutputBox->AddChild(fOutputStringView);
				fOutputBox->AddChild(new BButton(BRect(220, 20, 270, 40), NULL, "Select", new BMessage(CHANGE_BEOS_SYNTH_FILE)));
				fOutputBox->AddChild(new BButton(BRect(50, 140, 100, 160), NULL, "Rewind", new BMessage(REWIND_BEOS_SYNTH_FILE)));
				fOutputBox->AddChild(new BButton(BRect(110, 140, 160, 160), NULL, "Play", new BMessage(PLAY_BEOS_SYNTH_FILE)));
				fOutputBox->AddChild(new BButton(BRect(170, 140, 220, 160), NULL, "Pause", new BMessage(PAUSE_BEOS_SYNTH_FILE)));
				SetBeOSSynthView(fOutputBox);
				break;
//--------------
		case OUTPUT_CHANGE_TO_MIDIPORT :
				RemoveAll(fOutputBox);
				msg->FindPointer("Menu", (void**)&popupmenu);
				fOutputBox->AddChild(Field = new BMenuField(BRect(10, 25, 280, 45), NULL, "Selected", popupmenu));
				Field->SetDivider(60);
				break;
//--------------
//--------------
//For the display view
//--------------
		case VIEW_CHANGE_TO_NONE :
				RemoveAll(fViewBox);
				break;
//--------------
		case VIEW_CHANGE_TO_SCOPE :
				RemoveAll(fViewBox);
				fViewBox->AddChild(new Scope(BRect(10, 25, 340, 340)));
				break;
//--------------
		case VIEW_CHANGE_TO_ACTIVITY :
				RemoveAll(fViewBox);
				msg->FindPointer("View", (void**)&view);
				fViewBox->AddChild(view);
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
	default: BView::MessageReceived(msg);
	}
}

//----------------------------------------------------------

BStringView *MidiPlayerView::GetInputStringView(void)
{
	return fInputStringView;
}	

//----------------------------------------------------------

BStringView *MidiPlayerView::GetOutputStringView(void)
{
	return fOutputStringView;
}	

//----------------------------------------------------------

void MidiPlayerView::RemoveAll(BView *aView)
{//Remove and delete all view from aView exept the first one an TheStringView (delete)
BView *Temp = NULL;
	aView->Window()->Lock();
	Temp = aView->ChildAt(1);
	while (Temp != NULL)
	{
		aView->RemoveChild(Temp);
		if ((Temp != fInputStringView) && (Temp != fOutputStringView))
			delete Temp;
		Temp = aView->ChildAt(1);
	}
	aView->Window()->Unlock();
}

//----------------------------------------------------------

void MidiPlayerView::SetBeOSSynthView(BView *aView)
{
BPopUpMenu *Menu = NULL;
BMenuField *field = NULL;
BMessage *msg = NULL;
BMenuItem *item = NULL;
int32 temp = 0;

	Menu = new BPopUpMenu("Sample Rate");
	temp = be_synth->SamplingRate();
	msg = new BMessage(CHANGE_SAMPLE_RATE_SYNTH);
	Menu->AddItem(item = new BMenuItem("11025 Hz", msg));
	if (temp == 11025) item->SetMarked(true);
	msg = new BMessage(CHANGE_SAMPLE_RATE_SYNTH);
	Menu->AddItem(item = new BMenuItem("22050 Hz", msg));
	if (temp == 22050) item->SetMarked(true);
	msg = new BMessage(CHANGE_SAMPLE_RATE_SYNTH);
	Menu->AddItem(item = new BMenuItem("44100 Hz", msg));
	if (temp == 44100) item->SetMarked(true);
	field = new BMenuField(BRect(5, 50, 145, 70), NULL, "Sampling Rate", Menu);
	aView->AddChild(field);

	Menu = new BPopUpMenu("Interpolation");
	temp = be_synth->Interpolation();
	msg = new BMessage(CHANGE_INTERPOLATION_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Drop", msg));
	if (temp == B_DROP_SAMPLE) item->SetMarked(true);
	msg = new BMessage(CHANGE_INTERPOLATION_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("2 points", msg));
	if (temp == B_2_POINT_INTERPOLATION) item->SetMarked(true);
	msg = new BMessage(CHANGE_INTERPOLATION_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Linear", msg));
	if (temp == B_LINEAR_INTERPOLATION) item->SetMarked(true);
	field = new BMenuField(BRect(145, 50, 285, 70), NULL, "Interpolation", Menu);
	aView->AddChild(field);

	Menu = new BPopUpMenu("Reverb");
	temp = be_synth->Reverb();
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("None", msg));
	if (!be_synth->IsReverbEnabled())
	{
		item->SetMarked(true);
		temp = -1;
	}
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Closet", msg));
	if (temp == B_REVERB_CLOSET) item->SetMarked(true);
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Garage", msg));
	if (temp == B_REVERB_GARAGE) item->SetMarked(true);
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Ballroom", msg));
	if (temp == B_REVERB_BALLROOM) item->SetMarked(true);
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Cavern", msg));
	if (temp == B_REVERB_CAVERN) item->SetMarked(true);
	msg = new BMessage(CHANGE_REVERB_TYPE_SYNTH);
	Menu->AddItem(item = new BMenuItem("Dungeon", msg));
	if (temp == B_REVERB_DUNGEON) item->SetMarked(true);
	field = new BMenuField(BRect(5, 75, 145, 95), NULL, "Reverb", Menu);
	aView->AddChild(field);

BDirectory dir = BDirectory(BEOS_SYNTH_DIRECTORY);
entry_ref refs;
	Menu = new BPopUpMenu("Standart File");
	while (dir.GetNextRef(&refs) != B_ENTRY_NOT_FOUND)
	{
		msg = new BMessage(CHANGE_FILE_SAMPLE_SYNTH);
		Menu->AddItem(item = new BMenuItem(refs.name, msg));
	}
	field = new BMenuField(BRect(145, 75, 285, 95), NULL, "File", Menu);
	field->SetDivider(25);
	aView->AddChild(field);

BSlider *slider = new BSlider(BRect(5, 100, 275, 130), NULL, "Volume",
								new BMessage(CHANGE_VOLUME_SYNTH),
								0, 1500, B_TRIANGLE_THUMB);
	temp = (int32)(be_synth->SynthVolume() * be_synth->SampleVolume() * 1000);
	slider->SetValue(temp);
	slider->SetLimitLabels("Min", "Max");
	aView->AddChild(slider);
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
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
