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
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS),
		fCDQuery("freedb.freedb.org")
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fDiscID=-1;
	fVolume=255;
	fUseTrackNames=false;
	
	BuildGUI();
	
	if(fCDDrive.CountDrives()<1)
	{
		BAlert *alert = new BAlert("CDPlayer","It appears that there are no CD drives on your"
									"computer or there is no system software to support one."
									" Sorry.","OK");
		alert->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	fWindowState=fCDDrive.GetState();
	fVolumeSlider->SetValue(fCDDrive.GetVolume());
	WatchCDState();
}

CDPlayer::~CDPlayer()
{
	fCDDrive.Stop();
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
	fCurrentTrack->SetHighColor(fStopColor);
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
	
	fVolumeSlider = new BSlider( BRect(0,0,75,30), "VolumeSlider", "Volume", new BMessage(M_SET_VOLUME),0,255);
	fVolumeSlider->MoveTo(5, Bounds().bottom - 10 - fVolumeSlider->Frame().Height());
	AddChild(fVolumeSlider);
	
	fStop = new DrawButton( BRect(0,0,1,1), "Stop", BTranslationUtils::GetBitmap(B_PNG_FORMAT,"stop_up"),
							BTranslationUtils::GetBitmap(B_PNG_FORMAT,"stop_down"), new BMessage(M_STOP), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fStop->ResizeToPreferred();
	fStop->MoveTo(fVolumeSlider->Frame().right + 10, Bounds().bottom - 5 - fStop->Frame().Height());
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
			fCDDrive.SetVolume(fVolumeSlider->Value());
			break;
		}
		case M_STOP:
		{
			fWindowState=kStopped;
			fCDDrive.Stop();
			break;
		}
		case M_PLAY:
		{
			// If we are currently playing, then we will be showing
			// the pause images and will want to switch back to the play images	
			if(fWindowState==kPlaying)
			{
				fWindowState=kPaused;
				fCDDrive.Pause();
			}
			else
			if(fWindowState==kPaused)
			{
				fWindowState=kPlaying;
				fCDDrive.Resume();
			}
			else
			{
				fWindowState=kPlaying;
				fCDDrive.Play(fPlayList.GetCurrentTrack());
			}
			break;
		}
		case M_SELECT_TRACK:
		{
			fPlayList.SetCurrentTrack(fTrackMenu->Value()+1);
			fWindowState=kPlaying;
			fCDDrive.Play(fPlayList.GetCurrentTrack());
			break;
		}
		case M_EJECT:
		{
			fCDDrive.Eject();
			break;
		}
		case M_NEXT_TRACK:
		{
			int16 next = fPlayList.GetNextTrack();
			if(next <= 0)
			{
				// force a "wrap around" when possible. This makes it
				// possible for the user to be able to, for example, jump
				// back to the first track from the last one with 1 button push
				next = fPlayList.GetFirstTrack();
			}
			
			if(next > 0)
			{
				CDState state = fCDDrive.GetState();
				if(state == kPlaying)
					fCDDrive.Play(next);
				else
				if(state == kPaused)
				{
					fCDDrive.Play(next);
					fCDDrive.Pause();
				}
				else
					fPlayList.SetCurrentTrack(next);
				
				// Force an update for better responsiveness
				WatchCDState();
			}
			break;
		}
		case M_PREV_TRACK:
		{
			int16 prev = fPlayList.GetPreviousTrack();
			if(prev <= 0)
			{
				// force a "wrap around" when possible. This makes it
				// possible for the user to be able to, for example, jump
				// back to the first track from the last one with 1 button push
				prev = fPlayList.GetLastTrack();
			}
			
			if(prev > 0)
			{
				CDState state = fCDDrive.GetState();
				if(state == kPlaying)
					fCDDrive.Play(prev);
				else
				if(state == kPaused)
				{
					fCDDrive.Play(prev);
					fCDDrive.Pause();
				}
				else
					fPlayList.SetCurrentTrack(prev);
				
				// Force an update for better responsiveness
				WatchCDState();
			}
			break;
		}
		case M_FFWD:
		{
			if(fFastFwd->Value() == B_CONTROL_ON)
				fCDDrive.StartFastFwd();
			else
				fCDDrive.StopFastFwd();
			break;
		}
		case M_REWIND:
		{
			if(fRewind->Value() == B_CONTROL_ON)
				fCDDrive.StartRewind();
			else
				fCDDrive.StopRewind();
			break;
		}
		case M_SAVE:
		{
			// TODO: Implement
			break;
		}
		
		case M_SHUFFLE:
		{
			if(fPlayList.IsShuffled())
			{
				int16 track = fPlayList.GetCurrentTrack();
				fPlayList.SetShuffle(false);
				fPlayList.SetStartingTrack(track);
				fPlayList.SetTrackCount(fCDDrive.CountTracks());
				fShuffle->SetState(0);
			}
			else
			{
				fPlayList.SetTrackCount(fCDDrive.CountTracks());
				fPlayList.SetShuffle(true);
					fShuffle->SetState(1);
			}
			break;
		}
		case M_REPEAT:
		{
			if(fPlayList.IsLoop())
			{
				fPlayList.SetLoop(false);
				fRepeat->SetState(0);
			}
			else
			{
				fPlayList.SetLoop(true);
				fRepeat->SetState(1);
			}
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
			break;		
		}
	}
}

void
CDPlayer::AdjustButtonStates(void)
{
/*	CDState state = gCDDevice.GetState();
	
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
*/
}

void
CDPlayer::UpdateTimeInfo(void)
{
/*
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
*/
}

void 
CDPlayer::AttachedToWindow()
{
	fVolumeSlider->SetTarget(this);
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
	WatchCDState();
}

void CDPlayer::WatchCDState(void)
{
	// One watcher function to rule them all
	
	// first, watch the one setting independent of having a CD: volume
	uint8 drivevolume = fCDDrive.GetVolume();
	if(fVolume == drivevolume)
	{
		fVolume=drivevolume;
		fVolumeSlider->SetValue(fVolume);
	}
	
	// Second, establish whether or not we have a CD in the drive
	CDState playstate = fCDDrive.GetState();
	
	if(playstate == kNoCD)
	{
		// Yes, we have no bananas!
		
		if(fWindowState != kNoCD)
		{
			// We have just discovered that we have no bananas
			fWindowState = kNoCD;
			
			// Because we are changing play states, we will need to update the GUI
			
			fDiscID=-1;
			fCDTitle->SetText("No CD");
			
			fCurrentTrack->SetText("");
			fCurrentTrack->SetHighColor(fStopColor);
			fCurrentTrack->Invalidate();
			fTrackMenu->SetItemCount(0);
			fTrackTime->SetText("Track --:-- / --:--");
			fDiscTime->SetText("Disc --:-- / --:--");
			fPlayList.SetTrackCount(0);
			fPlayList.SetStartingTrack(1);
			fPlayList.SetCurrentTrack(1);
			
			if(fPlay->GetState()==1)
				fPlay->SetState(0);
		}
		else
		{
			// No change in the app's play state, so do nothing
		}
		
		return;
	}
	
//------------------------------------------------------------------------------------------------
	// Now otherwise handle the play state
	if(playstate == kStopped)
	{
		if(fWindowState == kPlaying)
		{
			// This means that the drive finished playing the song, so get the next one
			// from the list and play it
			int16 next = fPlayList.GetNextTrack();
			if(next > 0)
				fCDDrive.Play(next);
		}
		
		if(fPlay->GetState()==1)
		{
			fPlay->SetState(0);
			fCurrentTrack->SetHighColor(fStopColor);
			fCurrentTrack->Invalidate();
		}
	}
	else
	if(playstate == kPlaying)
	{
		if(fPlay->GetState()==0)
			fPlay->SetState(1);
		fCurrentTrack->SetHighColor(fPlayColor);
		fCurrentTrack->Invalidate();
	}
	else
	if(playstate == kPaused)
	{
		fPlay->SetState(0);
	}
	
//------------------------------------------------------------------------------------------------
	// If we got this far, then there must be a CD in the drive. The next order on the agenda
	// is to find out which CD it is
	int32 discid = fCDDrive.GetDiscID();
	bool update_track_gui=false;
	
	if(discid != fDiscID)
	{
		update_track_gui = true;
		
		// Apparently the disc has changed since we last looked.
		if(fCDQuery.CurrentDiscID()!=discid)
		{
			fCDQuery.SetToCD(fCDDrive.GetDrivePath());
		}
		
		if(fCDQuery.Ready())
		{
			fDiscID = discid;
			
			// Note that we only update the CD title for now. We still need a track number
			// in order to update the display for the selected track
			if(fCDQuery.GetTitles(&fCDName, &fTrackNames, 1000000))
			{
				fCDTitle->SetText(fCDName.String());
				fUseTrackNames=true;
			}
			else
			{
				fCDName="Audio CD";
				fCDTitle->SetText("Audio CD");
				fUseTrackNames=false;
			}
		}
	}
	
//------------------------------------------------------------------------------------------------
	// Now that we know which CD it is, update the track info
	int16 drivecount = fCDDrive.CountTracks();
	int16 drivetrack = fCDDrive.GetTrack();
	
	int16 playlisttrack = fPlayList.GetCurrentTrack();
	int16 playlistcount = fPlayList.TrackCount();
	
	if(playstate == kPlaying)
	{
		// The main thing is that we need to make sure that the playlist and the drive's track
		// stay in sync. The CD's track may have been changed by an outside source, so if
		// the drive is playing, check for playlist sync.
		if(playlisttrack != drivetrack)
		{
			playlisttrack = drivetrack;
			fPlayList.SetTrackCount(drivecount);
			fPlayList.SetCurrentTrack(drivetrack);
		}
		update_track_gui=true;
	}
	else
	{
		if(playlistcount != drivecount)
		{
			// This happens only when CDs are changed
			if(drivecount<0)
			{
				// There is no CD in the drive. The playlist needs to have its track
				// count set to 0 and it also needs to be rewound.
				fPlayList.SetStartingTrack(1);
				fPlayList.SetTrackCount(0);
				playlisttrack=1;
				playlistcount=0;
			}
			else
			{
				// Two possible cases here: playlist is empty or playlist has a different
				// number of tracks. In either case, the playlist needs to be reinitialized
				// to the current track data
				fPlayList.SetStartingTrack(1);
				fPlayList.SetTrackCount(drivecount);
				playlisttrack=fPlayList.GetCurrentTrack();
				playlistcount=drivecount;
			}
		}
		else
		{
			// CD has not changed, so check for change in tracks
			if(playlisttrack != drivetrack)
			{
				update_track_gui=true;
			}
			else
			{
				// do nothing. Everything is hunky-dory
			}
		}
	}
	
	if(update_track_gui)
	{
		BString currentTrackName;
		
		if(playlisttrack >= 0)
		{
			int16 whichtrack = playlisttrack;
			
			if(whichtrack == 0)
				whichtrack++;
			
			if(fUseTrackNames && fTrackNames.size()>0)
				currentTrackName << "Track " << whichtrack << ": " << fTrackNames[ whichtrack - 1];
			else
				currentTrackName << "Track " << whichtrack;
			
			fCurrentTrack->SetText(currentTrackName.String());
			
			fTrackMenu->SetItemCount(playlistcount);
			fTrackMenu->SetValue(playlisttrack-1);
		}
		else
		{
			fCurrentTrack->SetText("");
			fTrackMenu->SetItemCount(0);
			fTrackMenu->SetValue(1);
		}
	}
	
//------------------------------------------------------------------------------------------------
	// Now update the time info
	cdaudio_time tracktime;
	cdaudio_time disctime;
	cdaudio_time tracktotal;
	cdaudio_time disctotal;
	char timestring[1024];
	
	if(fCDDrive.GetTime(tracktime, disctime))
	{
		fCDDrive.GetTimeForDisc(disctotal);
		sprintf(timestring,"Disc %ld:%.2ld / %ld:%.2ld",disctime.minutes,disctime.seconds,
														disctotal.minutes,disctotal.seconds);
		fDiscTime->SetText(timestring);
		
		fCDDrive.GetTimeForTrack(playlisttrack,tracktotal);
		sprintf(timestring,"Track %ld:%.2ld / %ld:%.2ld",tracktime.minutes,tracktime.seconds,
														tracktotal.minutes,tracktotal.seconds);
		fTrackTime->SetText(timestring);
	}
	else
	{
		fTrackTime->SetText("Track --:-- / --:--");
		fDiscTime->SetText("Disc --:-- / --:--");
	}
}

class CDPlayerWindow : public BWindow
{
public:
	CDPlayerWindow(void);
	bool QuitRequested(void);
};

CDPlayerWindow::CDPlayerWindow(void)
 :	BWindow(BRect (100, 100, 610, 200), "CD Player", B_TITLED_WINDOW, B_NOT_V_RESIZABLE |
	B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
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
