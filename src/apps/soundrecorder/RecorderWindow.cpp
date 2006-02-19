/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#include <Application.h>
#include <Alert.h>
#include <Debug.h>
#include <Screen.h>
#include <Button.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Box.h>
#include <ScrollView.h>
#include <Beep.h>
#include <StringView.h>
#include <String.h>
#include <Slider.h>
#include <Message.h>

#include <Path.h>
#include <FindDirectory.h>
#include <MediaAddOn.h>

#include <SoundPlayer.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <MediaRoster.h>
#include <TimeSource.h>

#include "RecorderWindow.h"
#include "SoundConsumer.h"
#include "SoundListView.h"
#include "array_delete.h"
#include "FileUtils.h"

#if ! NDEBUG
#define FPRINTF(args) fprintf args
#else
#define FPRINTF(args)
#endif

#define DEATH FPRINTF
#define CONNECT FPRINTF
#define WINDOW FPRINTF

// default window positioning 
static const float MIN_WIDTH = 400.0f;
static const float MIN_HEIGHT = 336.0f;
static const float XPOS = 100.0f;
static const float YPOS = 200.0f;

#define FOURCC(a,b,c,d)	((((uint32)(d)) << 24) | (((uint32)(c)) << 16) | (((uint32)(b)) << 8) | ((uint32)(a)))

struct riff_struct
{ 
	uint32 riff_id; // 'RIFF'
	uint32 len;
	uint32 wave_id;	// 'WAVE'
}; 

struct chunk_struct 
{ 
	uint32 fourcc;
	uint32 len;
};

struct format_struct 
{ 
	uint16 format_tag; 
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
}; 


struct wave_struct
{
	struct riff_struct riff;
	struct chunk_struct format_chunk;
	struct format_struct format;
	struct chunk_struct data_chunk;
};


RecorderWindow::RecorderWindow() :
	BWindow(BRect(XPOS,YPOS,XPOS+MIN_WIDTH,YPOS+MIN_HEIGHT), "Sound Recorder", B_TITLED_WINDOW, 
		B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE),
		fPlayer(NULL),
		fSoundList(NULL),
		fPlayFile(NULL),
		fPlayTrack(NULL),
		fPlayFrames(0),
		fLooping(false),
		fSavePanel(NULL),
		fInitCheck(B_OK)
{
	fRoster = NULL;
	fRecordButton = NULL;
	fPlayButton = NULL;
	fStopButton = NULL;
	fSaveButton = NULL;
	fLoopButton = NULL;
	fLengthControl = NULL;
	fInputField = NULL;
	fRecordNode = 0;
	fRecording = false;
	fTempCount = -1;
	fButtonState = btnPaused;

	CalcSizes(MIN_WIDTH, MIN_HEIGHT);
	
	fInitCheck = InitWindow();
	if (fInitCheck != B_OK) {
		ErrorAlert("connect to media server", fInitCheck);
		PostMessage(B_QUIT_REQUESTED);
	} else
		Show();
}

RecorderWindow::~RecorderWindow()
{
	//	The sound consumer and producer are Nodes; it has to be Release()d and the Roster
	//	will reap it when it's done.
	if (fRecordNode) {
		fRecordNode->Release();
	}
	if (fPlayer) {
		delete fPlayer;
	}
	
	if (fPlayTrack && fPlayFile)
		fPlayFile->ReleaseTrack(fPlayTrack);
	if (fPlayFile)
		delete fPlayFile;
	fPlayTrack = NULL;
	fPlayFile = NULL;
	
	//	Clean up items in list view.
	if (fSoundList) {
		fSoundList->DeselectAll();
		for (int ix=0; ix<fSoundList->CountItems(); ix++) {
			WINDOW((stderr, "clean up item %d\n", ix+1));
			SoundListItem * item = dynamic_cast<SoundListItem *>(fSoundList->ItemAt(ix));
			if (item) {
				if (item->IsTemp()) {
					item->Entry().Remove();	//	delete temp file
				}
				delete item;
			}
		}
		fSoundList->MakeEmpty();
	}
	//	Clean up currently recording file, if any.
	fRecEntry.Remove();
	fRecEntry.Unset();
}


status_t
RecorderWindow::InitCheck()
{
	return fInitCheck;
}


void
RecorderWindow::CalcSizes(float min_wid, float min_hei)
{
	//	Set up size limits based on new screen size
	BScreen screen(this);
	BRect r = screen.Frame();
	float wid = r.Width()-12;
	SetSizeLimits(min_wid, wid, min_hei, r.Height()-24);

	//	Don't zoom to cover all of screen; user can resize last bit if necessary.
	//	This leaves other windows visible.
	if (wid > 640) {
		wid = 640 + (wid-640)/2;
	}
	SetZoomLimits(wid, r.Height()-24);
}


status_t
RecorderWindow::InitWindow()
{
	BPopUpMenu * popup = 0;
	status_t error;
	
	try {
		//	Find temp directory for recorded sounds.
		BPath path;
		if (!(error = find_directory(B_COMMON_TEMP_DIRECTORY, &path))) {
			error = fTempDir.SetTo(path.Path());
		}
		if (error < 0) {
			goto bad_mojo;
		}

		//	Make sure the media roster is there (which means the server is there).
		fRoster = BMediaRoster::Roster(&error);
		if (!fRoster) {
			goto bad_mojo;
		}

		error = fRoster->GetAudioInput(&fAudioInputNode);
		if (error < B_OK) {	//	there's no input?
			goto bad_mojo;
		}

		error = fRoster->GetAudioMixer(&fAudioMixerNode);
		if (error < B_OK) {	//	there's no mixer?
			goto bad_mojo;
		}

		//	Create our internal Node which records sound, and register it.
		fRecordNode = new SoundConsumer("Sound Recorder");
		error = fRoster->RegisterNode(fRecordNode);
		if (error < B_OK) {
			goto bad_mojo;
		}

		//	Create the window header with controls
		BRect r(Bounds());
		r.bottom = r.top + 175;
		BBox *background = new BBox(r, "_background", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
		AddChild(background);

		
		
		r = background->Bounds();
		r.left = 2;
		r.right = r.left + 39;
		r.bottom = r.top + 104;
		fVUView = new VUView(r, B_FOLLOW_LEFT|B_FOLLOW_TOP);
		background->AddChild(fVUView);
		
		r = background->Bounds();
		r.left = r.left + 40;
		r.bottom = r.top + 104;
		fScopeView = new ScopeView(r, B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
		background->AddChild(fScopeView);
		
		r = background->Bounds();
		r.left = 2;
		r.right -= 26;
		r.top = 115;
		r.bottom = r.top + 30;
		fTrackSlider = new TrackSlider(r, "trackSlider", new BMessage(POSITION_CHANGED), B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
		background->AddChild(fTrackSlider);
		
		BRect buttonRect;
  
  		//	Button for rewinding
		buttonRect = BRect(BPoint(0,0), kSkipButtonSize);
		buttonRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-7, 25));
		fRewindButton = new TransportButton(buttonRect, "Rewind", 
			kSkipBackBitmapBits, kPressedSkipBackBitmapBits, kDisabledSkipBackBitmapBits, 
			new BMessage(REWIND));
		background->AddChild(fRewindButton);
		
		//	Button for stopping recording or playback
		buttonRect = BRect(BPoint(0,0), kStopButtonSize);
		buttonRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-48, 25));
		fStopButton = new TransportButton(buttonRect, "Stop", 
			kStopButtonBitmapBits, kPressedStopButtonBitmapBits, kDisabledStopButtonBitmapBits, 
			new BMessage(STOP));
		background->AddChild(fStopButton);
		
		//	Button for starting playback of selected sound
		BRect playRect(BPoint(0,0), kPlayButtonSize);
		playRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-82, 25));
		fPlayButton = new PlayPauseButton(playRect, "Play", 
			new BMessage(PLAY), new BMessage(PLAY_PERIOD), ' ', 0);
		background->AddChild(fPlayButton);
		
		//	Button for forwarding
		buttonRect = BRect(BPoint(0,0), kSkipButtonSize);
		buttonRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-133, 25));
		fForwardButton = new TransportButton(buttonRect, "Forward", 
			kSkipForwardBitmapBits, kPressedSkipForwardBitmapBits, kDisabledSkipForwardBitmapBits, 
			new BMessage(FORWARD));
		background->AddChild(fForwardButton);
		
		//	Button to start recording (or waiting for sound)
		buttonRect = BRect(BPoint(0,0), kRecordButtonSize);
		buttonRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-174, 25));
		fRecordButton = new RecordButton(buttonRect, "Record", 
			new BMessage(RECORD), new BMessage(RECORD_PERIOD));
		background->AddChild(fRecordButton);

		//	Button for saving selected sound
		buttonRect = BRect(BPoint(0,0), kDiskButtonSize);
		buttonRect.OffsetTo(background->Bounds().LeftBottom() - BPoint(-250, 21));
		fSaveButton = new TransportButton(buttonRect, "Save", 
			kDiskButtonBitmapsBits, kPressedDiskButtonBitmapsBits, kDisabledDiskButtonBitmapsBits, new BMessage(SAVE));
		fSaveButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		background->AddChild(fSaveButton);
		
		//	Button Loop
		buttonRect = BRect(BPoint(0,0), kArrowSize);
		buttonRect.OffsetTo(background->Bounds().RightBottom() - BPoint(23, 48));
		fLoopButton = new DrawButton(buttonRect, "Loop", 
			kLoopArrowBits, kArrowBits, new BMessage(LOOP));
		fLoopButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		fLoopButton->SetTarget(this);
		background->AddChild(fLoopButton);
		
		buttonRect = BRect(BPoint(0,0), kSpeakerIconBitmapSize);
		buttonRect.OffsetTo(background->Bounds().RightBottom() - BPoint(121, 17));
		SpeakerView *speakerView = new SpeakerView(buttonRect, B_FOLLOW_LEFT | B_FOLLOW_TOP);
		speakerView->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		background->AddChild(speakerView);
				
		buttonRect = BRect(BPoint(0,0), BPoint(84, 19));
		buttonRect.OffsetTo(background->Bounds().RightBottom() - BPoint(107, 20));
		fVolumeSlider = new VolumeSlider(buttonRect, "volumeSlider", B_FOLLOW_LEFT | B_FOLLOW_TOP);
		fVolumeSlider->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		background->AddChild(fVolumeSlider);
		
		// Button to mask/see sounds list
		buttonRect = BRect(BPoint(0,0), kUpDownButtonSize);
		buttonRect.OffsetTo(background->Bounds().RightBottom() - BPoint(21, 25));
		fUpDownButton = new UpDownButton(buttonRect, new BMessage(VIEW_LIST));
		fUpDownButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);
		background->AddChild(fUpDownButton);
		
		r = Bounds();
		r.top = background->Bounds().bottom + 1;
		fBottomBox = new BBox(r, "bottomBox", B_FOLLOW_ALL);
		fBottomBox->SetBorder(B_NO_BORDER);
		AddChild(fBottomBox);
		
		//	The actual list of recorded sounds (initially empty) sits
		//	below the header with the controls.
		r = fBottomBox->Bounds();
		r.left += 190;
		r.InsetBy(10, 10);
		r.left -= 10;
		r.top += 4;
		r.right -= B_V_SCROLL_BAR_WIDTH;
		r.bottom -= 25;
		fSoundList = new SoundListView(r, "Sound List", B_FOLLOW_ALL);
		fSoundList->SetSelectionMessage(new BMessage(SOUND_SELECTED));
		fSoundList->SetViewColor(216, 216, 216);
		BScrollView *scroller = new BScrollView("scroller", fSoundList, B_FOLLOW_ALL,
			0, false, true, B_FANCY_BORDER);
		fBottomBox->AddChild(scroller);
		
		r = fBottomBox->Bounds();
		r.right = r.left + 190;
		r.bottom -= 25;
		r.InsetBy(10, 8);
		r.top -= 1;
		fFileInfoBox = new BBox(r, "fileinfo", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		fFileInfoBox->SetLabel("File Info");
		
		r = fFileInfoBox->Bounds();
		r.left = 8;
		r.top = 13;
		r.bottom = r.top + 15;
		r.right -= 10;
		fFilename = new BStringView(r, "filename", "File Name:");
		fFileInfoBox->AddChild(fFilename);
		r.top += 13;
		r.bottom = r.top + 15;
		fFormat = new BStringView(r, "format", "Format:");
		fFileInfoBox->AddChild(fFormat);
		r.top += 13;
		r.bottom = r.top + 15;
		fCompression = new BStringView(r, "compression", "Compression:");
		fFileInfoBox->AddChild(fCompression);
		r.top += 13;
		r.bottom = r.top + 15;
		fChannels = new BStringView(r, "channels", "Channels:");
		fFileInfoBox->AddChild(fChannels);
		r.top += 13;
		r.bottom = r.top + 15;
		fSampleSize = new BStringView(r, "samplesize", "Sample Size:");
		fFileInfoBox->AddChild(fSampleSize);
		r.top += 13;
		r.bottom = r.top + 15;
		fSampleRate = new BStringView(r, "samplerate", "Sample Rate:");
		fFileInfoBox->AddChild(fSampleRate);
		r.top += 13;
		r.bottom = r.top + 15;
		fDuration = new BStringView(r, "duration", "Duration:");
		fFileInfoBox->AddChild(fDuration);
		
		//	Input selection lists all available physical inputs that produce
		//	buffers with B_MEDIA_RAW_AUDIO format data.
		popup = new BPopUpMenu("Input");
		int max_input_count = 64;
		dormant_node_info dni[max_input_count];
		
		int32 real_count = max_input_count;
		media_format output_format;
		output_format.type = B_MEDIA_RAW_AUDIO;
		output_format.u.raw_audio = media_raw_audio_format::wildcard;
		error = fRoster->GetDormantNodes(dni, &real_count, 0, &output_format,
			0, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
		if (real_count > max_input_count) {
			WINDOW((stderr, "dropped %ld inputs\n", real_count - max_input_count));
			real_count = max_input_count;
		}
		char selected_name[B_MEDIA_NAME_LENGTH] = "Default Input";
		BMessage * msg;
		BMenuItem * item;
		for (int ix=0; ix<real_count; ix++) {
			msg = new BMessage(INPUT_SELECTED);
			msg->AddData("node", B_RAW_TYPE, &dni[ix], sizeof(dni[ix]));
			item = new BMenuItem(dni[ix].name, msg);
			popup->AddItem(item);
			media_node_id ni[12];
			int32 ni_count = 12;
			error = fRoster->GetInstancesFor(dni[ix].addon, dni[ix].flavor_id, ni, &ni_count);
			if (error == B_OK)
				for (int iy=0; iy<ni_count; iy++)
					if (ni[iy] == fAudioInputNode.node) {
						strcpy(selected_name, dni[ix].name);
						break;
					}
		}
		
		//	Create the actual widget
		BRect frame(fBottomBox->Bounds());
		r = frame;
		r.left = 42;
		r.right = (r.left + r.right) / 2;
		r.InsetBy(10,10);
		r.top = r.bottom - 18;
		fInputField = new BMenuField(r, "Input", "Input:", popup);
		fInputField->SetDivider(50);
		fBottomBox->AddChild(fInputField);

		//	Text field for entering length to record (in seconds)
		r.OffsetBy(r.Width()+20, 1);
		r.right -= 70;
		msg = new BMessage(LENGTH_CHANGED);
		fLengthControl = new BTextControl(r, "Length", "Length:", "8", msg);
		fLengthControl->SetDivider(fLengthControl->StringWidth("Length:") + 4.0f);
		fLengthControl->SetAlignment(B_ALIGN_CENTER, B_ALIGN_RIGHT);
		fBottomBox->AddChild(fLengthControl);

		r.left += r.Width()+5;
		r.right = r.left + 65;
		r.bottom -= 1;
		BStringView* lenUnits = new BStringView(r, "Seconds", "seconds");
		fBottomBox->AddChild(lenUnits);
		
		fBottomBox->AddChild(fFileInfoBox);
		
		fBottomBox->Hide();
		CalcSizes(Frame().Width(), MIN_HEIGHT-161);
		ResizeTo(Frame().Width(), MIN_HEIGHT-161);

		
		popup->Superitem()->SetLabel(selected_name);
		
		// Make sure the save panel is happy.
		fSavePanel = new BFilePanel(B_SAVE_PANEL);
		fSavePanel->SetTarget(this);
	}
	catch (...) {
		goto bad_mojo;
	}
	UpdateButtons();
	return B_OK;

	//	Error handling.
bad_mojo:
	if (error >= 0) {
		error = B_ERROR;
	}
	if (fRecordNode) {
		fRecordNode->Release();
	}
	
	delete fPlayer;
	if (!fInputField) {
		delete popup;
	}
	return error;
}


bool
RecorderWindow::QuitRequested()	//	this means Close pressed
{
	StopRecording();
	StopPlaying();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
RecorderWindow::MessageReceived(BMessage * message)
{
	//	Your average generic message dispatching switch() statement.
	switch (message->what) {
	case INPUT_SELECTED:
		Input(message);
		break;
	case LENGTH_CHANGED:
		Length(message);
		break;
	case SOUND_SELECTED:
		Selected(message);
		break;
	case STOP_PLAYING:
		StopPlaying();
		break;
	case STOP_RECORDING:
		StopRecording();
		break;
	case PLAY_PERIOD:
		if (fPlayer) {
			if (fPlayer->HasData())
				fPlayButton->SetPlaying();
			else
				fPlayButton->SetPaused();
		}
		break;
	case RECORD_PERIOD:
		fRecordButton->SetRecording();
		break;
	case RECORD:
		Record(message);
		break;
	case STOP:
		Stop(message);
		break;
	case PLAY:
		Play(message);
		break;
	case SAVE:
		Save(message);
		break;
	case B_SAVE_REQUESTED:
		DoSave(message);
		break;
	case VIEW_LIST:
		if (fUpDownButton->Value() == B_CONTROL_ON) {
			fBottomBox->Show();
			CalcSizes(Frame().Width(), MIN_HEIGHT);
			ResizeTo(Frame().Width(), MIN_HEIGHT);
		} else {
			fBottomBox->Hide();
			CalcSizes(Frame().Width(), MIN_HEIGHT-161);
			ResizeTo(Frame().Width(), MIN_HEIGHT-161);
			
		}
		break;
	case UPDATE_TRACKSLIDER:
		{
			bigtime_t timestamp = fPlayTrack->CurrentTime();
			fTrackSlider->SetMainTime(timestamp, false);
			fScopeView->SetMainTime(timestamp);
		}
		break;
	case POSITION_CHANGED:
		{
		bigtime_t right, left, main;
		if (message->FindInt64("main", &main) == B_OK) {
			if (fPlayTrack) {
				fPlayTrack->SeekToTime(fTrackSlider->MainTime());
				fPlayFrame = fPlayTrack->CurrentFrame();
			}
			fScopeView->SetMainTime(main);
		}
		if (message->FindInt64("right", &right) == B_OK) {
			if (fPlayTrack)
				fPlayLimit = MIN(fPlayFrames, (off_t)(right * fPlayFormat.u.raw_audio.frame_rate/1000000LL));
			fScopeView->SetRightTime(right);
		}
		if (message->FindInt64("left", &left) == B_OK) {
			fScopeView->SetLeftTime(left);
		}
		}
		break;
	case LOOP:
		fLooping = fLoopButton->ButtonState();
		break;
	case B_SIMPLE_DATA:
	case B_REFS_RECEIVED:
		{
			RefsReceived(message);
			break;
		}
	default:
		BWindow::MessageReceived(message);
		break;
	}
}


void
RecorderWindow::Record(BMessage * message)
{	
	//	User pressed Record button
	fRecording = true;
	int seconds = atoi(fLengthControl->Text());
	if (seconds < 1) {
		ErrorAlert("record a sound that's shorter than a second", B_ERROR);
		return;
	}

	if (fButtonState != btnPaused) {
		StopRecording();
		return;			//	user is too fast on the mouse
	}
	SetButtonState(btnRecording);
	fRecordButton->SetRecording();

	char name[256];
	//	Create a file with a temporary name
	status_t err = NewTempName(name);
	if (err < B_OK) {
		ErrorAlert("find an unused name to use for the new recording", err);
		return;
	}
	//	Find the file so we can refer to it later
	err = fTempDir.FindEntry(name, &fRecEntry);
	if (err < B_OK) {
		ErrorAlert("find the temporary file created to hold the new recording", err);
		return;
	}
	err = fRecFile.SetTo(&fTempDir, name, O_RDWR);
	if (err < B_OK) {
		ErrorAlert("open the temporary file created to hold the new recording", err);
		fRecEntry.Unset();
		return;
	}
	//	Reserve space on disk (creates fewer fragments)
	err = fRecFile.SetSize(seconds*4*48000LL);
	if (err < B_OK) {
		ErrorAlert("record a sound that long", err);
		fRecEntry.Remove();
		fRecEntry.Unset();
		return;
	}
	fRecLimit = seconds*4*48000LL;
	fRecSize = 0;

	fRecFile.Seek(sizeof(struct wave_struct), SEEK_SET);

	//	Hook up input
	err = MakeRecordConnection(fAudioInputNode);
	if (err < B_OK) {
		ErrorAlert("connect to the selected sound input", err);
		fRecEntry.Remove();
		fRecEntry.Unset();
		return;
	}

	//	And get it going...
	bigtime_t then = fRecordNode->TimeSource()->Now()+50000LL;
	fRoster->StartNode(fRecordNode->Node(), then);
	if (fAudioInputNode.kind & B_TIME_SOURCE) {
		fRoster->StartNode(fAudioInputNode, fRecordNode->TimeSource()->RealTimeFor(then, 0));
	}
	else {
		fRoster->StartNode(fAudioInputNode, then);
	}
}

void
RecorderWindow::Play(BMessage * message)
{
	if (fPlayer) {
		//	User pressed Play button and playing
		if (fPlayer->HasData())
			fPlayButton->SetPaused();
		else
			fPlayButton->SetPlaying();
		fPlayer->SetHasData(!fPlayer->HasData());
		return;
	}
	
	SetButtonState(btnPlaying);
	fPlayButton->SetPlaying();
	
	if (!fPlayTrack) {
		ErrorAlert("get the file to play", B_ERROR);
		return;
	}
	
	fPlayLimit = MIN(fPlayFrames, (off_t)(fTrackSlider->RightTime()*fPlayFormat.u.raw_audio.frame_rate/1000000LL));
	fPlayTrack->SeekToTime(fTrackSlider->MainTime());
	fPlayFrame = fPlayTrack->CurrentFrame();
	
	// Create our internal Node which plays sound, and register it.
	fPlayer = new BSoundPlayer(fAudioMixerNode, &fPlayFormat.u.raw_audio, "Sound Player");
	status_t err = fPlayer->InitCheck();
	if (err < B_OK) {
		return;
	}
	
	fVolumeSlider->SetSoundPlayer(fPlayer);
	fPlayer->SetCallbacks(PlayFile, NotifyPlayFile, this);
	
	//	And get it going...
	fPlayer->Start();
	fPlayer->SetHasData(true);
}

void
RecorderWindow::Stop(BMessage * message)
{
	//	User pressed Stop button.
	//	Stop recorder.
	StopRecording();
	//	Stop player.
	StopPlaying();
}

void
RecorderWindow::Save(BMessage * message)
{
	//	User pressed Save button.
	//	Find the item to save.
	int32 index = fSoundList->CurrentSelection();
	SoundListItem* pItem = dynamic_cast<SoundListItem*>(fSoundList->ItemAt(index));
	if ((! pItem) || (pItem->Entry().InitCheck() != B_OK)) {
		return;
	}
	
	// Update the save panel and show it.
	char filename[B_FILE_NAME_LENGTH];
	pItem->Entry().GetName(filename);
	BMessage saveMsg(B_SAVE_REQUESTED);
	entry_ref ref;
	pItem->Entry().GetRef(&ref);
	
	saveMsg.AddPointer("sound list item", pItem);
	fSavePanel->SetSaveText(filename);
	fSavePanel->SetMessage(&saveMsg);
	fSavePanel->Show();
}

void
RecorderWindow::DoSave(BMessage * message)
{
	// User picked a place to put the file.
	// Find the location of the old (e.g.
	// temporary file), and the name of the
	// new file to save.
	entry_ref old_ref, new_dir_ref;
	const char* new_name;
	SoundListItem* pItem;
		
	if ((message->FindPointer("sound list item", (void**) &pItem) == B_OK)
		&& (message->FindRef("directory", &new_dir_ref) == B_OK)
		&& (message->FindString("name", &new_name) == B_OK))
	{
		BEntry& oldEntry = pItem->Entry();
		BFile oldFile(&oldEntry, B_READ_WRITE);
		if (oldFile.InitCheck() != B_OK)
			return;

		BDirectory newDir(&new_dir_ref);
		if (newDir.InitCheck() != B_OK)
			return;
			
		BFile newFile;		
		newDir.CreateFile(new_name, &newFile); 
		
		if (newFile.InitCheck() != B_OK)
			return;
				
		status_t err = CopyFile(newFile, oldFile);
		
		if (err == B_OK) {
			// clean up the sound list and item
			if (pItem->IsTemp())
				oldEntry.Remove(); // blows away temp file!
			oldEntry.SetTo(&newDir, new_name);
			pItem->SetTemp(false);	// don't blow the new entry away when we exit!
			fSoundList->Invalidate();
		}
	} else {
		WINDOW((stderr, "Couldn't save file.\n"));
	}
}


void
RecorderWindow::Length(BMessage * message)
{
	//	User changed the Length field -- validate
	const char * ptr = fLengthControl->Text();
	const char * start = ptr;
	const char * anchor = ptr;
	const char * end = fLengthControl->Text() + fLengthControl->TextView()->TextLength();
	while (ptr < end) {
		//	Remember the last start-of-character for UTF-8 compatibility
		//	needed in call to Select() below (which takes bytes).
		if (*ptr & 0x80) {
			if (*ptr & 0xc0 == 0xc0) {
				anchor = ptr;
			}
		}
		else {
			anchor = ptr;
		}
		if (!isdigit(*ptr)) {
			fLengthControl->TextView()->MakeFocus(true);
			fLengthControl->TextView()->Select(anchor-start, fLengthControl->TextView()->TextLength());
			beep();
			break;
		}
		ptr++;
	}
}


void
RecorderWindow::Input(BMessage * message)
{
	//	User selected input from pop-up
	const dormant_node_info * dni = 0;
	ssize_t size = 0;
	if (message->FindData("node", B_RAW_TYPE, (const void **)&dni, &size)) {
		return;		//	bad input selection message
	}
	
	media_node_id node_id;
	status_t error = fRoster->GetInstancesFor(dni->addon, dni->flavor_id, &node_id);
	if (error != B_OK) {
		fRoster->InstantiateDormantNode(*dni, &fAudioInputNode);
	} else {
		fRoster->GetNodeFor(node_id, &fAudioInputNode);
	}
}

void
RecorderWindow::Selected(BMessage * message)
{
	//	User selected a sound in list view
	UpdatePlayFile();
	UpdateButtons();
}

status_t
RecorderWindow::MakeRecordConnection(const media_node & input)
{
	CONNECT((stderr, "RecorderWindow::MakeRecordConnection()\n"));
	
	//	Find an available output for the given input node.
	int32 count = 0;
	status_t err = fRoster->GetFreeOutputsFor(input, &fAudioOutput, 1, &count, B_MEDIA_RAW_AUDIO);
	if (err < B_OK) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): couldn't get free outputs from audio input node\n"));
		return err;
	}
	if (count < 1) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): no free outputs from audio input node\n"));
		return B_BUSY;
	}

	//	Find an available input for our own Node. Note that we go through the
	//	MediaRoster; calling Media Kit methods directly on Nodes in our app is
	//	not OK (because synchronization happens in the service thread, not in
	//	the calling thread).
	// TODO: explain this
	err = fRoster->GetFreeInputsFor(fRecordNode->Node(), &fRecInput, 1, &count, B_MEDIA_RAW_AUDIO);
	if (err < B_OK) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): couldn't get free inputs for sound recorder\n"));
		return err;
	}
	if (count < 1) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): no free inputs for sound recorder\n"));
		return B_BUSY;
	}

	//	Find out what the time source of the input is.
	//	For most nodes, we just use the preferred time source (the DAC) for synchronization.
	//	However, nodes that record from an input need to synchronize to the audio input node
	//	instead for best results.
	//	MakeTimeSourceFor gives us a "clone" of the time source node that we can manipulate
	//	to our heart's content. When we're done with it, though, we need to call Release()
	//	on the time source node, so that it keeps an accurate reference count and can delete
	//	itself when it's no longer needed.
	// TODO: what about filters connected to audio input?
	media_node use_time_source;
	BTimeSource * tsobj = fRoster->MakeTimeSourceFor(input);
	if (! tsobj) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): couldn't clone time source from audio input node\n"));
		return B_MEDIA_BAD_NODE;
	}

	//	Apply the time source in effect to our own Node.
	err = fRoster->SetTimeSourceFor(fRecordNode->Node().node, tsobj->Node().node);
	if (err < B_OK) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): couldn't set the sound recorder's time source\n"));
		tsobj->Release();
		return err;
	}

	//	Get a format, any format.
	media_format fmt;
	fmt.u.raw_audio = fAudioOutput.format.u.raw_audio;
	fmt.type = B_MEDIA_RAW_AUDIO;

	//	Tell the consumer where we want data to go.
	err = fRecordNode->SetHooks(RecordFile, NotifyRecordFile, this);
	if (err < B_OK) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): couldn't set the sound recorder's hook functions\n"));
		tsobj->Release();
		return err;
	}

	//	Using the same structs for input and output is OK in BMediaRoster::Connect().
	err = fRoster->Connect(fAudioOutput.source, fRecInput.destination, &fmt, &fAudioOutput, &fRecInput);
	if (err < B_OK) {
		CONNECT((stderr, "RecorderWindow::MakeRecordConnection(): failed to connect sound recorder to audio input node.\n"));
		tsobj->Release();
		fRecordNode->SetHooks(0, 0, 0);
		return err;
	}

	//	Start the time source if it's not running.
	if ((tsobj->Node() != input) && !tsobj->IsRunning()) {
		fRoster->StartNode(tsobj->Node(), BTimeSource::RealTime());
	}
	tsobj->Release();	//	we're done with this time source instance!
	return B_OK;
}


status_t
RecorderWindow::BreakRecordConnection()
{
	status_t err;

	//	If we are the last connection, the Node will stop automatically since it
	//	has nowhere to send data to.
	err = fRoster->StopNode(fRecInput.node, 0);
	err = fRoster->Disconnect(fAudioOutput.node.node, fAudioOutput.source, fRecInput.node.node, fRecInput.destination);
	fAudioOutput.source = media_source::null;
	fRecInput.destination = media_destination::null;
	return err;
}

status_t
RecorderWindow::StopRecording()
{
	if (!fRecording)
		return B_OK;
	fRecording = false;
	BreakRecordConnection();
	fRecordNode->SetHooks(NULL,NULL,NULL);
	if (fRecSize > 0) {
		
		wave_struct header;
		header.riff.riff_id = FOURCC('R','I','F','F');
		header.riff.len = fRecSize + 36;
		header.riff.wave_id = FOURCC('W','A','V','E');
		header.format_chunk.fourcc = FOURCC('f','m','t',' ');
		header.format_chunk.len = sizeof(header.format);
		header.format.format_tag = 1;
		header.format.channels = 2;
		header.format.samples_per_sec = 48000;
		header.format.avg_bytes_per_sec = 48000 * 4;
		header.format.block_align = 4;
		header.format.bits_per_sample = 16;
		header.data_chunk.fourcc = FOURCC('d','a','t','a');
		header.data_chunk.len = fRecSize;
		fRecFile.Seek(0, SEEK_SET);
		fRecFile.Write(&header, sizeof(header));
	
		fRecFile.SetSize(fRecSize + sizeof(header));	//	We reserve space; make sure we cut off any excess at the end.
		AddSoundItem(fRecEntry, true);
	}
	else {
		fRecEntry.Remove();
	}
	//	We're done for this time.
	fRecEntry.Unset();
	//	Close the file.
	fRecFile.Unset();
	//	No more recording going on.
	fRecLimit = 0;
	fRecSize = 0;
	SetButtonState(btnPaused);
	fRecordButton->SetStopped();
	
	return B_OK;
}


status_t
RecorderWindow::StopPlaying()
{
	if (fPlayer) {
		fPlayer->Stop();
		fPlayer->SetCallbacks(0, 0, 0);
		fVolumeSlider->SetSoundPlayer(NULL);
		delete fPlayer;
		fPlayer = NULL;
	}
	SetButtonState(btnPaused);
	fPlayButton->SetStopped();
	fTrackSlider->ResetMainTime();
	fScopeView->SetMainTime(*fTrackSlider->MainTime());
	return B_OK;	
}


void
RecorderWindow::SetButtonState(BtnState state)
{
	fButtonState = state;
	UpdateButtons();
}


void
RecorderWindow::UpdateButtons()
{
	bool hasSelection = (fSoundList->CurrentSelection() >= 0);
	fRecordButton->SetEnabled(fButtonState != btnPlaying);
	fPlayButton->SetEnabled((fButtonState != btnRecording) && hasSelection);
	fRewindButton->SetEnabled((fButtonState != btnRecording) && hasSelection);
	fForwardButton->SetEnabled((fButtonState != btnRecording) && hasSelection);
	fStopButton->SetEnabled(fButtonState != btnPaused);
	fSaveButton->SetEnabled(hasSelection && (fButtonState != btnRecording));
	fLengthControl->SetEnabled(fButtonState != btnRecording);
	fInputField->SetEnabled(fButtonState != btnRecording);
}

#ifndef __HAIKU__
extern "C" status_t DecodedFormat__11BMediaTrackP12media_format(BMediaTrack *self, media_format *inout_format);
#endif

void 
RecorderWindow::UpdatePlayFile()
{
	//	Get selection.
	int32 selIdx = fSoundList->CurrentSelection();
	SoundListItem* pItem = dynamic_cast<SoundListItem*>(fSoundList->ItemAt(selIdx));
	if (! pItem) {
		return;
	}
	
	if (fPlayTrack && fPlayFile)
		fPlayFile->ReleaseTrack(fPlayTrack);
	if (fPlayFile)
		delete fPlayFile;
	fPlayTrack = NULL;
	fPlayFile = NULL;

	status_t err;
	BEntry& entry = pItem->Entry();
	entry_ref ref;
	entry.GetRef(&ref);
	fPlayFile = new BMediaFile(&ref); //, B_MEDIA_FILE_UNBUFFERED);
	if ((err = fPlayFile->InitCheck()) < B_OK) {
		ErrorAlert("get the file to play", err);
		delete fPlayFile;
		return;
	}
		
	for (int ix=0; ix<fPlayFile->CountTracks(); ix++) {
		BMediaTrack * track = fPlayFile->TrackAt(ix);
		fPlayFormat.type = B_MEDIA_RAW_AUDIO;
#ifdef __HAIKU__
		if ((track->DecodedFormat(&fPlayFormat) == B_OK) 
#else
		if ((DecodedFormat__11BMediaTrackP12media_format(track, &fPlayFormat) == B_OK)
#endif
			&& (fPlayFormat.type == B_MEDIA_RAW_AUDIO)) {
			fPlayTrack = track;
			break;
		}
		if (track)
			fPlayFile->ReleaseTrack(track);
	}
	
	if (!fPlayTrack) {
		ErrorAlert("get the file to play", err);
		delete fPlayFile;
		return;
	}
	
	BString filename = "File Name: ";
	filename << ref.name;
	fFilename->SetText(filename.String());

	BString format = "Format: ";
	media_file_format file_format;
	if (fPlayFile->GetFileFormatInfo(&file_format) == B_OK)
		format << file_format.short_name;
	BString compression = "Compression: ";
	media_codec_info codec_info;
	if (fPlayTrack->GetCodecInfo(&codec_info) == B_OK) {
		if (strcmp(codec_info.short_name, "raw")==0)
			compression << "None";
		else
			compression << codec_info.short_name;
	}
	BString channels = "Channels: ";
	channels << fPlayFormat.u.raw_audio.channel_count;
	BString samplesize = "Sample Size: ";
	samplesize << 8 * (fPlayFormat.u.raw_audio.format & 0xf) << " bits";
	BString samplerate = "Sample Rate: ";
	samplerate << (int)fPlayFormat.u.raw_audio.frame_rate;
	BString durationString = "Duration: ";
	bigtime_t duration = fPlayTrack->Duration();
	durationString << (float)(duration / 1000000.0) << " seconds"; 
	
	fFormat->SetText(format.String());
	fCompression->SetText(compression.String());
	fChannels->SetText(channels.String());
	fSampleSize->SetText(samplesize.String());
	fSampleRate->SetText(samplerate.String());
	fDuration->SetText(durationString.String());
	
	fTrackSlider->SetTotalTime(duration, true);
	fScopeView->SetMainTime(fTrackSlider->LeftTime());
	fScopeView->SetTotalTime(duration);
	fScopeView->SetRightTime(fTrackSlider->RightTime());
	fScopeView->SetLeftTime(fTrackSlider->LeftTime());
	fScopeView->RenderTrack(fPlayTrack, fPlayFormat);
	
	fPlayFrames = fPlayTrack->CountFrames();
}


void
RecorderWindow::ErrorAlert(const char * action, status_t err)
{
	char msg[300];
	sprintf(msg, "Cannot %s: %s. [%lx]", action, strerror(err), (int32) err);
	(new BAlert("", msg, "Stop"))->Go();
}


status_t
RecorderWindow::NewTempName(char * name)
{
	int init_count = fTempCount;
again:
	if (fTempCount-init_count > 25) {
		return B_ERROR;
	}
	else {
		fTempCount++;
		if (fTempCount==0)
			sprintf(name, "Audio Clip");
		else
			sprintf(name, "Audio Clip %d", fTempCount);
		BPath path;
		status_t err;
		BEntry tempEnt;
		if ((err = fTempDir.GetEntry(&tempEnt)) < B_OK) {
			return err;
		}
		if ((err = tempEnt.GetPath(&path)) < B_OK) {
			return err;
		}
		path.Append(name);
		int fd;
		//	Use O_EXCL so we know we created the file (sync with other instances)
		if ((fd = open(path.Path(), O_RDWR | O_CREAT | O_EXCL, 0666)) < 0) {
			goto again;
		}
		close(fd);
	}
	return B_OK;
}


void
RecorderWindow::AddSoundItem(const BEntry& entry, bool temp)
{
	//	Create list item to display.
	SoundListItem * listItem = new SoundListItem(entry, temp);
	fSoundList->AddItem(listItem);
	fSoundList->Invalidate();
	fSoundList->Select(fSoundList->IndexOf(listItem));
}

void
RecorderWindow::RecordFile(void * cookie, bigtime_t timestamp, void * data, size_t size, const media_raw_audio_format & format)
{
	//	Callback called from the SoundConsumer when receiving buffers.
	assert((format.format & 0x02) && format.channel_count);
	RecorderWindow * window = (RecorderWindow *)cookie;
	
	if (window->fRecording) {
		//	Write the data to file (we don't buffer or guard file access
		//	or anything)
		if (window->fRecSize < window->fRecLimit) {
			window->fRecFile.WriteAt(window->fRecSize, data, size);
			window->fVUView->ComputeNextLevel(data, size);
			window->fRecSize += size;
		} else {
			// We're done!
			window->PostMessage(STOP_RECORDING);
		}
	}
}


void
RecorderWindow::NotifyRecordFile(void * cookie, int32 code, ...)
{
	if ((code == B_WILL_STOP) || (code == B_NODE_DIES)) {
		RecorderWindow * window = (RecorderWindow *)cookie;
		// Tell the window we've stopped, if it doesn't
		// already know.
		window->PostMessage(STOP_RECORDING);
	}
}


void
RecorderWindow::PlayFile(void * cookie, void * data, size_t size, const media_raw_audio_format & format)
{	
	//	Callback called from the SoundProducer when producing buffers.
	RecorderWindow * window = (RecorderWindow *)cookie;
	int32 frame_size = (window->fPlayFormat.u.raw_audio.format & 0xf) *
		window->fPlayFormat.u.raw_audio.channel_count;
	
	if ((window->fPlayFrame < window->fPlayLimit) || window->fLooping) {
		if (window->fPlayFrame >= window->fPlayLimit) {
			bigtime_t left = window->fTrackSlider->LeftTime();
			window->fPlayTrack->SeekToTime(&left);
			window->fPlayFrame = window->fPlayTrack->CurrentFrame();
		}
		int64 frames = 0;
		window->fPlayTrack->ReadFrames(data, &frames);
		window->fVUView->ComputeNextLevel(data, size/frame_size);
		window->fPlayFrame += size/frame_size;
		window->PostMessage(UPDATE_TRACKSLIDER);
	} else {
		//	we're done!
		window->PostMessage(STOP_PLAYING);
	}
}

void
RecorderWindow::NotifyPlayFile(void * cookie, BSoundPlayer::sound_player_notification code, ...)
{
	if ((code == BSoundPlayer::B_STOPPED) || (code == BSoundPlayer::B_SOUND_DONE)) {
		RecorderWindow * window = (RecorderWindow *)cookie;
		// tell the window we've stopped, if it doesn't
		// already know.
		window->PostMessage(STOP_PLAYING);
	}
}


void 
RecorderWindow::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		
		BEntry entry(&ref, true);
		BPath path(&entry);
		BNode node(&entry);
		
		if (node.IsFile()) {
			AddSoundItem(entry, false);
		} else if(node.IsDirectory()) {
			
		}
	}
}
