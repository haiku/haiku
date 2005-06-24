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

#include "Observer.h"
#include "FunctionObjectMessage.h"
#include "CDDBSupport.h"

class CDEngine;
class PeriodicWatcher : public Notifier {
// watcher sits somewhere were it can get pulses and makes sure
// notices get sent if state changes
public:
	PeriodicWatcher(int devicefd);
	PeriodicWatcher(BMessage *);
	
	virtual ~PeriodicWatcher() {}

	void DoPulse();
	void UpdateNow();

	void AttachedToLooper(CDEngine *engine)
		{ this->engine = engine; }

	virtual BHandler *RecipientHandler() const;

protected:
	virtual bool UpdateState() = 0;

	int devicefd;
	CDEngine *engine;
};

enum CDState {
	kNoCD=0,
	kStopped,
	kPaused,
	kPlaying,
	kSkipping
};

class PlayState : public PeriodicWatcher {
	// this watcher sends notices to observers that are interrested
	// about play state changes
public:
	PlayState(int devicefd);
	PlayState(BMessage *message)
		:	PeriodicWatcher(message)
		{}

	CDState GetState() const;

private:
	bool UpdateState();
	bool CurrentState(CDState);
	CDState oldState;
};

class TrackState : public PeriodicWatcher {
	// this watcher sends notices to observers that are interrested
	// about changes in the current track
public:
	TrackState(int devicefd);
	TrackState(BMessage *message)
		:	PeriodicWatcher(message)
		{}

	int32 GetTrack() const;
	int32 GetNumTracks() const;

private:
	bool UpdateState();
	bool CurrentState(int32);
	int32 currentTrack;
};

class TimeState : public PeriodicWatcher {
	// this watcher sends notices to observers that are interrested
	// about changes in the current time
public:
	TimeState(int devicefd)
		:	PeriodicWatcher(devicefd)
		{ }
	TimeState(BMessage *message)
		:	PeriodicWatcher(message)
		{}

	void GetTime(int32 &minutes, int32 &seconds) const;

private:
	bool UpdateState();
	bool CurrentState(int32 minutes, int32 seconds);
	int32 oldMinutes;
	int32 oldSeconds;
};

class CDContentWatcher : public PeriodicWatcher {
public:
	CDContentWatcher(int devicefd);
	CDContentWatcher(BMessage *message);
	
	bool GetContent(BString *title, vector<BString> *tracks);
	
private:
	bool UpdateState();

	CDDBQuery cddbQuery;
	int32 discID;
	bool wasReady;
};

class VolumeState : public PeriodicWatcher {
	// this watcher sends notices to observers that are interrested
	// about changes in the current volume
	// currently not used yet
public:
	VolumeState(int devicefd)
		:	PeriodicWatcher(devicefd)
		{ }
	VolumeState(BMessage *message)
		:	PeriodicWatcher(message)
		{}

	bool UpdateState() { return true; }
	virtual void DoPulse() {}
	int32 GetVolume() const;
};

class CDEngine : public BHandler {
	// The CD engine defines all the different CD control calls; also,
	// it hosts the different state watchers and helps them send notices
	// to observers about the CD state changes
public:
	CDEngine(int devicefd);
	CDEngine(BMessage *);

	virtual ~CDEngine();

	// observing supprt
	virtual void MessageReceived(BMessage *);
	void AttachedToLooper(BLooper *);
	void DoPulse();

	// control calls
	void PlayOrPause();
	void PlayContinue();
	void Play();
	void Pause();
	void Stop();
	void Eject();
	void SkipOneForward();
	void SkipOneBackward();
	void StartSkippingBackward();
	void StartSkippingForward();
	void StopSkipping();
	void SelectTrack(int32);
	

	TrackState *TrackStateWatcher()
		{ return &trackState; }
		// to find the current Track, you may call the GetTrack function
		// TrackState defines

	PlayState *PlayStateWatcher()
		{ return &playState; }
		// to find the current play state, you may call the GetState function
		// PlayState defines

	TimeState *TimeStateWatcher()
		{ return &timeState; }
		// to find the current location on the CD, you may call the GetTime function
		// TimeState defines

	CDContentWatcher *ContentWatcher()
		{ return &contentWatcher; }

	static int FindCDPlayerDevice();
	
private:
	int devicefd;

	PlayState playState;
	TrackState trackState;
	TimeState timeState;
	VolumeState volumeState;
	CDContentWatcher contentWatcher;

	bigtime_t lastPulse;
};


// some function object glue
class CDEngineFunctorFactory : public FunctorFactoryCommon {
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