/*******************************************************************************
/
/	File:			SoundConsumer.h
/
/   Description:	Record sound from some sound-producing Node.
/
/	Copyright 1998, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#if !defined( _SoundConsumer_h )
#define _SoundConsumer_h

#include <BufferConsumer.h>
#include "SoundUtils.h"

//	To use this Consumer:
//	1. Create Record and Notify hooks, or subclass SoundConsumer
//	   if you'd rather use the inheritance hierarchy.
//	   * The Record function should do whatever you want to do
//	     when you receive a buffer.
//	   * The Notify function should handle whatever events
//	     you wish to handle (defined in SoundUtil.h).
//	2: Create an instance of SoundConsumer, giving it the
//	   appropriate hook functions. Or, create an instance of an
//	   appropriate subclass if you've made one.
//	3: Register your new Consumer with the MediaRoster.
//	4: Connect your Consumer to some Producer.
//	5: Start or Stop the Consumer if your hook functions
//	   implement behavior for these kinds of events.
//	   Seek the Consumer to set the offset of the timestamps that
//	   your Record function will see.
//	6: When you're done, disconnect the Consumer, then delete it.

class SoundConsumer :
	public BBufferConsumer
{
public:
		SoundConsumer(
				const char * name,
				SoundProcessFunc recordFunc = NULL,
				SoundNotifyFunc notifyFunc = NULL,
				void * cookie = NULL);
		~SoundConsumer();

		//	This function is OK to call from any thread.
		status_t SetHooks(
				SoundProcessFunc recordFunc = NULL,
				SoundNotifyFunc notifyFunc = NULL,
				void * cookie = NULL);

		//	The MediaNode interface
public:
virtual	port_id ControlPort() const;
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;	/* Who instantiated you -- or NULL for app class */


protected:

virtual	void Start(
				bigtime_t performance_time);
virtual	void Stop(
				bigtime_t performance_time,
				bool immediate);
virtual	void Seek(
				bigtime_t media_time,
				bigtime_t performance_time);
virtual	void SetRunMode(
				run_mode mode);
virtual	void TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time);
virtual	void Preroll();
virtual	void SetTimeSource(
				BTimeSource * time_source);
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);

		//	The BufferConsumer interface
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput( /* cookie starts at 0 */
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	void ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_media_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format & format);

protected:
		//	Functions called when no hooks are installed.
		//	OK to override instead of installing hooks.
virtual	void Record(
				bigtime_t time,
				const void * data,
				size_t size,
				const media_raw_audio_format & format);
virtual	void Notify(
				int32 cause,
				...);

private:
		SoundProcessFunc m_recordHook;
		SoundNotifyFunc m_notifyHook;
		void * m_cookie;
		media_input m_input;
		thread_id m_thread;
		port_id m_port;

		// The times we need to deal with
		// My notation for times: tr = real time,
		// tp = performance time, tm = media time.
		bigtime_t m_trTimeout;	// how long to wait on the input port
		bigtime_t m_tpSeekAt;	// when we Seek
		bigtime_t m_tmSeekTo;	// target time for Seek

		// The transformation from media to peformance time.
		// d = p - m, so m + d = p.
		// Media time is generally governed by the Seek
		// function. In our node, we simply use media time as
		// the time that we report to the record hook function.
		// If we were a producer node, we might use media time
		// to track where we were in playing a certain piece
		// of media. But we aren't.
		bigtime_t m_delta;
		
		// State variables
		bool m_seeking;		// a Seek is pending

		//	Functions to calculate timing values. OK to override.
		//	ProcessingLatency is the time it takes to process a buffer;
		//	TotalLatency is returned to producer; Timeout is passed
		//	to call to read_port_etc() in service thread loop.
virtual	bigtime_t Timeout();
virtual	bigtime_t ProcessingLatency();
virtual bigtime_t TotalLatency();
		//	The actual thread doing the work
static	status_t ThreadEntry(
				void * obj);
		void ServiceThread();
		void DoHookChange(
				void * msg);
};


#endif	/* _SoundConsumer_h */

