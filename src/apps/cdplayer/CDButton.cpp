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

#include "CDButton.h"
#include "DrawButton.h"
#include <TranslationUtils.h>

enum
{
	M_STOP='mstp',
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

CDButton::CDButton(BRect frame, const char *name, uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	engine = new CDEngine(CDEngine::FindCDPlayerDevice());
	
	BuildGUI();
	
	fCDState = engine->PlayStateWatcher()->GetState();
	
	if(fCDState == kNoCD)
	{
		fStop->SetEnabled(false);
		fPlay->SetEnabled(false);
		fNextTrack->SetEnabled(false);
		fPrevTrack->SetEnabled(false);
		fFastFwd->SetEnabled(false);
		fRewind->SetEnabled(false);
		fSave->SetEnabled(false);
	}
}

CDButton::~CDButton()
{
	delete engine;
}

void CDButton::BuildGUI(void)
{
	fCDTitle = new BTextControl( BRect(5,5,230,30), "CDTitle","","",new BMessage(M_SET_CD_TITLE));
	AddChild(fCDTitle);
	fCDTitle->SetDivider(0);
	fCDTitle->TextView()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BBox *box = new BBox(BRect(0,0,75,20),"TrackBox");
	box->MoveTo(fCDTitle->Frame().right + 15, 5);
	AddChild(box);
	
	BView *view = new BView( box->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_NONE,B_WILL_DRAW);
	view->SetViewColor(0,0,0);
	box->AddChild(view);
	
	fTrackNumber = new BStringView( view->Bounds(),"TrackNumber","Track: --");
	view->AddChild(fTrackNumber);
	fTrackNumber->SetAlignment(B_ALIGN_CENTER);
	fTrackNumber->SetHighColor(0,255,0);
	fTrackNumber->SetFont(be_bold_font);
	
	fVolume = new BSlider( BRect(0,0,75,30), "VolumeSlider", "Volume", new BMessage(M_SET_VOLUME),0,100);
	fVolume->MoveTo(5, Bounds().bottom - 10 - fVolume->Frame().Height());
	AddChild(fVolume);
	fVolume->SetEnabled(false);
	
	fStop = new DrawButton( BRect(0,0,1,1), "Stop", BTranslationUtils::GetBitmap('PNG ',"stop_up"),
							BTranslationUtils::GetBitmap('PNG ',"stop_down"), new BMessage(M_STOP), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fStop->ResizeToPreferred();
	fStop->MoveTo(fVolume->Frame().right + 10, Bounds().bottom - 5 - fStop->Frame().Height());
	fStop->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"stop_disabled"));
	AddChild(fStop);
	
	// TODO: Play is a special button. Implement as two-state buttons
	fPlay = new DrawButton( BRect(0,0,1,1), "Play", BTranslationUtils::GetBitmap('PNG ',"play_up"),
							BTranslationUtils::GetBitmap('PNG ',"play_down"), new BMessage(M_PLAY), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fPlay->ResizeToPreferred();
	fPlay->MoveTo(fStop->Frame().right + 2, Bounds().bottom - 5 - fPlay->Frame().Height());
	fPlay->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"play_disabled"));
	AddChild(fPlay);
	
	fPrevTrack = new DrawButton( BRect(0,0,1,1), "PrevTrack", BTranslationUtils::GetBitmap('PNG ',"prev_up"),
							BTranslationUtils::GetBitmap('PNG ',"prev_down"), new BMessage(M_PREV_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fPrevTrack->ResizeToPreferred();
	fPrevTrack->MoveTo(fPlay->Frame().right + 10, Bounds().bottom - 5 - fPrevTrack->Frame().Height());
	fPrevTrack->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"prev_disabled"));
	AddChild(fPrevTrack);
	
	fNextTrack = new DrawButton( BRect(0,0,1,1), "NextTrack", BTranslationUtils::GetBitmap('PNG ',"next_up"),
							BTranslationUtils::GetBitmap('PNG ',"next_down"), new BMessage(M_NEXT_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fNextTrack->ResizeToPreferred();
	fNextTrack->MoveTo(fPrevTrack->Frame().right + 2, Bounds().bottom - 5 - fNextTrack->Frame().Height());
	fNextTrack->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"next_disabled"));
	AddChild(fNextTrack);
	
	// TODO: Fast Forward and Rewind are special buttons. Implement as two-state buttons
	fRewind = new DrawButton( BRect(0,0,1,1), "Rewind", BTranslationUtils::GetBitmap('PNG ',"rew_up"),
							BTranslationUtils::GetBitmap('PNG ',"rew_down"), new BMessage(M_PREV_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fRewind->ResizeToPreferred();
	fRewind->MoveTo(fNextTrack->Frame().right + 10, Bounds().bottom - 5 - fRewind->Frame().Height());
	fRewind->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"rew_disabled"));
	AddChild(fRewind);
	
	fFastFwd = new DrawButton( BRect(0,0,1,1), "FastFwd", BTranslationUtils::GetBitmap('PNG ',"ffwd_up"),
							BTranslationUtils::GetBitmap('PNG ',"ffwd_down"), new BMessage(M_NEXT_TRACK), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fFastFwd->ResizeToPreferred();
	fFastFwd->MoveTo(fRewind->Frame().right + 2, Bounds().bottom - 5 - fFastFwd->Frame().Height());
	fFastFwd->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"ffwd_disabled"));
	AddChild(fFastFwd);
	
	fEject = new DrawButton( BRect(0,0,1,1), "Eject", BTranslationUtils::GetBitmap('PNG ',"eject_up"),
							BTranslationUtils::GetBitmap('PNG ',"eject_down"), new BMessage(M_EJECT), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fEject->ResizeToPreferred();
	fEject->MoveTo(fFastFwd->Frame().right + 20, Bounds().bottom - 5 - fEject->Frame().Height());
	fEject->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"eject_disabled"));
	AddChild(fEject);
	
	fSave = new DrawButton( BRect(0,0,1,1), "Save", BTranslationUtils::GetBitmap('PNG ',"save_up"),
							BTranslationUtils::GetBitmap('PNG ',"save_down"), new BMessage(M_SAVE), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fSave->ResizeToPreferred();
	fSave->MoveTo(fEject->Frame().right + 20, Bounds().bottom - 5 - fSave->Frame().Height());
	fSave->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"save_disabled"));
	AddChild(fSave);
	fSave->SetEnabled(false);
	
	// TODO: Shuffle and Repeat are special buttons. Implement as two-state buttons
	fShuffle = new DrawButton( BRect(0,0,1,1), "Shuffle", BTranslationUtils::GetBitmap('PNG ',"shuffle_up"),
							BTranslationUtils::GetBitmap('PNG ',"shuffle_down"), new BMessage(M_SHUFFLE), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fShuffle->ResizeToPreferred();
	fShuffle->MoveTo(fSave->Frame().right + 2, Bounds().bottom - 5 - fShuffle->Frame().Height());
	fShuffle->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"shuffle_disabled"));
	AddChild(fShuffle);
	fShuffle->SetEnabled(false);
	
	fRepeat = new DrawButton( BRect(0,0,1,1), "Repeat", BTranslationUtils::GetBitmap('PNG ',"repeat_up"),
							BTranslationUtils::GetBitmap('PNG ',"repeat_down"), new BMessage(M_REPEAT), 
							B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fRepeat->ResizeToPreferred();
	fRepeat->MoveTo(fShuffle->Frame().right + 2, Bounds().bottom - 5 - fRepeat->Frame().Height());
	fRepeat->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"repeat_disabled"));
	AddChild(fRepeat);
	fRepeat->SetEnabled(false);
}


void
CDButton::MessageReceived(BMessage *message)
{
	switch (message->what) 
	{
		case M_SET_VOLUME:
		{
			// TODO: Implement
			break;
		}
		case M_STOP:
		{
			engine->Stop();
			break;
		}
		case M_PLAY:
		{
			engine->PlayOrPause();
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
		case M_FFWD:
		{
			// TODO: Implement
			break;
		}
		case M_REWIND:
		{
			// TODO: Implement
			break;
		}
		case M_EJECT:
		{
			engine->Eject();
			break;
		}
		case M_SAVE:
		{
			// TODO: Implement
			break;
		}
		case M_SHUFFLE:
		{
			// TODO: Implement
			break;
		}
		case M_REPEAT:
		{
			// TODO: Implement
			break;
		}
		default:
		{
			
			if (!Observer::HandleObservingMessages(message))
			{
				// just support observing messages
				BView::MessageReceived(message);
				break;		
			}
		}
	}
}

void
CDButton::NoticeChange(Notifier *notifier)
{
//	debugger("");
	
	PlayState *ps;
	TrackState *trs;
	TimeState *tms;
	CDContentWatcher *ccw;
	
	ps = dynamic_cast<PlayState *>(notifier);
	trs = dynamic_cast<TrackState *>(notifier);
	tms = dynamic_cast<TimeState *>(notifier);
	ccw = dynamic_cast<CDContentWatcher *>(notifier);
	
	if(ps)
	{
		switch(ps->GetState())
		{
			case kNoCD:
			{
				if(fStop->IsEnabled())
				{
					fStop->SetEnabled(false);
					fPlay->SetEnabled(false);
					fNextTrack->SetEnabled(false);
					fPrevTrack->SetEnabled(false);
					fFastFwd->SetEnabled(false);
					fRewind->SetEnabled(false);
					fSave->SetEnabled(false);
				}
				break;
			}
			case kStopped:
			{
				if(!fStop->IsEnabled())
				{
					fStop->SetEnabled(true);
					fPlay->SetEnabled(true);
					fNextTrack->SetEnabled(true);
					fPrevTrack->SetEnabled(true);
					fFastFwd->SetEnabled(true);
					fRewind->SetEnabled(true);
					fSave->SetEnabled(true);
				}
				break;
			}
			case kPaused:
			{
				// TODO: Set play button to Pause
				if(!fStop->IsEnabled())
				{
					fStop->SetEnabled(true);
					fPlay->SetEnabled(true);
					fNextTrack->SetEnabled(true);
					fPrevTrack->SetEnabled(true);
					fFastFwd->SetEnabled(true);
					fRewind->SetEnabled(true);
					fSave->SetEnabled(true);
				}
				UpdateTrackInfo();
				break;
			}
			case kPlaying:
			{
				if(!fStop->IsEnabled())
				{
					fStop->SetEnabled(true);
					fPlay->SetEnabled(true);
					fNextTrack->SetEnabled(true);
					fPrevTrack->SetEnabled(true);
					fFastFwd->SetEnabled(true);
					fRewind->SetEnabled(true);
					fSave->SetEnabled(true);
				}
				UpdateTrackInfo();
				break;
			}
			case kSkipping:
			{
				if(!fStop->IsEnabled())
				{
					fStop->SetEnabled(true);
					fPlay->SetEnabled(true);
					fNextTrack->SetEnabled(true);
					fPrevTrack->SetEnabled(true);
					fFastFwd->SetEnabled(true);
					fRewind->SetEnabled(true);
					fSave->SetEnabled(true);
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else
	if(trs)
	{
		UpdateTrackInfo();
		// TODO: Update track count indicator
	}
	else
	if(tms)
	{
	}
	else
	if(ccw)
	{
	}
}

void
CDButton::UpdateTrackInfo(void)
{

	fCurrentTrack=engine->TrackStateWatcher()->GetTrack();
	fTrackCount=engine->TrackStateWatcher()->GetNumTracks();
	
	if(fCurrentTrack < 1)
	{
		char string[255];
		sprintf(string,"Track: %ld",fCurrentTrack);
		fTrackNumber->SetText(string);
	}
	else
		fTrackNumber->SetText("Track: --");
}

void 
CDButton::AttachedToWindow()
{
	// start observing
	engine->AttachedToLooper(Window());
	StartObserving(engine->TrackStateWatcher());
	StartObserving(engine->PlayStateWatcher());
	
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
}

void 
CDButton::Pulse()
{
	engine->DoPulse();
}

class CDButtonWindow : public BWindow
{
public:
	CDButtonWindow(void);
	bool QuitRequested(void);
};

CDButtonWindow::CDButtonWindow(void)
 : BWindow(BRect (100, 100, 610, 400), "CD Player", B_TITLED_WINDOW, 0)
{
}

bool CDButtonWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

CDButtonApplication::CDButtonApplication()
	:	BApplication("application/x-vnd.Be-CDPlayer")
{
	BWindow *window = new CDButtonWindow();
	
	BView *button = new CDButton(window->Bounds(), "CD");
	window->AddChild(button);
	window->Show();
}


int
main(int, char **argv)
{
	(new CDButtonApplication())->Run();

	return 0;
}
