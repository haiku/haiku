/*******************************************************************************
/
/	File:			MediaEventLooper.h
/
/	Description:	BMediaEventLooper spawns a thread which calls WaitForMessage,
/					pushes BMediaNode messages onto its BTimedEventQueues.
/					informs you when it is time to handle an event.
/					Report your event latency, push other events onto the queue
/					and override HandleEvent to do your work.
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_MEDIA_EVENT_LOOPER_H)
#define _MEDIA_EVENT_LOOPER_H

#include <MediaNode.h>
#include <TimedEventQueue.h>

class BMediaEventLooper :
	public virtual BMediaNode
{
	protected:
		enum run_state {
			B_IN_DISTRESS = -1,
			B_UNREGISTERED,
			B_STOPPED,
			B_STARTED,
			B_QUITTING,
			B_TERMINATED,
			B_USER_RUN_STATES = 0x4000
		};
	
	protected:
		/* this has to be on top rather than bottom to force a vtable in mwcc */
		virtual				~BMediaEventLooper();
		explicit			BMediaEventLooper(uint32 apiVersion = B_BEOS_VERSION);

	/* from BMediaNode */		
	protected:
		virtual void		NodeRegistered();
		virtual void		Start(bigtime_t performance_time);
		virtual void		Stop(bigtime_t performance_time, bool immediate);
		virtual void		Seek(bigtime_t media_time, bigtime_t performance_time);
		virtual void		TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);
		virtual status_t	AddTimer(bigtime_t at_performance_time, int32 cookie);
		virtual	void 		SetRunMode(run_mode mode);
		
	/* BMediaEventLooper Hook functions */
	protected:	
		/* you must override to handle your events! */
		/* you should not call HandleEvent directly */
		virtual void		HandleEvent(	const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent = false) = 0;

		/* override to clean up custom events you have added to your queue */
		virtual void		CleanUpEvent(const media_timed_event *event);
		
		/* called from Offline mode to determine the current time of the node */
		/* update your internal information whenever it changes */
		virtual	bigtime_t	OfflineTime();

		/* override only if you know what you are doing! */
		/* otherwise much badness could occur */
		/* the actual control loop function: */
		/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
		virtual void		ControlLoop();

		thread_id			ControlThread();

	protected:
		BTimedEventQueue * 	EventQueue();
		BTimedEventQueue *	RealTimeQueue();

		int32				Priority() const;
		int32				RunState() const;
		bigtime_t			EventLatency() const;
		bigtime_t			BufferDuration() const;
		bigtime_t			SchedulingLatency() const;
		
		/* use the priority constants from OS.h */
		/* or suggest_thread_priority from scheduler.h */
		/* will clamp priorities to be inbetween 5 and 120 */
		status_t			SetPriority(int32 priority);

		/* set the run state */
		void				SetRunState(run_state state);
		
		/* clamps to 0 if latency < 0 */
		void				SetEventLatency(bigtime_t latency);
		
		/* clamps to 0 if duration is < 0 */
		void				SetBufferDuration(bigtime_t duration);

		/* set the offline time returned in OfflineTime */
		void				SetOfflineTime(bigtime_t offTime);
		
		/* spawn and resume the thread - must be called from NodeRegistered */
		void				Run();
		
		/* close down the thread - must be called from your destructor */
		void				Quit();
		
		/* calls HandleEvent and does BMediaEventLooper event work */
		void				DispatchEvent(	const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent = false);

	private:
		static int32		_ControlThreadStart(void *arg);
		static void			_CleanUpEntry(const media_timed_event *event, void *context);
		void				_DispatchCleanUp(const media_timed_event *event);

		BTimedEventQueue	fEventQueue;
		BTimedEventQueue	fRealTimeQueue;
		thread_id			fControlThread;
		int32				fCurrentPriority;
		int32				fSetPriority;
		volatile int32		fRunState;
		bigtime_t			fEventLatency;
		bigtime_t			fSchedulingLatency;
		bigtime_t			fBufferDuration;
		bigtime_t			fOfflineTime;
		uint32				fApiVersion;

	private:
	/* unimplemented for your protection */
							BMediaEventLooper(const BMediaEventLooper&);
							BMediaEventLooper& operator=(const BMediaEventLooper&);

	protected:
		virtual	status_t 	DeleteHook(BMediaNode * node);

	/* fragile base class stuffing */
	private:	
		/* it must be thanksgiving!! lots of stuffing! */
		virtual	status_t 	_Reserved_BMediaEventLooper_0(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_1(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_2(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_3(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_4(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_5(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_6(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_7(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_8(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_9(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_10(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_11(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_12(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_13(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_14(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_15(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_16(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_17(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_18(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_19(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_20(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_21(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_22(int32 arg, ...);
		virtual	status_t 	_Reserved_BMediaEventLooper_23(int32 arg, ...);
		
		/* turkey for weeks! */
		bool				_reserved_bool_[4];
		uint32				_reserved_BMediaEventLooper_[12];
};

#endif
