/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines an engine that tracks the state of the CD player
// and supports the CD player control and status calls

#ifndef __CD_ENGINE__
#define __CD_ENGINE__

#include <Looper.h>
#include <String.h>
#include <View.h>

#include <vector>

#include "CDAudioDevice.h"
#include "Observer.h"
#include "FunctionObjectMessage.h"
#include "CDDBSupport.h"
#include "PlayList.h"

class CDEngine;

// watcher sits somewhere were it can get pulses and makes sure
// notices get sent if state changes
class PeriodicWatcher : public Notifier 
{
public:
						PeriodicWatcher(void);
	virtual				~PeriodicWatcher() {}

			void		DoPulse();
			void		UpdateNow();
			
			void		AttachedToLooper(CDEngine *engine)
						{ this->engine = engine; }
	virtual BHandler	*RecipientHandler() const;

protected:
	virtual	bool		UpdateState() = 0;
			CDEngine	*engine;
};

// this watcher sends notices to observers that are interrested
// about play state changes
class PlayState : public PeriodicWatcher 
{
public:
				PlayState(CDEngine *engine);

	CDState		GetState() const;

private:
	bool		UpdateState();
	bool		CurrentState(CDState);
	CDState		oldState;
	
	CDEngine	*fEngine;
};

// this watcher sends notices to observers that are interested
// about changes in the current track
class TrackState : public PeriodicWatcher 
{
public:
			TrackState(void);

	int32	GetTrack() const;
	int32	GetNumTracks() const;
	
private:
	bool	UpdateState();
	bool	CurrentState(int32,int32);
	int32	currentTrack;
	int32	trackCount;
};

// this watcher sends notices to observers that are interested
// about changes in the current time
class TimeState : public PeriodicWatcher 
{
public:
			TimeState(void) : PeriodicWatcher() { }

	void	GetDiscTime(int32 &minutes, int32 &seconds) const;
	void	GetTotalDiscTime(int32 &minutes, int32 &seconds) const;
	
	void	GetTrackTime(int32 &minutes, int32 &seconds) const;
	void	GetTotalTrackTime(int32 &minutes, int32 &seconds) const;

private:
	bool			UpdateState();
	bool			CurrentState(cdaudio_time tracktime, 
								cdaudio_time totaltracktime,
								cdaudio_time disctime,
								cdaudio_time totaldisctime);
	
	cdaudio_time	fDiscTime,
					fTotalDiscTime,
					fTrackTime,
					fTotalTrackTime;
};

class CDContentWatcher : public PeriodicWatcher 
{
public:
				CDContentWatcher(void);
	
	bool		GetContent(BString *title, vector<BString> *tracks);
	
private:
	bool		UpdateState();

	CDDBQuery	cddbQuery;
	int32		discID;
	bool		fReady;
};

// this watcher sends notices to observers that are interested
// about changes in the current volume
// currently not used yet
class VolumeState : public PeriodicWatcher 
{
public:
			VolumeState(void);
	
	int32	GetVolume() const;

private:
	bool	UpdateState();
	int32	fVolume;
	
};

// The CD engine defines all the different CD control calls; also,
// it hosts the different state watchers and helps them send notices
// to observers about the CD state changes
class CDEngine : public BHandler 
{
public:
					CDEngine(void);

	virtual 		~CDEngine();

			// observing support
	virtual	void	MessageReceived(BMessage *);
			void	AttachedToLooper(BLooper *);
			void	DoPulse();

			// control calls
			void	Play();
			void	Pause();
			void 	Stop();
			void 	Eject();
			void 	SkipOneForward();
			void 	SkipOneBackward();
			void 	StartSkippingBackward();
			void 	StartSkippingForward();
			void 	StopSkipping();
			void 	SelectTrack(int32);
			
			void 	SetVolume(uint8 value);
			
			void 	ToggleShuffle(void);
			void 	ToggleRepeat(void);
	
			CDState	GetState(void) const { return fEngineState; }

	// to find the current Track, you may call the GetTrack function
	// TrackState defines
	TrackState *TrackStateWatcher()
		{ return &trackState; }

	// to find the current play state, you may call the GetState function
	// PlayState defines
	PlayState *PlayStateWatcher()
		{ return &playState; }

	// to find the current location on the CD, you may call the GetTime function
	// TimeState defines
	TimeState *TimeStateWatcher()
		{ return &timeState; }

	CDContentWatcher *ContentWatcher()
		{ return &contentWatcher; }
	
	// to find the current location on the CD, you may call the GetVolume function
	// VolumeState defines
	VolumeState *VolumeStateWatcher()
		{ return &volumeState; }

private:
	PlayState			playState;
	TrackState			trackState;
	TimeState			timeState;
	VolumeState			volumeState;
	CDContentWatcher 	contentWatcher;

	bigtime_t 			lastPulse;
	
	CDState 			fEngineState;
};


// some function object glue
class CDEngineFunctorFactory : public FunctorFactoryCommon 
{
public:
	static BMessage *NewFunctorMessage(void (CDEngine::*func)(),
										CDEngine *target)
	{
		PlainMemberFunctionObject<void (CDEngine::*)(),
			CDEngine> tmp(func, target);
		return NewMessage(&tmp);
	}

	static BMessage *NewFunctorMessage(void (CDEngine::*func)(ulong),
										CDEngine *target, ulong param)
	{
		SingleParamMemberFunctionObject<void (CDEngine::*)(ulong),
			CDEngine, ulong> tmp(func, target, param);
		return NewMessage(&tmp);
	}
};


#endif