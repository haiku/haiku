/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */


#include "ButtonBitmaps.h"
#include "CDPlayer.h"
#include "DrawButton.h"
#include "DoubleShotDrawButton.h"
#include "TwoStateDrawButton.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h> 
#include <Debug.h>
#include <Deskbar.h>
#include <Dragger.h>
#include <Entry.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <TranslationUtils.h>
#include <Window.h>

#include <stdlib.h>
#include <string.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "CDPlayer"

enum {
	M_STOP = 'mstp',
	M_PLAY,
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


class CDPlayerWindow : public BWindow {
	public:
		CDPlayerWindow();
		bool QuitRequested();
};


inline void
SetLabel(BStringView *label, const char *text)
{
	if (strcmp(label->Text(), text) != 0)
		label->SetText(text);
}


//	#pragma mark -


CDPlayer::CDPlayer(BRect frame, const char *name, uint32 resizeMask,
		uint32 flags)
	: BView(frame, name, resizeMask, flags | B_FRAME_EVENTS),
 	fCDQuery("freedb.freedb.org")
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fVolume = 255;

	BuildGUI();

	if (fCDDrive.CountDrives() < 1) {
		BAlert *alert = new BAlert("CDPlayer", B_TRANSLATE(
			"It appears that there are no CD"
			" drives on your computer or there is no system software to "
			"support one. Sorry."), B_TRANSLATE("OK"));
		alert->Go();
	}

	fWindowState = fCDDrive.GetState();
	fVolumeSlider->SetValue(fCDDrive.GetVolume());
	if (fVolumeSlider->Value() <= 2) {
		fCDDrive.SetVolume(255);
		fVolumeSlider->SetValue(255);
	}
	_WatchCDState();
}


CDPlayer::~CDPlayer()
{
	fCDDrive.Stop();
}


bool
CDPlayer::InitCheck()
{
	return fCDDrive.CountDrives() > 0;
}


void
CDPlayer::BuildGUI()
{
	fStopColor.red = 80;
	fStopColor.green = 164;
	fStopColor.blue = 80;
	fStopColor.alpha = 255;

	fPlayColor.red = 40;
	fPlayColor.green = 230;
	fPlayColor.blue = 40;
	fPlayColor.alpha = 255;

	BRect r(Bounds().InsetByCopy(10, 10));
	BBox *box = new BBox(r, "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(box);

	r = box->Bounds().InsetByCopy(10, 10);
	r.bottom = 25;

	float labelWidth, labelHeight;
	fCDTitle = new BStringView(r, "CDTitle", B_TRANSLATE("CD drive is empty"),
		B_FOLLOW_LEFT_RIGHT);
	fCDTitle->GetPreferredSize(&labelWidth, &labelHeight);
	fCDTitle->ResizeTo(r.Width(), labelHeight);
	box->AddChild(fCDTitle);

	r.bottom = r.top + labelHeight;
	r.OffsetBy(0, r.Height() + 5);

	fCurrentTrack = new BStringView(r, "TrackNumber",
		 "", B_FOLLOW_LEFT_RIGHT);
	box->AddChild(fCurrentTrack);

	r.OffsetBy(0, r.Height() + 5);
	r.right = r.left + (r.Width() / 2);
	fTrackTime = new BStringView(r, "TrackTime",
		 B_TRANSLATE("Track: 88:88 / 88:88"), B_FOLLOW_LEFT_RIGHT);
	fTrackTime->ResizeToPreferred();
	fTrackTime->SetText(B_TRANSLATE("Track: --:-- / --:--"));
	box->AddChild(fTrackTime);

	r.OffsetTo(fTrackTime->Frame().right + 5, r.top);
	fDiscTime = new BStringView(r, "DiscTime",
		B_TRANSLATE("Disc: 88:88 / 88:88"), B_FOLLOW_RIGHT);
	fDiscTime->ResizeToPreferred();
	fDiscTime->SetText(B_TRANSLATE("Disc: --:-- / --:--"));
	box->AddChild(fDiscTime);

	float maxWidth = max_c(fDiscTime->Frame().right, fCDTitle->Frame().right);

	box->ResizeTo(maxWidth + 5, fDiscTime->Frame().bottom + 10);

	fStop = new DrawButton(BRect(0, 0, 1, 1), "Stop",
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "stop_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "stop_down"),
		new BMessage(M_STOP), B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fStop->ResizeToPreferred();
	fStop->MoveTo(10, box->Frame().bottom + 15);
	fStop->SetDisabled(BTranslationUtils::GetBitmap(B_PNG_FORMAT,
		"stop_disabled"));
	AddChild(fStop);
	float stopTop = fStop->Frame().top;


	fPlay = new TwoStateDrawButton(BRect(0, 0, 1, 1), "Play", 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_down"), 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_up_on"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_down"), 
		new BMessage(M_PLAY), B_FOLLOW_NONE, B_WILL_DRAW);

	fPlay->ResizeToPreferred();
	fPlay->MoveTo(fStop->Frame().right + 2, stopTop);
	AddChild(fPlay);

	fPrevTrack = new DrawButton(BRect(0, 0, 1, 1), "PrevTrack",
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "prev_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "prev_down"),
		new BMessage(M_PREV_TRACK), 0, B_WILL_DRAW);
	fPrevTrack->ResizeToPreferred();
	fPrevTrack->MoveTo(fPlay->Frame().right + 40, stopTop);
	AddChild(fPrevTrack);

	fNextTrack = new DrawButton(BRect(0, 0, 1, 1), "NextTrack",
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "next_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "next_down"),
		new BMessage(M_NEXT_TRACK), 0, B_WILL_DRAW);
	fNextTrack->ResizeToPreferred();
	fNextTrack->MoveTo(fPrevTrack->Frame().right + 1, stopTop);
	AddChild(fNextTrack);

	fRewind = new DoubleShotDrawButton(BRect(0, 0, 1, 1), "Rewind", 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "rew_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "rew_down"),
		new BMessage(M_REWIND), 0, B_WILL_DRAW);
	fRewind->ResizeToPreferred();
	fRewind->MoveTo(fNextTrack->Frame().right + 40, stopTop);
	AddChild(fRewind);

	fFastFwd = new DoubleShotDrawButton(BRect(0, 0, 1, 1), "FastFwd", 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "ffwd_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "ffwd_down"),
		new BMessage(M_FFWD), 0, B_WILL_DRAW);
	fFastFwd->ResizeToPreferred();
	fFastFwd->MoveTo(fRewind->Frame().right + 1, stopTop);
	AddChild(fFastFwd);

	r.left = 10;
	r.right = fPlay->Frame().right;
	r.top = fStop->Frame().bottom + 14;
	r.bottom = r.top + 14;
	fVolumeSlider = new VolumeSlider(r, "VolumeSlider",
		0, 255, new BMessage(M_SET_VOLUME), this);
	AddChild(fVolumeSlider);

	fRepeat = new TwoStateDrawButton(BRect(0, 0, 1, 1), "Repeat", 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "repeat_up_off"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "repeat_down"), 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "repeat_up_on"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "repeat_down"), 
		new BMessage(M_REPEAT), B_FOLLOW_NONE, B_WILL_DRAW);
	fRepeat->ResizeToPreferred();
	fRepeat->MoveTo(fPrevTrack->Frame().left, fVolumeSlider->Frame().top - 
		((fRepeat->Frame().Height() - fVolumeSlider->Frame().Height()) / 2));
	AddChild(fRepeat);

	fShuffle = new TwoStateDrawButton(BRect(0, 0, 1, 1), "Shuffle", 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "shuffle_up_off"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "shuffle_down"), 
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "shuffle_up_on"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "shuffle_down"), 
		new BMessage(M_SHUFFLE), B_FOLLOW_NONE, B_WILL_DRAW);
	fShuffle->ResizeToPreferred();
	fShuffle->MoveTo(fRepeat->Frame().right + 2, fRepeat->Frame().top);
	AddChild(fShuffle);

	fEject = new DrawButton(BRect(0, 0, 1, 1), "Eject",
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "eject_up"),
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "eject_down"),
		new BMessage(M_EJECT), 0, B_WILL_DRAW);
	fEject->ResizeToPreferred();
	fEject->MoveTo(fFastFwd->Frame().left, fShuffle->Frame().top);
	AddChild(fEject);

	ResizeTo(box->Frame().right + 10, fVolumeSlider->Frame().bottom + 10);	
}


void
CDPlayer::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_SET_VOLUME:
			fCDDrive.SetVolume(fVolumeSlider->Value());
			break;

		case M_STOP:
			if (fWindowState == kPaused) {
				fPlay->SetBitmaps(0, BTranslationUtils::GetBitmap(B_PNG_FORMAT,
					"play_up"),
					BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_down"));
				fPlay->SetState(1);
			}
			fWindowState = kStopped;
			fCDDrive.Stop();
			break;

		case M_PLAY:
			// If we are currently playing, then we will be showing
			// the pause images and will want to switch back to the play images	
			if (fWindowState == kPlaying) {
				fWindowState = kPaused;
				fCDDrive.Pause();
				fPlay->SetBitmaps(0, BTranslationUtils::GetBitmap(B_PNG_FORMAT,
					"paused_up"),
					BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_down"));
			} else if (fWindowState == kPaused) {
				fWindowState = kPlaying;
				fCDDrive.Resume();
				fPlay->SetBitmaps(0, BTranslationUtils::GetBitmap(B_PNG_FORMAT,
					"play_up"),
					BTranslationUtils::GetBitmap(B_PNG_FORMAT, "play_down"));
			} else {
				fWindowState = kPlaying;
				fCDDrive.Play(fPlayList.GetCurrentTrack());
			}
			break;

		case M_EJECT:
			fCDDrive.Eject();
			break;

		case M_NEXT_TRACK:
		{
			int16 next = fPlayList.GetNextTrack();
			if (next <= 0) {
				// force a "wrap around" when possible. This makes it
				// possible for the user to be able to, for example, jump
				// back to the first track from the last one with 1 button push
				next = fPlayList.GetFirstTrack();
			}

			if (next > 0) {
				CDState state = fCDDrive.GetState();
				if (state == kPlaying) {
					while (!fCDDrive.Play(next)) {
						next = fPlayList.GetNextTrack();
						if (next < 1) {
							fWindowState = kStopped;
							fCDDrive.Stop();
							fPlayList.Rewind();
							break;
						}
					}
				}
				else if (state == kPaused) {
					fCDDrive.Play(next);
					fCDDrive.Pause();
				} 
				else
					fPlayList.SetCurrentTrack(next);
				
				// Force an update for better responsiveness
				_WatchCDState();
			}
			break;
		}

		case M_PREV_TRACK:
		{
			int16 prev = fPlayList.GetPreviousTrack();
			if (prev <= 0) {
				// force a "wrap around" when possible. This makes it
				// possible for the user to be able to, for example, jump
				// back to the first track from the last one with 1 button push
				prev = fPlayList.GetLastTrack();
			}

			if (prev > 0) {
				CDState state = fCDDrive.GetState();
				if (state == kPlaying) {
					while (!fCDDrive.Play(prev)) {
						prev = fPlayList.GetPreviousTrack();
						if (prev < 1) {
							fWindowState = kStopped;
							fCDDrive.Stop();
							fPlayList.Rewind();
							break;
						}
					}
				} else if (state == kPaused) {
					fCDDrive.Play(prev);
					fCDDrive.Pause();
				} else
					fPlayList.SetCurrentTrack(prev);
				
				// Force an update for better responsiveness
				_WatchCDState();
			}
			break;
		}

		case M_FFWD:
			if (fFastFwd->Value() == B_CONTROL_ON)
				fCDDrive.StartFastFwd();
			else
				fCDDrive.StopFastFwd();
			break;

		case M_REWIND:
			if (fRewind->Value() == B_CONTROL_ON)
				fCDDrive.StartRewind();
			else
				fCDDrive.StopRewind();
			break;

		case M_SHUFFLE:
			if (fPlayList.IsShuffled()) {
				int16 track = fPlayList.GetCurrentTrack();
				fPlayList.SetShuffle(false);
				fPlayList.SetStartingTrack(track);
				fPlayList.SetTrackCount(fCDDrive.CountTracks());
				fShuffle->SetState(0);
			} else {
				fPlayList.SetTrackCount(fCDDrive.CountTracks());
				fPlayList.SetShuffle(true);
					fShuffle->SetState(1);
			}
			break;

		case M_REPEAT:
			if (fPlayList.IsLoop()) {
				fPlayList.SetLoop(false);
				fRepeat->SetState(0);
			} else {
				fPlayList.SetLoop(true);
				fRepeat->SetState(1);
			}
			break;

		default:
			BView::MessageReceived(msg);
			break;		

	}
}


void 
CDPlayer::AttachedToWindow()
{
	fStop->SetTarget(this);
	fPlay->SetTarget(this);
	fNextTrack->SetTarget(this);
	fPrevTrack->SetTarget(this);
	fFastFwd->SetTarget(this);
	fRewind->SetTarget(this);
	fEject->SetTarget(this);
	fShuffle->SetTarget(this);
	fRepeat->SetTarget(this);
}


void 
CDPlayer::Pulse()
{
	_WatchCDState();
}


void
CDPlayer::_WatchCDState()
{
	// One watcher function to rule them all
	
	// first, watch the one setting independent of having a CD: volume
	uint8 driveVolume = fCDDrive.GetVolume();
	if (fVolume == driveVolume) {
		fVolume = driveVolume;
		fVolumeSlider->SetValue(fVolume);
	}

	// Second, establish whether or not we have a CD in the drive
	CDState playState = fCDDrive.GetState();
	bool internalTrackChange = false;

	if (playState == kNoCD) {
		// Yes, we have no bananas!

		// Do something only if there is a change in the app's play state
		if (fWindowState != kNoCD) {
			// We have just discovered that we have no bananas
			fWindowState = kNoCD;

			// Because we are changing play states, we will need to update
			// the GUI
			fCDData.SetDiscID(-1);
			SetLabel(fCDTitle, B_TRANSLATE("CD drive is empty"));

			SetLabel(fCurrentTrack, "");
			SetLabel(fTrackTime, B_TRANSLATE("Track: --:-- / --:--"));
			SetLabel(fDiscTime, B_TRANSLATE("Disc: --:-- / --:--"));
			fPlayList.SetTrackCount(0);
			fPlayList.SetStartingTrack(1);
			fPlayList.SetCurrentTrack(1);

			if (fPlay->GetState() == 1)
				fPlay->SetState(0);
		}

		return;
	}

	// Now otherwise handle the play state
	if (playState == kStopped) {
		if (fWindowState == kPlaying) {
			internalTrackChange = true;

			// This means that the drive finished playing the song, so get
			// the next one from the list and play it
			int16 next = fPlayList.GetNextTrack();
			if (next > 0)
				fCDDrive.Play(next);
		}

		if (fPlay->GetState() == 1)
			fPlay->SetState(0);
	} else if (playState == kPlaying) {
		if (fPlay->GetState() == 0)
			fPlay->SetState(1);
	} else if (playState == kPaused)
		fPlay->SetState(0);

	// If we got this far, then there must be a CD in the drive. The next order
	// on the agenda is to find out which CD it is
	int32 discId = fCDDrive.GetDiscID();
	bool updateTrackGui = false;

	if (discId != fCDData.DiscID()) {
		updateTrackGui = true;

		// Apparently the disc has changed since we last looked.
		if (fCDQuery.CurrentDiscID() != discId)
			fCDQuery.SetToCD(fCDDrive.GetDrivePath());

		if (fCDQuery.Ready()) {
			// Note that we only update the CD title for now. We still need a
			// track number in order to update the display for the selected track
			if (fCDQuery.GetData(&fCDData, 1000000)) {
				BString display(fCDData.Artist());
				display << " - " << fCDData.Album();
				SetLabel(fCDTitle, display.String());
			} else
				SetLabel(fCDTitle, B_TRANSLATE("Audio CD"));
		}
	}

	// Now that we know which CD it is, update the track info
	int16 driveCount = fCDDrive.CountTracks();
	int16 driveTrack = fCDDrive.GetTrack();

	int16 playlistTrack = fPlayList.GetCurrentTrack();
	int16 playlistCount = fPlayList.TrackCount();

	if (playState == kPlaying) {
		if (playlistTrack != driveTrack) {
			playlistTrack = driveTrack;

			if (!internalTrackChange) {
				// The main thing is that we need to make sure that the
				// playlist and the drive's track stay in sync. The CD's
				// track may have been changed by an outside source, so if
				// the drive is playing, check for playlist sync.
				fPlayList.SetTrackCount(driveCount);
				fPlayList.SetCurrentTrack(driveTrack);
			}
		}
		updateTrackGui = true;
	} else {
		if (playlistCount != driveCount) {
			// This happens only when CDs are changed
			if (driveCount < 0) {
				// There is no CD in the drive. The playlist needs to have its
				// track count set to 0 and it also needs to be rewound.
				fPlayList.SetStartingTrack(1);
				fPlayList.SetTrackCount(0);
				playlistTrack = 1;
				playlistCount = 0;
			} else {
				// Two possible cases here: playlist is empty or playlist has a
				// different number of tracks. In either case, the playlist
				// needs to be reinitialized to the current track data
				fPlayList.SetStartingTrack(1);
				fPlayList.SetTrackCount(driveCount);
				playlistTrack = fPlayList.GetCurrentTrack();
				playlistCount = driveCount;
			}
		} else {
			// update only with a track change
			if (playlistTrack != driveTrack)
				updateTrackGui = true;
		}
	}

	if (updateTrackGui) {
		BString currentTrackName(B_TRANSLATE("Track %trackAt%: %whichTrack%"));

		if (playlistTrack >= 0) {
			int16 whichTrack = playlistTrack;

			if (whichTrack == 0)
				whichTrack++;

			BString stringTrack;
			stringTrack << whichTrack;

			BString stringTrackAt;
			stringTrack << fCDData.TrackAt(whichTrack - 1);

			currentTrackName.ReplaceFirst("%whichTrack%", stringTrack);
			currentTrackName.ReplaceFirst("%trackAt%", stringTrackAt);

			SetLabel(fCurrentTrack, currentTrackName.String());

		} else
			SetLabel(fCurrentTrack, "");
	}

	// Now update the time info
	CDAudioTime trackTime;
	CDAudioTime discTime;
	CDAudioTime trackTotal;
	CDAudioTime discTotal;
	char timeString[1024];

	if (fCDDrive.GetTime(trackTime, discTime)) {
		fCDDrive.GetTimeForDisc(discTotal);
		sprintf(timeString, B_TRANSLATE("Disc: %ld:%.2ld / %ld:%.2ld"),
			discTime.GetMinutes(), discTime.GetSeconds(),
			discTotal.GetMinutes(), discTotal.GetSeconds());
		SetLabel(fDiscTime, timeString);

		fCDDrive.GetTimeForTrack(playlistTrack, trackTotal);
		sprintf(timeString, B_TRANSLATE("Track: %ld:%.2ld / %ld:%.2ld"),
			trackTime.GetMinutes(), trackTime.GetSeconds(),
			trackTotal.GetMinutes(), trackTotal.GetSeconds());
		SetLabel(fTrackTime, timeString);
	} else {
		SetLabel(fTrackTime, B_TRANSLATE("Track: --:-- / --:--"));
		SetLabel(fDiscTime, B_TRANSLATE("Disc: --:-- / --:--"));
	}
}


//	#pragma mark -


CDPlayerWindow::CDPlayerWindow()
	:
	BWindow(BRect (100, 100, 405, 280), B_TRANSLATE_SYSTEM_NAME("CDPlayer"), 
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_ASYNCHRONOUS_CONTROLS)
{
}


bool
CDPlayerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


CDPlayerApplication::CDPlayerApplication()
	: BApplication("application/x-vnd.Haiku-CDPlayer")
{
	BWindow *window = new CDPlayerWindow();
	CDPlayer *view = new CDPlayer(window->Bounds(), B_TRANSLATE("CD"));
	if (view->InitCheck()) {
		window->ResizeTo(view->Bounds().Width(), view->Bounds().Height());
		window->AddChild(view);
		window->Show();
	} else 
		PostMessage(B_QUIT_REQUESTED);
}


int
main(int, char **argv)
{
	CDPlayerApplication app;
	app.Run();

	return 0;
}

