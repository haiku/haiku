/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
// This defines the replicant button

#ifndef CDPLAYER_H
#define CDPLAYER_H

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
class TwoStateDrawButton;

class CDPlayer : public BView, private Observer 
{
public:
						CDPlayer(BRect frame, const char *name,	
								uint32 resizeMask = B_FOLLOW_ALL,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE | 
												B_PULSE_NEEDED);
	virtual 			~CDPlayer();
	
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
			void				HandlePlayState(void);
			void				UpdateCDInfo(void);
			void				UpdateTimeInfo(void);
	
			CDEngine			*engine;
	
			DrawButton			*fStop,
//								*fPlay,
								*fNextTrack,
								*fPrevTrack,
								*fFastFwd,
								*fRewind,
								*fEject,
								*fSave;
			
			TwoStateDrawButton	*fShuffle,
								*fRepeat,
								*fPlay;
			
			BSlider				*fVolume;
			
			BStringView			*fCDTitle,
								*fCurrentTrack,
								*fTrackTime,
								*fDiscTime;
			
			BBox				*fCDBox,
								*fTrackBox,
								*fTimeBox;
			
			CDState				fCDState;
			
			rgb_color			fStopColor;
			rgb_color			fPlayColor;
};


class CDPlayerApplication : public BApplication 
{
public:
	CDPlayerApplication();
};


#endif