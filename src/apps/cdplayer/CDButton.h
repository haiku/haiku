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

#include "Observer.h"
#include "CDEngine.h"

class DrawButton;

class CDButton : public BView, private Observer 
{
public:
	CDButton(BRect frame, const char *name,	uint32 resizeMask = B_FOLLOW_ALL, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED);
		
	virtual ~CDButton();
	
	void BuildGUI(void);
	
	// misc BView overrides
	virtual void AttachedToWindow();	
	
	virtual void Pulse();

	virtual void MessageReceived(BMessage *);

	// observing overrides
	virtual BHandler *RecipientHandler() const
		{ return (BHandler *)this; }

	virtual void NoticeChange(Notifier *);

private:
	void UpdateTrackInfo(void);
	
	CDEngine *engine;
	
	DrawButton *fStop, *fPlay, *fNextTrack, *fPrevTrack;
	DrawButton *fFastFwd, *fRewind, *fEject, *fSave, *fShuffle, *fRepeat;
	
	BSlider *fVolume;
	
	BTextControl *fCDTitle;
	BStringView *fTrackNumber;
	
	CDState fCDState;
	int32 fTrackCount, fCurrentTrack;
};


class CDButtonApplication : public BApplication 
{
public:
	CDButtonApplication();
};


#endif