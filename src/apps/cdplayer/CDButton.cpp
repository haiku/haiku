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
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>

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
	:	BView(frame, name, resizeMask, flags | B_FRAME_EVENTS)
{
	// This will eventually one of a few preferences - to stop playing music
	// when the app is closed
	fStopOnQuit = false;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// TODO: Support multiple CD drives
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
	if(fStopOnQuit)
		engine->Stop();
	
	delete engine;
}

void CDButton::BuildGUI(void)
{
	BRect r(5,5,230,25);
	
	// Assemble the CD Title box
	fCDBox = new BBox(r,"TrackBox");
	AddChild(fCDBox);
	
	BView *view = new BView( fCDBox->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(20,20,20);
	fCDBox->AddChild(view);
	
	fCDTitle = new BStringView(view->Bounds(),"CDTitle","");
	view->AddChild(fCDTitle);
	fCDTitle->SetHighColor(200,200,200);
	fCDTitle->SetFont(be_bold_font);
	
	r.Set(fCDTitle->Frame().right + 15,5,Bounds().right - 5,25);
	fTrackBox = new BBox(r,"TrackBox",B_FOLLOW_TOP);
	AddChild(fTrackBox);

	view = new BView( fTrackBox->Bounds().InsetByCopy(2,2), "view",B_FOLLOW_ALL,B_WILL_DRAW);
	view->SetViewColor(0,34,7);
	fTrackBox->AddChild(view);
	
	fCurrentTrack = new BStringView( view->Bounds(),"TrackNumber","Track:",B_FOLLOW_ALL);
	view->AddChild(fCurrentTrack);
	fCurrentTrack->SetHighColor(40,230,40);
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
							B_FOLLOW_NONE, B_WILL_DRAW);
	fSave->ResizeToPreferred();
	fSave->MoveTo(fEject->Frame().right + 20, Bounds().bottom - 5 - fSave->Frame().Height());
	fSave->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"save_disabled"));
	AddChild(fSave);
	fSave->SetEnabled(false);
	
	// TODO: Shuffle and Repeat are special buttons. Implement as two-state buttons
	fShuffle = new DrawButton( BRect(0,0,1,1), "Shuffle", BTranslationUtils::GetBitmap('PNG ',"shuffle_up"),
							BTranslationUtils::GetBitmap('PNG ',"shuffle_down"), new BMessage(M_SHUFFLE), 
							B_FOLLOW_NONE, B_WILL_DRAW);
	fShuffle->ResizeToPreferred();
	fShuffle->MoveTo(fSave->Frame().right + 2, Bounds().bottom - 5 - fShuffle->Frame().Height());
	fShuffle->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"shuffle_disabled"));
	AddChild(fShuffle);
	fShuffle->SetEnabled(false);
	
	fRepeat = new DrawButton( BRect(0,0,1,1), "Repeat", BTranslationUtils::GetBitmap('PNG ',"repeat_up"),
							BTranslationUtils::GetBitmap('PNG ',"repeat_down"), new BMessage(M_REPEAT), 
							B_FOLLOW_NONE, B_WILL_DRAW);
	fRepeat->ResizeToPreferred();
	fRepeat->MoveTo(fShuffle->Frame().right + 2, Bounds().bottom - 5 - fRepeat->Frame().Height());
	fRepeat->SetDisabled(BTranslationUtils::GetBitmap('PNG ',"repeat_disabled"));
	AddChild(fRepeat);
	fRepeat->SetEnabled(false);
}


void
CDButton::MessageReceived(BMessage *msg)
{
	if(msg->WasDropped())
	{
		// We'll handle two types of drops: those from Tracker and those from ShowImage
		if(msg->what==B_SIMPLE_DATA)
		{
			int32 actions;
			if(msg->FindInt32("be:actions",&actions)==B_OK)
			{
				// ShowImage drop. This is a negotiated drag&drop, so send a reply
				BMessage reply(B_COPY_TARGET), response;
				reply.AddString("be:types","image/jpeg");
				reply.AddString("be:types","image/png");
				
				msg->SendReply(&reply,&response);
				
				// now, we've gotten the response
				if(response.what==B_MIME_DATA)
				{
					// Obtain and translate the received data
					uint8 *imagedata;
					ssize_t datasize;
										
					// Try JPEG first
					if(response.FindData("image/jpeg",B_MIME_DATA,(const void **)&imagedata,&datasize)!=B_OK)
					{
						// Try PNG next and piddle out if unsuccessful
						if(response.FindData("image/png",B_PNG_FORMAT,(const void **)&imagedata,&datasize)!=B_OK)
							return;
					}
					
					// Set up to decode into memory
					BMemoryIO memio(imagedata,datasize);
					BTranslatorRoster *roster=BTranslatorRoster::Default();
					BBitmapStream bstream;
					
					if(roster->Translate(&memio,NULL,NULL,&bstream, B_TRANSLATOR_BITMAP)==B_OK)
					{
						BBitmap *bmp;
						if(bstream.DetachBitmap(&bmp)!=B_OK)
							return;
						
						SetBitmap(bmp);
					}
				}
				return;
			}
			
			entry_ref ref;
			if(msg->FindRef("refs",&ref)==B_OK)
			{
				// Tracker drop
				BBitmap *bmp=BTranslationUtils::GetBitmap(&ref);
				SetBitmap(bmp);
			}
		}
		return;
	}
	
	switch (msg->what) 
	{
		case M_SET_VOLUME:
		{
			// TODO: Implement
			engine->SetVolume(fVolume->Value());
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
CDButton::NoticeChange(Notifier *notifier)
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
				fCurrentTrack->SetText("Track: ");
				fTrackTime->SetText("Track --:-- / --:--");
				fDiscTime->SetText("Disc --:-- / --:--");
				printf("Notification: No CD\n");
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
				fCurrentTrack->SetText("Track: ");
				fTrackTime->SetText("Track --:-- / --:--");
				fDiscTime->SetText("Disc --:-- / --:--");
				printf("Notification: CD Stopped\n");
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
				printf("Notification: CD Paused\n");
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
				printf("Notification: CD Playing\n");
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
				printf("Notification: CD Skipping\n");
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
		UpdateCDInfo();
		
		// TODO: Update track count indicator
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
CDButton::UpdateCDInfo(void)
{
	BString CDName, currentTrackName;
	vector<BString> trackNames;
	
	int32 currentTrack = engine->TrackStateWatcher()->GetTrack();
	engine->ContentWatcher()->GetContent(&CDName,&trackNames);
	
	fCDTitle->SetText(CDName.String());
	
	if(currentTrack > 0)
	{
		currentTrackName << "Track " << currentTrack << ": " << trackNames[ currentTrack - 1];
		fCurrentTrack->SetText(currentTrackName.String());
	}
	else
	{
		fCurrentTrack->SetText("Track:");
	}
	
}

void
CDButton::UpdateTimeInfo(void)
{
	int32 min,sec;
	char string[1024];
	
	engine->TimeStateWatcher()->GetDiscTime(min,sec);
	if(min >= 0)
		sprintf(string,"Disc %ld:%.2ld / --:--",min,sec);
	else
		sprintf(string,"Disc --:-- / --:--");
	fDiscTime->SetText(string);
	
	engine->TimeStateWatcher()->GetTrackTime(min,sec);
	if(min >= 0)
		sprintf(string,"Track %ld:%.2ld / --:--",min,sec);
	else
		sprintf(string,"Track --:-- / --:--");
	fTrackTime->SetText(string);
}

void 
CDButton::AttachedToWindow()
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
}

void
CDButton::FrameResized(float new_width, float new_height)
{
	// We implement this method because there is no resizing mode to split the window's
	// width into two and have a box fill each half
	
	// The boxes are laid out with 5 pixels between the window edge and each box.
	// Additionally, 15 pixels of padding are between the two boxes themselves
	
	float half = (new_width / 2);
	
	fCDBox->ResizeTo( half - 7, fCDBox->Bounds().Height() );
	
	fTrackBox->MoveTo(half + 8, fTrackBox->Frame().top);
	fTrackBox->ResizeTo( Bounds().right - (half + 8) - 5, fTrackBox->Bounds().Height() );

	fTimeBox->MoveTo(half + 8, fTimeBox->Frame().top);
	fTimeBox->ResizeTo( Bounds().right - (half + 8) - 5, fTimeBox->Bounds().Height() );
	
	fRepeat->MoveTo(new_width - fRepeat->Bounds().right - 5, fRepeat->Frame().top);
	
	fShuffle->MoveTo(fRepeat->Frame().left - fShuffle->Bounds().Width() - 2, fShuffle->Frame().top);
	fSave->MoveTo(fShuffle->Frame().left - fSave->Bounds().Width() - 2, fSave->Frame().top);
}

void 
CDButton::Pulse()
{
	engine->DoPulse();
}

void
CDButton::SetBitmap(BBitmap *bitmap)
{
	if(!bitmap)
	{
		ClearViewBitmap();
		return;
	}
	
	SetViewBitmap(bitmap);
	fVolume->SetViewBitmap(bitmap);
	fVolume->Invalidate();
	Invalidate();
}

class CDButtonWindow : public BWindow
{
public:
	CDButtonWindow(void);
	bool QuitRequested(void);
};

CDButtonWindow::CDButtonWindow(void)
 : BWindow(BRect (100, 100, 610, 200), "CD Player", B_TITLED_WINDOW, B_NOT_V_RESIZABLE)
{
	float wmin,wmax,hmin,hmax;
	
	GetSizeLimits(&wmin,&wmax,&hmin,&hmax);
	wmin=510;
	hmin=100;
	SetSizeLimits(wmin,wmax,hmin,hmax);
}

bool CDButtonWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

CDButtonWindow *cdbwin;
CDButton *cdbutton;

CDButtonApplication::CDButtonApplication()
	:	BApplication("application/x-vnd.Haiku-CDPlayer")
{
//	BWindow *window = new CDButtonWindow();
	
//	BView *button = new CDButton(window->Bounds(), "CD");
///	window->AddChild(button);
//	window->Show();
	cdbwin = new CDButtonWindow();

	cdbutton = new CDButton(cdbwin->Bounds(), "CD");
	cdbwin->AddChild(cdbutton);
	cdbwin->Show();
}

int
main(int, char **argv)
{
	(new CDButtonApplication())->Run();

	return 0;
}
