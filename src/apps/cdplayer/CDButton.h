/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
// This defines the replicant button

#ifndef __CD_BUTTON__
#define __CD_BUTTON__

#include <Application.h>
#include <View.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <Slider.h>
#include <TextControl.h>
#include <StringView.h>

#include "PlayList.h"
#include "Observer.h"
#include "CDEngine.h"

class DrawButton;
class TwoStateDrawButton;

class CDButton : public BView, private Observer 
{
public:
						CDButton(BRect frame, const char *name,	
								uint32 resizeMask = B_FOLLOW_ALL,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE | 
												B_PULSE_NEEDED);
	virtual 			~CDButton();
	
			void		BuildGUI(void);
	
	virtual	void		AttachedToWindow();	
	virtual void 		Pulse();
	virtual	void 		FrameResized(float new_width, float new_height);
	virtual void	 	MessageReceived(BMessage *);

	// observing overrides
	virtual BHandler	*RecipientHandler() const
		{ return (BHandler *)this; }
	
	virtual void		NoticeChange(Notifier *);

private:
			void		UpdateCDInfo(void);
			void		UpdateTimeInfo(void);
			void		SetBitmap(BBitmap *bitmap);
	
	CDEngine	*engine;
	
	DrawButton *fStop, *fPlay, *fNextTrack, *fPrevTrack;
	DrawButton *fFastFwd, *fRewind, *fEject, *fSave, *fShuffle, *fRepeat;
	
	
	BSlider *fVolume;
	
	BStringView *fCDTitle;
	BStringView *fCurrentTrack;
	BStringView *fTrackTime;
	BStringView *fDiscTime;
	
	BBox *fCDBox;
	BBox *fTrackBox;
	BBox *fTimeBox;
	
	CDState fCDState;
	PlayList fPlaylist;
};


class CDButtonApplication : public BApplication 
{
public:
	CDButtonApplication();
};


#endif