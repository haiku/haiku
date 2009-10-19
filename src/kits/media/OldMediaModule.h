/* ================

   FILE:  MediaModule.h
   REVS:  $Revision: 1.1 $
   NAME:  marc

   Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

================ */

#ifndef _MEDIA_MODULE_H
#define _MEDIA_MODULE_H

#include <List.h>
#include <Locker.h>

typedef double mk_time;
typedef double mk_rate;
typedef void (*mk_deferred_call)(void*);

#define B_UNDEFINED_MK_TIME ((mk_time) -1)

enum transport_status {
  B_TRANSPORT_RUNNING,
  B_TRANSPORT_STOPPED,
  B_TRANSPORT_PAUSED
};

enum channel_selector {
  B_VIDEO_MAIN_CHANNEL,
  B_VIDEO_MONITOR_CHANNEL,
  B_AUDIO_FRONT_CHANNEL,
  B_AUDIO_CENTER_CHANNEL,
  B_AUDIO_REAR_CHANNEL,
  B_AUDIO_SUB_CHANNEL,
  B_AUDIO_MONITOR_CHANNEL,
  B_MIDI_CHANNEL,
  B_MISC_CHANNEL
};


class BMediaRenderer;
class BTransport;
class BTimeBase;
class BMessage;
class BMediaChannel;


class BMediaEvent {
public:
  virtual				~BMediaEvent() {};
  virtual mk_time		Start() = 0;
  virtual mk_time		Duration();
  virtual bigtime_t		CaptureTime();
};


class BEventStream {
public:
  BEventStream();
  virtual ~BEventStream();

  virtual mk_time			Start();
  virtual void				SetStart(mk_time);
  virtual mk_time			Duration();
  virtual void				SetDuration(mk_time);
  virtual BMediaEvent*		GetEvent(BMediaChannel* channel) = 0;
  virtual BMediaEvent*		PeekEvent(BMediaChannel* channel,
									  mk_time asap = 0) = 0;
  virtual status_t			SeekToTime(BMediaChannel* channel,
									   mk_time time);

private:
  mk_time					fStart;
  mk_time					fDuration;
};


class BMediaRenderer {
public:
  BMediaRenderer(const char* name = NULL, int32 priority = B_NORMAL_PRIORITY);
  virtual ~BMediaRenderer();

  virtual char*			Name();
  virtual mk_time		Latency();
  virtual BTransport*	Transport();
  virtual void			SetTransport(BTransport*);
  virtual mk_time		Start();
  virtual mk_time		Duration();
  virtual BTimeBase*	TimeBase();

  virtual void			Open();
  virtual void			Close();
  virtual void			WakeUp();
  virtual void			TransportChanged(mk_time time, mk_rate rate,
										 transport_status status);
  virtual void			StreamChanged();

  virtual void			OpenReceived();
  virtual void			CloseReceived();
  virtual void			WakeUpReceived();
  virtual void			TransportChangedReceived(mk_time time, mk_rate rate,
												  transport_status status);
  virtual void			StreamChangedReceived();

private:
  static int32			_LoopThread(void* arg);
  void					LoopThread();

  thread_id				fLoopThread;
  sem_id				fLoopSem;
  int32					fFlags;
  mk_time				fTCTime;
  mk_rate				fTCRate;
  transport_status		fTCStatus;
  BLocker				fLoopLock;

  char*					fName;
  BTransport*			fTransport;
};


class BTransport {
public:
  BTransport();
  virtual ~BTransport();

  virtual BTimeBase*		TimeBase();
  virtual void				SetTimeBase(BTimeBase*);
  virtual BList*			Renderers();
  virtual void				AddRenderer(BMediaRenderer*);
  virtual bool				RemoveRenderer(BMediaRenderer*);
  virtual transport_status	Status();
  virtual void				SetStatus(transport_status);
  virtual mk_time			PerformanceTime();
  virtual mk_rate			PerformanceRate();
  virtual mk_time			TimeOffset();
  virtual void				SetTimeOffset(mk_time);
  virtual mk_time			MaximumLatency();
  virtual mk_time			PerformanceStart();
  virtual mk_time			PerformanceEnd();
  virtual void				Open();
  virtual void				Close();
  virtual void				TransportChanged();
  virtual void				TimeSkipped();
  virtual void				RequestWakeUp(mk_time, BMediaRenderer*);
  virtual void				SeekToTime(mk_time);
  virtual BMediaChannel*	GetChannel(int32 selector);

private:
  BTimeBase*		fTimeBase;
  BList				fRenderers;
  BList				fChannels;
  BLocker			fChannelListLock;
  transport_status	fStatus;
  mk_time			fTimeOffset;
};


class BTimeBase {
public:
  BTimeBase(mk_rate rate = 1.0);
  virtual ~BTimeBase();

  virtual BList*	Transports();
  virtual void		AddTransport(BTransport*);
  virtual bool		RemoveTransport(BTransport*);
  virtual void		TimeSkipped();
  virtual status_t	CallAt(mk_time time, mk_deferred_call function, void* arg);
  virtual mk_time	Time();
  virtual mk_rate	Rate();
  virtual mk_time	TimeAt(bigtime_t system_time);
  virtual bigtime_t	SystemTimeAt(mk_time time);
  virtual void		Sync(mk_time time, bigtime_t system_time);
  virtual bool		IsAbsolute();

private:
  static int32	_SnoozeThread(void* arg);
  void			SnoozeThread();

  BList			fTransports;
  BList			fDeferredCalls;
  BLocker		fLock;
  mk_rate		fRate;
  mk_time		fSyncT;
  bigtime_t		fSyncS;
  mk_time		fSnoozeUntil;
  thread_id		fSnoozeThread;
};


class BMediaChannel {
public:
  BMediaChannel(mk_rate rate,
				BMediaRenderer* renderer = NULL,
				BEventStream* source = NULL);
  virtual ~BMediaChannel();

  virtual BMediaRenderer*	Renderer();
  virtual void				SetRenderer(BMediaRenderer*);
  virtual BEventStream*		Source();
  virtual void				SetSource(BEventStream*);
  virtual mk_rate			Rate();
  virtual void				SetRate(mk_rate);
		  bool				LockChannel();
		  status_t			LockWithTimeout(bigtime_t);
		  void				UnlockChannel();
  virtual void				StreamChanged();

private:
  mk_rate			fRate;
  BMediaRenderer*	fRenderer;
  BEventStream*		fSource;
  BLocker			fChannelLock;
};


#endif
