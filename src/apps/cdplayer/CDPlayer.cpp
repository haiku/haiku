// Copyright 1992-1999, Be Incorporated, All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
//
// send comments/suggestions/feedback to pavel@be.com
//

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Entry.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include <stdlib.h>
#include <string.h>

#include "CDPlayer.h"
#include "DrawButton.h"
#include "DoubleShotDrawButton.h"
#include "TwoStateDrawButton.h"
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>

enum
{
	M_STOP='mstp',
	M_PLAY,
	M_SELECT_TRACK,
	M_NEXT_TRACK,
	M_PREV_TRACK,
	M_FFWD,
	M_REWIND,
	M_EJECT,
	M_SAVE,
	M_SHUFFLE,
	M_REPEAT,
	M_SET_VOLUME,
	M_SET_CD_TITLE
};

CDPlayer::CDPlayer(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// TODO: Support multiple CD drives
	engine = new CDEngine;
	
	BuildGUI();
	
	CDAudioDevice cd;
	if(cd.CountDrives()<1)
	{
		BAlert *alert = new BAlert("CDPlayer","It appears that there are no CD drives on your"
									"computer or there is no system software to support one."
									" Sorry.","OK");
		alert->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
}

CDPlayer::~CDPlayer()
{
	engine->Stop();
	delete engine;
}

void CDPlayer::BuildGUI(void)
{
	fStopColor.red = 80;
	fStopColor.green = 164;
	fStopColor.blue = 80;
	fStopColor.alpha = 255;
	
	fPlayColor.red = 40;
	fPlayColor.green = 230;
	fPlayColor.blue = 40;
	fPlayColor.alpha = 255;
	
	BRect r;
	r.left = 5;
	r.top = 5;
	r.right = (Bounds().Width()/2) - 7;
	r.bottom = 25;
	
	
	// Assemble the CD Title box
	fCDBox = new BBox(r,"TrackBox");
	AddChild(fCDBox);
	
	BView *view = new BView( fCDBox->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(20,20,20);
	fCDBox->AddChild(view);
	
	fCDTitle = new BStringView(view->Bounds(),"CDTitle","", B_FOLLOW_ALL);
	view->AddChild(fCDTitle);
	fCDTitle->SetHighColor(200,200,200);
	fCDTitle->SetFont(be_bold_font);
	
	r.OffsetBy(0, r.Height() + 5);
	fTrackMenu = new TrackMenu(r, "TrackMenu", new BMessage(M_SELECT_TRACK));
	AddChild(fTrackMenu);
	
	r.Set(fCDTitle->Frame().right + 15,5,Bounds().right - 5,25);
	fTrackBox = new BBox(r,"TrackBox",B_FOLLOW_TOP);
	AddChild(fTrackBox);

	view = new BView( fTrackBox->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(0,34,7);
	fTrackBox->AddChild(view);
	
	fCurrentTrack = new BStringView( view->Bounds(),"TrackNumber","Track:",B_FOLLOW_ALL);
	view->AddChild(fCurrentTrack);
	fCurrentTrack->SetHighColor(fPlayColor);
	fCurrentTrack->SetFont(be_bold_font);
	
	r.OffsetBy(0, r.Height() + 5);
	fTimeBox = new BBox(r,"TimeBox",B_FOLLOW_LEFT_RIGHT);
	AddChild(fTimeBox);
	
	view = new BView( fTimeBox->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(0,7,34);
	fTimeBox->AddChild(view);
	
	r = view->Bounds();
	r.right /= 2;
	
	fTrackTime = new BStringView(r,"TrackTime","Track --:-- / --:--",B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fTrackTime);
	fTrackTime->SetHighColor(120,120,255);
	fTrackTime->SetFont(be_bold_font);
	
	r.right = view->Bounds().right;
	r.left = fTrackTime->Frame().right + 1;
	
	fDiscTime = new BStringView(r,"DiscTime","Disc --:-- / --:--",B_FOLLOW_RIGHT);
	view->AddChild(fDiscTime);
	fDiscTime->SetHighColor(120,120,255);
	fDiscTime->SetFont(be_bold_font);
	
	fVolume = new BSlider( BRect(0,0,75,30), "VolumeSlider", "Volume", new BMessage(M_SET_VOLUME),0,255);
	fVolume->MoveTo(5, Bounds().bottom - 10 - fVolume->Frame().Height());
	AddChild(fVolume);
	
	fStop = new DrawButton( BRect(0,0,1,1), "Stop", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"stop_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"stop_down"), new BMessage(M_STOP), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fStop->ResizeToPreferred();
	fStop->MoveTo(fVolume->Frame().right + 10, Bounds().bottom - 5 - fStop->Frame().Height());
	fStop->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"stop_disabled"));
	AddChild(fStop);
	
	fPlay = new TwoStateDrawButton( BRect(0,0,1,1), "Play", 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"play_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"play_down"), 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"pause_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"pause_down"), 
							new BMessage(M_PLAY), B_FOLLOW_NONE, B_WILL_DRAW);

	fPlay->ResizeToPreferred();
	fPlay->MoveTo(fStop->Frame().right + 2, Bounds().bottom - 5 - fPlay->Frame().Height());
	fPlay->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"play_disabled"),
						BTranslationUtils::GetBitmap(B_PNG_FORMAT,"pause_disabled"));
	AddChild(fPlay);
	
	fPrevTrack = new DrawButton( BRect(0,0,1,1), "PrevTrack", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"prev_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"prev_down"), new BMessage(M_PREV_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fPrevTrack->ResizeToPreferred();
	fPrevTrack->MoveTo(fPlay->Frame().right + 10, Bounds().bottom - 5 - fPrevTrack->Frame().Height());
	fPrevTrack->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"prev_disabled"));
	AddChild(fPrevTrack);
	
	fNextTrack = new DrawButton( BRect(0,0,1,1), "NextTrack", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"next_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"next_down"), new BMessage(M_NEXT_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fNextTrack->ResizeToPreferred();
	fNextTrack->MoveTo(fPrevTrack->Frame().right + 2, Bounds().bottom - 5 - fNextTrack->Frame().Height());
	fNextTrack->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"next_disabled"));
	AddChild(fNextTrack);
	
	fRewind = new DoubleShotDrawButton( BRect(0,0,1,1), "Rewind", 
										BTranslationUtils::GetBitmap(B_PNG_FORMAT,"rew_up"),
										BTranslationUtils::GetBitmap(B_PNG_FORMAT,"rew_down"),
										new BMessage(M_REWIND), B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fRewind->ResizeToPreferred();
	fRewind->MoveTo(fNextTrack->Frame().right + 10, Bounds().bottom - 5 - fRewind->Frame().Height());
	fRewind->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"rew_disabled"));
	AddChild(fRewind);
	
	fFastFwd = new DoubleShotDrawButton( BRect(0,0,1,1), "FastFwd", 
										BTranslationUtils::GetBitmap(B_PNG_FORMAT,"ffwd_up"),
										BTranslationUtils::GetBitmap(B_PNG_FORMAT,"ffwd_down"),
										new BMessage(M_FFWD), B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fFastFwd->ResizeToPreferred();
	fFastFwd->MoveTo(fRewind->Frame().right + 2, Bounds().bottom - 5 - fFastFwd->Frame().Height());
	fFastFwd->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"ffwd_disabled"));
	AddChild(fFastFwd);
	
	fEject = new DrawButton( BRect(0,0,1,1), "Eject", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"eject_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"eject_down"), new BMessage(M_EJECT), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fEject->ResizeToPreferred();
	fEject->MoveTo(fFastFwd->Frame().right + 20, Bounds().bottom - 5 - fEject->Frame().Height());
	fEject->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"eject_disabled"));
	AddChild(fEject);
	
	fSave = new DrawButton( BRect(0,0,1,1), "Save", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"save_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"save_down"), new BMessage(M_SAVE), 
							B_FOLLOW_NONE, B_WILL_DRAW);
	fSave->ResizeToPreferred();
	fSave->MoveTo(fEject->Frame().right + 20, Bounds().bottom - 5 - fSave->Frame().Height());
	fSave->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,"save_disabled"));
	AddChild(fSave);
	fSave->SetEnabled(false);
	
	fShuffle = new TwoStateDrawButton( BRect(0,0,1,1), "Shuffle", 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"shuffle_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"shuffle_down"), 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"shuffle_up_on"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"shuffle_down_on"), 
							new BMessage(M_SHUFFLE), B_FOLLOW_NONE, B_WILL_DRAW);
	fShuffle->ResizeToPreferred();
	fShuffle->MoveTo(fSave->Frame().right + 2, Bounds().bottom - 5 - fShuffle->Frame().Height());
	AddChild(fShuffle);
	
	fRepeat = new TwoStateDrawButton( BRect(0,0,1,1), "Repeat", 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"repeat_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"repeat_down"), 
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"repeat_up_on"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"repeat_down_on"), 
							new BMessage(M_REPEAT), B_FOLLOW_NONE, B_WILL_DRAW);
	fRepeat->ResizeToPreferred();
	fRepeat->MoveTo(fShuffle->Frame().right + 2, Bounds().bottom - 5 - fRepeat->Frame().Height());
	AddChild(fRepeat);
}


void
CDPlayer::MessageReceived(BMessage *msg)
{
	switch (msg->what) 
	{
		case M_SET_VOLUME:
		{
			engine->SetVolume(fVolume->Value());
			break;
		}
		case M_STOP:
		{
			if(engine->GetState()==kPlaying)
				fPlay->SetState(0);
			
			engine->Stop();
			break;
		}
		case M_PLAY:
		{
			// If we are currently playing, then we will be showing
			// the pause images and will want to switch back to the play images
			if(engine->GetState()==kPlaying)
			{
				fPlay->SetState(0);
				engine->Pause();
			}
			else
			{
				fPlay->SetState(1);
				engine->Play();
			}
			break;
		}
		case M_SELECT_TRACK:
		{
			engine->SelectTrack(fTrackMenu->Value()+1);
			break;
		}
		case M_NEXT_TRACK:
		{
			engine->SkipOneForward();
			break;
		}
		case M_PREV_TRACK:
		{
			engine->SkipOneBackward();
			break;
		}
		case M_EJECT:
		{
			engine->Eject();
			break;
		}
		
		
		
		case M_FFWD:
		{
			if(fFastFwd->Value() == B_CONTROL_ON)
				engine->StartSkippingForward();
			else
				engine->StopSkipping();
			break;
		}
		case M_REWIND:
		{
			if(fRewind->Value() == B_CONTROL_ON)
				engine->StartSkippingBackward();
			else
				engine->StopSkipping();
			break;
		}
		case M_SAVE:
		{
			// TODO: Implement
			break;
		}
		
		case M_SHUFFLE:
		{
			engine->ToggleShuffle();
			
			if(engine->IsShuffled() && fShuffle->GetState()==0)
			{
				fShuffle->SetState(1);
			}
			else
			{
				if(fShuffle->GetState()==1)
				fShuffle->SetState(0);
			}
			break;
		}
		case M_REPEAT:
		{
			engine->ToggleRepeat();
			if(engine->IsRepeated() && fRepeat->GetState()==0)
			{
				fRepeat->SetState(1);
			}
			else
			{
				if(fRepeat->GetState()==1)
				fRepeat->SetState(0);
			}
			break;
		}
		
		default:
		{
			
			if (!Observer::HandleObservingMessages(msg))
			{
				// just support observing messages
				BView::MessageReceived(msg);
				break;		
			}
		}
	}
}

void
CDPlayer::NoticeChange(Notifier *notifier)
{
	PlayState *ps;
	TrackState *trs;
	TimeState *tms;
	CDContentWatcher *ccw;
	VolumeState *vs;
	
	ps = dynamic_cast<PlayState *>(notifier);
	trs = dynamic_cast<TrackState *>(notifier);
	tms = dynamic_cast<TimeState *>(notifier);
	ccw = dynamic_cast<CDContentWatcher *>(notifier);
	vs = dynamic_cast<VolumeState *>(notifier);
	
	if(ps)
	{
		AdjustButtonStates();
		HandlePlayState();
	}
	else
	if(trs)
	{

		if(fTrackMenu->CountItems() != engine->TrackStateWatcher()->GetNumTracks())
			fTrackMenu->SetItemCount(engine->TrackStateWatcher()->GetNumTracks());
		
		fTrackMenu->SetValue(engine->TrackStateWatcher()->GetTrack()-1);
		
		UpdateCDInfo();
	}
	else
	if(tms)
	{
		UpdateTimeInfo();
	}
	else
	if(ccw)
	{
		UpdateCDInfo();
	}
	else
	if(vs)
	{
		fVolume->SetValue(engine->VolumeStateWatcher()->GetVolume());
	}
}

void
CDPlayer::HandlePlayState(void)
{
	switch(engine->PlayStateWatcher()->GetState())
	{
		case kNoCD:
		{
			fCurrentTrack->SetHighColor(fStopColor);
			fCurrentTrack->Invalidate();
			break;
		}
		case kStopped:
		{
			fCurrentTrack->SetHighColor(fStopColor);
			fCurrentTrack->Invalidate();
			break;
		}
		case kPaused:
		{
			fCurrentTrack->SetHighColor(fPlayColor);
			break;
		}
		case kPlaying:
		{
			fCurrentTrack->SetHighColor(fPlayColor);
			fCurrentTrack->Invalidate();
			break;
		}
		case kSkipping:
		{
			fCurrentTrack->SetHighColor(fStopColor);
			break;
		}
		default:
		{
			break;
		}
	}
}

void
CDPlayer::AdjustButtonStates(void)
{
	CDState state = gCDDevice.GetState();
	
	if(state==kNoCD)
	{
		// Everything needs to be disabled when there is no CD
		fStop->SetEnabled(false);
		fPlay->SetEnabled(false);
		fNextTrack->SetEnabled(false);
		fPrevTrack->SetEnabled(false);
		
		fSave->SetEnabled(false);
	}
	else
	{
		fStop->SetEnabled(true);
		fPlay->SetEnabled(true);
		fNextTrack->SetEnabled(true);
		fPrevTrack->SetEnabled(true);
		
		// TODO: Enable when Save is implemented
//		fSave->SetEnabled(true);
	}
	
	if(state==kPlaying)
		fPlay->SetState(1);
	else
		fPlay->SetState(0);
}

void
CDPlayer::UpdateCDInfo(void)
{
	BString CDName, currentTrackName;
	vector<BString> trackNames;
	
	int32 currentTrack = engine->TrackStateWatcher()->GetTrack();
	
	bool trackresult = engine->ContentWatcher()->GetContent(&CDName,&trackNames);
		
	if(currentTrack < 0)
		return;
	
	if(currentTrack == 0)
		currentTrack++;
	
	if(trackresult)
	{
		if(CDName.CountChars()<1)
		{
			// if the CD name is NULL, then it means we have no disc in the drive.
			fCDTitle->SetText("");
			fCurrentTrack->SetText("");
		}
		else
		{
			currentTrackName << "Track " << currentTrack << ": " << trackNames[ currentTrack - 1];
			fCurrentTrack->SetText(currentTrackName.String());
			fCDTitle->SetText(CDName.String());
		}
		
		return;
	}
	else
	{
		fCDTitle->SetText("Audio CD");
		currentTrackName << "Track " << currentTrack;
		fCurrentTrack->SetText(currentTrackName.String());
	}
}

void
CDPlayer::UpdateTimeInfo(void)
{
	int32 min,sec;
	char string[1024];
	
	engine->TimeStateWatcher()->GetDiscTime(min,sec);
	if(min >= 0)
	{
		int32 tmin,tsec;
		engine->TimeStateWatcher()->GetTotalDiscTime(tmin,tsec);
		
		sprintf(string,"Disc %ld:%.2ld / %ld:%.2ld",min,sec,tmin,tsec);
	}
	else
		sprintf(string,"Disc --:-- / --:--");
	fDiscTime->SetText(string);
	
	engine->TimeStateWatcher()->GetTrackTime(min,sec);
	if(min >= 0)
	{
		int32 tmin,tsec;
		engine->TimeStateWatcher()->GetTotalTrackTime(tmin,tsec);
		
		sprintf(string,"Track %ld:%.2ld / %ld:%.2ld",min,sec,tmin,tsec);
	}
	else
		sprintf(string,"Track --:-- / --:--");
	fTrackTime->SetText(string);
}

void 
CDPlayer::AttachedToWindow()
{

	// start observing
	engine->AttachedToLooper(Window());
	StartObserving(engine->TrackStateWatcher());
	StartObserving(engine->PlayStateWatcher());
	StartObserving(engine->ContentWatcher());
	StartObserving(engine->TimeStateWatcher());
	StartObserving(engine->VolumeStateWatcher());
	
	fVolume->SetTarget(this);
	fStop->SetTarget(this);
	fPlay->SetTarget(this);
	fNextTrack->SetTarget(this);
	fPrevTrack->SetTarget(this);
	fFastFwd->SetTarget(this);
	fRewind->SetTarget(this);
	fEject->SetTarget(this);
	fSave->SetTarget(this);
	fShuffle->SetTarget(this);
	fRepeat->SetTarget(this);
	fTrackMenu->SetTarget(this);
}

void
CDPlayer::FrameResized(float new_width, float new_height)
{
	// We implement this method because there is no resizing mode to split the window's
	// width into two and have a box fill each half
	
	// The boxes are laid out with 5 pixels between the window edge and each box.
	// Additionally, 15 pixels of padding are between the two boxes themselves
	
	float half = (new_width / 2);
	
	fCDBox->ResizeTo( half - 7, fCDBox->Bounds().Height() );
	fTrackMenu->ResizeTo( half - 7, fTrackMenu->Bounds().Height() );
	fTrackMenu->Invalidate();
	
	fTrackBox->MoveTo(half + 8, fTrackBox->Frame().top);
	fTrackBox->ResizeTo( Bounds().right - (half + 8) - 5, fTrackBox->Bounds().Height() );

	fTimeBox->MoveTo(half + 8, fTimeBox->Frame().top);
	fTimeBox->ResizeTo( Bounds().right - (half + 8) - 5, fTimeBox->Bounds().Height() );
	
	fRepeat->MoveTo(new_width - fRepeat->Bounds().right - 5, fRepeat->Frame().top);
	
	fShuffle->MoveTo(fRepeat->Frame().left - fShuffle->Bounds().Width() - 2, fShuffle->Frame().top);
	fSave->MoveTo(fShuffle->Frame().left - fSave->Bounds().Width() - 2, fSave->Frame().top);
}

void 
CDPlayer::Pulse()
{
	engine->DoPulse();
}

class CDPlayerWindow : public BWindow
{
public:
	CDPlayerWindow(void);
	bool QuitRequested(void);
};

CDPlayerWindow::CDPlayerWindow(void)
 :	BWindow(BRect (100, 100, 610, 200), "CD Player", B_TITLED_WINDOW, B_NOT_V_RESIZABLE |
	B_NOT_ZOOMABLE)
{
	float wmin,wmax,hmin,hmax;
	
	GetSizeLimits(&wmin,&wmax,&hmin,&hmax);
	wmin=510;
	hmin=100;
	SetSizeLimits(wmin,wmax,hmin,hmax);
}

bool CDPlayerWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

CDPlayerApplication::CDPlayerApplication()
	:	BApplication("application/x-vnd.Haiku-CDPlayer")
{
	BWindow *window = new CDPlayerWindow();
	BView *button = new CDPlayer(window->Bounds(), "CD");
	window->AddChild(button);
	window->Show();
}

int
main(int, char **argv)
{
	(new CDPlayerApplication())->Run();

	return 0;
}
