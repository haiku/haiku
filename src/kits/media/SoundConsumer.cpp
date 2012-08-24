/******************************************************************************
/
/	File:			SoundConsumer.cpp
/
/   Description:	Record sound from some sound-producing Node.
/
/	Copyright 1998-1999, Be Incorporated, All Rights Reserved
/
******************************************************************************/
#include "SoundConsumer.h"

#include <new>
#include <stdio.h>

#include <OS.h>
#include <scheduler.h>
#include <Buffer.h>
#include <TimeSource.h>

#include "AutoDeleter.h"


using std::nothrow;


namespace BPrivate {


//	If we don't mind the format changing to another format while
//	running, we can define this to 1. Look for the symbol down in the source.
#define ACCEPT_ANY_FORMAT_CHANGE 0


//	Compiling with NDEBUG means "release" -- it also turns off assert() and
//	other such debugging aids. By contrast, DEBUG turns on extra debugging
//	in some programs.
#if !NDEBUG
#define TRACE_SOUNDCONSUMER
#endif

//	Comment out the FPRINTF part of these lines to reduce verbiage.
//	Enabling MESSAGE will kill performance on slower machines, because it
//	prints for each message received (including each buffer).
#ifdef TRACE_SOUNDCONSUMER
#define NODE fprintf
#define MESSAGE fprintf
#else
#define NODE(x...)
#define MESSAGE(x...)
#endif


//	This structure is the body of a request that we use to
//	implement SetHooks().
struct set_hooks_q {
	port_id reply;
	void * cookie;
	SoundProcessFunc process;
	SoundNotifyFunc notify;
};


//	All incoming buffers and Media Kit requests arrive at a
//	media node in the form of messages (which are generally
//	dispatched for you by your superclasses' HandleMessage
//	implementations). Each message has a 'type' which is
//	analagous to a BMessage's 'what' field. We'll define our
//	own private message types for our SoundConsumer and
//	SoundProducer to use. The BeOS reserves a range,
//	0x60000000 to 0x7fffffff, for us to use.
enum {
	MSG_QUIT_NOW = 0x60000000L,
	MSG_CHANGE_HOOKS
};


SoundConsumer::SoundConsumer(
	const char * name,
	SoundProcessFunc recordFunc,
	SoundNotifyFunc notifyFunc,
	void * cookie) :
	BMediaNode(name ? name : "SoundConsumer"),
	BBufferConsumer(B_MEDIA_RAW_AUDIO)
{
	NODE(stderr, "SoundConsumer::SoundConsumer(%p, %p, %p, %p)\n", name,
		recordFunc, notifyFunc, cookie);

	if (!name) name = "SoundConsumer";

	//	Set up the hook functions.
	m_recordHook = recordFunc;
	m_notifyHook = notifyFunc;
	m_cookie = cookie;

	//	Create the port that we publish as our Control Port.
	char pname[32];
	sprintf(pname, "%.20s Control", name);
	m_port = create_port(10, pname);

	// Initialize our single media_input. Make sure it knows
	// the Control Port associated with the destination, and
	// the index of the destination (since we only have one,
	// that's trivial).
	m_input.destination.port = m_port;
	m_input.destination.id = 1;
	sprintf(m_input.name, "%.20s Input", name);

	// Set up the timing variables that we'll be using.
	m_trTimeout = 0LL;
	m_tpSeekAt = 0;
	m_tmSeekTo = 0;
	m_delta = 0;
	m_seeking = false;

	//	Create, and run, the thread that we use to service
	// the Control Port.
	sprintf(pname, "%.20s Service", name);
	m_thread = spawn_thread(ThreadEntry, pname, 110, this);
	resume_thread(m_thread);
}


SoundConsumer::~SoundConsumer()
{
	NODE(stderr, "SoundConsumer::~SoundConsumer()\n");
	//	Signal to our thread that it's time to go home.
	write_port(m_port, MSG_QUIT_NOW, 0, 0);
	status_t s;
	while (wait_for_thread(m_thread, &s) == B_INTERRUPTED)
		NODE(stderr, "wait_for_thread() B_INTERRUPTED\n");
	delete_port(m_port);
}


status_t
SoundConsumer::SetHooks(SoundProcessFunc recordFunc, SoundNotifyFunc notifyFunc,
	void* cookie)
{
	//	SetHooks needs to be synchronized with the service thread, else we may
	//	call the wrong hook function with the wrong cookie, which would be bad.
	//	Rather than do locking, which is expensive, we streamline the process
	//	by sending our service thread a request to change the hooks, and waiting
	//	for the acknowledge.
	status_t err = B_OK;
	set_hooks_q cmd;
	cmd.process = recordFunc;
	cmd.notify = notifyFunc;
	cmd.cookie = cookie;
	//	If we're not in the service thread, we need to round-trip a message.
	if (find_thread(0) != m_thread) {
		cmd.reply = create_port(1, "SetHooks reply");
		//	Send the private message to our service thread.
		err = write_port(ControlPort(), MSG_CHANGE_HOOKS, &cmd, sizeof(cmd));
		if (err >= 0) {
			int32 code;
			//	Wait for acknowledge from the service thread.
			err = read_port_etc(cmd.reply, &code, 0, 0, B_TIMEOUT, 6000000LL);
			if (err > 0) err = 0;
			NODE(stderr, "SoundConsumer::SetHooks read reply: %#010lx\n", err);
		}
		//	Clean up.
		delete_port(cmd.reply);
	} else {
		//	Within the service thread, it's OK to just go ahead and do the
		//	change.
		DoHookChange(&cmd);
	}
	return err;
}


// #pragma mark -BMediaNode-derived methods


port_id
SoundConsumer::ControlPort() const
{
	return m_port;
}


BMediaAddOn*
SoundConsumer::AddOn(int32 * internal_id) const
{
	//	This object is instantiated inside an application.
	//	Therefore, it has no add-on.
	if (internal_id) *internal_id = 0;
	return 0;
}


void
SoundConsumer::Start(bigtime_t performance_time)
{
	//	Since we are a consumer and just blindly accept buffers that are
	//	thrown at us, we don't need to do anything special in Start()/Stop().
	//	If we were (also) a producer, we'd have to be more elaborate.
	//	The only thing we do is immediately perform any queued Seek based on
	//	the start time, which is the right thing to do (seeing as we were
	//	Seek()-ed when we weren't started).
	if (m_seeking) {
		m_delta = performance_time - m_tmSeekTo;
		m_seeking = false;
	}
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_WILL_START, performance_time);
	else
		Notify(B_WILL_START, performance_time);
}


void
SoundConsumer::Stop(bigtime_t performance_time, bool immediate)
{
	//	Since we are a consumer and just blindly accept buffers that are
	//	thrown at us, we don't need to do anything special in Start()/Stop().
	//	If we were (also) a producer, we'd have to be more elaborate.
	//	Note that this is not strictly in conformance with The Rules,
	//	but since this is not an add-on Node for use with any application;
	//	it's a Node over which we have complete control, we can live with
	//	treating buffers received before the start time or after the stop
	//	time as any other buffer.
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_WILL_STOP, performance_time, immediate);
	else
		Notify(B_WILL_STOP, performance_time, immediate);
}


void
SoundConsumer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	//	Seek() on a consumer just serves to offset the time stamp
	//	of received buffers passed to our Record hook function.
	//	In the hook function, you may wish to save those time stamps
	//	to disk or otherwise store them. You may also want to
	//	synchronize this node's media time with an upstream
	//	producer's media time to make this offset meaningful.
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_WILL_SEEK, performance_time, media_time);
	else
		Notify(B_WILL_SEEK, performance_time, media_time);

	m_tpSeekAt = performance_time;
	m_tmSeekTo = media_time;
	m_seeking = true;
}


void
SoundConsumer::SetRunMode(run_mode mode)
{
	if (mode == BMediaNode::B_OFFLINE) {
		// BMediaNode::B_OFFLINE means we don't need to run in
		// real time. So, we shouldn't run as a real time
		// thread.
		int32 new_prio = suggest_thread_priority(B_OFFLINE_PROCESSING);
		set_thread_priority(m_thread, new_prio);
	} else {
		//	We're running in real time, so we'd better have
		//	a big enough thread priority to handle it!
		//	Here's where those magic scheduler values
		//	come from:
		//
		//	* In the worst case, we process one buffer per
		//	  reschedule (we get rescheduled when we go to
		//	  look for a message on our Control Port), so
		//	  in order to keep up with the incoming buffers,
		//	  the duration of one buffer becomes our
		//	  scheduling period. If we don't know anything
		//	  about the buffers, we pick a reasonable
		//	  default.
		//	* We're a simple consumer, so we don't have to
		//	  be too picky about the jitter. Half a period
		//	  of jitter means that we'd have to get two
		//	  consecutive worst-case reschedules before
		//	  we'd fall behind.
		//	* The amount of time we spend processing is
		//	  our ProcessingLatency().
		bigtime_t period = 10000;
		if (buffer_duration(m_input.format.u.raw_audio) > 0)
			period = buffer_duration(m_input.format.u.raw_audio);

		//	assuming we're running for 500 us or less per buffer
		int32 new_prio = suggest_thread_priority(B_AUDIO_RECORDING,
			period, period / 2, ProcessingLatency());
		set_thread_priority(m_thread, new_prio);
	}
}


void
SoundConsumer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	//	Since buffers will come pre-time-stamped, we only need to look
	//	at them, so we can ignore the time warp as a consumer.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_WILL_TIMEWARP, at_real_time,
			to_performance_time);
	} else
		Notify(B_WILL_TIMEWARP, at_real_time, to_performance_time);
}


void
SoundConsumer::Preroll()
{
	//	There is nothing for us to do in Preroll()
}


void
SoundConsumer::SetTimeSource(BTimeSource* /* time_source */)
{
	//	We don't need to do anything special to take note of the
	//	fact that the time source changed, because we get our timing
	//	information from the buffers we receive.
}


status_t
SoundConsumer::HandleMessage(int32 message, const void* data, size_t size)
{
	// Check with each of our superclasses to see if they
	// understand the message. If none of them do, call
	// BMediaNode::HandleBadMessage().
	if (BBufferConsumer::HandleMessage(message, data, size) < 0
		&& BMediaNode::HandleMessage(message, data, size) < 0) {
		HandleBadMessage(message, data, size);
		return B_ERROR;
	}
	return B_OK;
}


// #pragma mark - BBufferConsumer-derived methods


status_t
SoundConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
	//	We only accept formats aimed at our single input.
	if (dest != m_input.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (format->type <= 0) {
		//	If no format is specified, we say we want raw audio.
		format->type = B_MEDIA_RAW_AUDIO;
		format->u.raw_audio = media_raw_audio_format::wildcard;
	} else if (format->type != B_MEDIA_RAW_AUDIO) {
		//	If a non-raw-audio format is specified, we tell the world what
		//	we want, and that the specified format was unacceptable to us.
		format->type = B_MEDIA_RAW_AUDIO;
		format->u.raw_audio = media_raw_audio_format::wildcard;
		return B_MEDIA_BAD_FORMAT;
	}
#if !ACCEPT_ANY_FORMAT_CHANGE
	//	If we're already connected, and this format doesn't go with the
	//	format in effect, we dont' accept this new format.
	if (!format_is_compatible(*format, m_input.format)) {
		*format = m_input.format;
		return B_MEDIA_BAD_FORMAT;
	}
#endif
	//	I guess we're OK by now, because we have no particular needs as
	//	far as frame rate, sample format, etc go.
	return B_OK;
}


status_t
SoundConsumer::GetNextInput(int32* cookie, media_input* out_input)
{
	NODE(stderr, "SoundConsumer: GetNextInput()\n");
	//	The "next" is kind of misleading, since it's also used for
	//	getting the first (and only) input.
	if (*cookie == 0) {
		if (m_input.source == media_source::null) {
			//	If there's no current connection, make sure we return a
			//	reasonable format telling the world we accept any raw audio.
			m_input.format.type = B_MEDIA_RAW_AUDIO;
			m_input.format.u.raw_audio = media_raw_audio_format::wildcard;
			m_input.node = Node();
			m_input.destination.port = ControlPort();
			m_input.destination.id = 1;
		}
		*out_input = m_input;
		*cookie = 1;
		return B_OK;
	}
	//	There's only one input.
	return B_BAD_INDEX;
}


void
SoundConsumer::DisposeInputCookie(int32 /* cookie */)
{
	//	We didn't allocate any memory or set any state in GetNextInput()
	//	so this function is a no-op.
}


void
SoundConsumer::BufferReceived(BBuffer* buffer)
{
	NODE(stderr, "SoundConsumer::BufferReceived()\n");
	//	Whee, a buffer! Update the seek info, if necessary.
	if (m_seeking && buffer->Header()->start_time >= m_tpSeekAt) {
		m_delta = m_tpSeekAt - m_tmSeekTo;
		m_seeking = false;
	}
	//	If there is a record hook, let the interested party have at it!
	if (m_recordHook) {
		(*m_recordHook)(m_cookie, buffer->Header()->start_time-m_delta,
			buffer->Data(), buffer->Header()->size_used,
			m_input.format.u.raw_audio);
	} else {
		Record(buffer->Header()->start_time-m_delta, buffer->Data(),
			buffer->Header()->size_used, m_input.format.u.raw_audio);
	}
	//	Buffers should ALWAYS be recycled, else whomever is producing them
	//	will starve.
	buffer->Recycle();
}


void
SoundConsumer::ProducerDataStatus(const media_destination& for_whom,
	int32 status, bigtime_t at_media_time)
{
	if (for_whom != m_input.destination)
		return;

	//	Tell whomever is interested that the upstream producer will or won't
	//	send more data in the immediate future.
	if (m_notifyHook) {
		(*m_notifyHook)(m_cookie, B_PRODUCER_DATA_STATUS, status,
			at_media_time);
	} else
		Notify(B_PRODUCER_DATA_STATUS, status, at_media_time);
}


status_t
SoundConsumer::GetLatencyFor(const media_destination& for_whom,
	bigtime_t* out_latency, media_node_id* out_timesource)
{
	//	We only accept requests for the one-and-only input of our Node.
	if (for_whom != m_input.destination)
		return B_MEDIA_BAD_DESTINATION;

	//	Tell the world about our latency information (overridable by user).
	*out_latency = TotalLatency();
	*out_timesource = TimeSource()->Node().node;
	return B_OK;
}


status_t
SoundConsumer::Connected(const media_source& producer,
	const media_destination& where, const media_format& with_format,
	media_input* out_input)
{
	NODE(stderr, "SoundConsumer::Connected()\n");
	//	Only accept connection requests when we're not already connected.
	if (m_input.source != media_source::null)
		return B_MEDIA_BAD_DESTINATION;

	//	Only accept connection requests on the one-and-only available input.
	if (where != m_input.destination)
		return B_MEDIA_BAD_DESTINATION;

	//	Other than that, we accept pretty much anything. The format has been
	//	pre-cleared through AcceptFormat(), and we accept any format anyway.
	m_input.source = producer;
	m_input.format = with_format;
	//	Tell whomever is interested that there's now a connection.
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_CONNECTED, m_input.name);
	else
		Notify(B_CONNECTED, m_input.name);

	//	This is the most important line -- return our connection information
	//	to the world so it can use it!
	*out_input = m_input;
	return B_OK;
}


void
SoundConsumer::Disconnected(const media_source& producer,
	const media_destination& where)
{
	//	We can't disconnect something which isn't us.
	if (where != m_input.destination)
		return;
	//	We can't disconnect from someone who isn't connected to us.
	if (producer != m_input.source)
		return;
	//	Tell the interested party that it's time to leave.
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_DISCONNECTED);
	else
		Notify(B_DISCONNECTED);
	//	Mark ourselves as not-connected.
	m_input.source = media_source::null;
}


status_t
SoundConsumer::FormatChanged(const media_source& producer,
	const media_destination& consumer, int32 from_change_count,
	const media_format& format)
{
	NODE(stderr, "SoundConsumer::Connected()\n");
	//	The up-stream guy feels like changing the format. If we can accept
	//	arbitrary format changes, we just say "OK". If, however, we're recording
	//	to a file, that's not such a good idea; we only accept format changes
	//	that are compatible with the format we're already using. You set this
	//	behaviour at compile time by defining ACCEPT_ANY_FORMAT_CHANGE to 1 or
	//	0.
	status_t err = B_OK;
#if ACCEPT_ANY_FORMAT_CHANGE
	media_format fmt(format);
	err = AcceptFormat(m_input.destination, &fmt);
#else
	if (m_input.source != media_source::null) {
		err = format_is_compatible(format, m_input.format) ? B_OK
			: B_MEDIA_BAD_FORMAT;
	}
#endif
	if (err >= B_OK) {
		m_input.format = format;
		if (m_notifyHook) {
			(*m_notifyHook)(m_cookie, B_FORMAT_CHANGED,
				&m_input.format.u.raw_audio);
		} else
			Notify(B_FORMAT_CHANGED, &m_input.format.u.raw_audio);
	}
	return err;
}


void
SoundConsumer::DoHookChange(void* msg)
{
	//	Tell the old guy we're changing the hooks ...
	if (m_notifyHook)
		(*m_notifyHook)(m_cookie, B_HOOKS_CHANGED);
	else
		Notify(B_HOOKS_CHANGED);

	//	... and then do it.
	set_hooks_q * ptr = (set_hooks_q *)msg;
	m_recordHook = ptr->process;
	m_notifyHook = ptr->notify;
	m_cookie = ptr->cookie;
}


status_t
SoundConsumer::ThreadEntry(void* cookie)
{
	SoundConsumer* consumer = (SoundConsumer*)cookie;
	consumer->ServiceThread();
	return 0;
}


void
SoundConsumer::ServiceThread()
{
	//	The Big Bad ServiceThread receives messages aimed at this
	//	Node and dispatches them (typically to HandleMessage()).
	//	If we were a Producer, we might have to do finicky timing and
	//	queued Start()/Stop() processing in here. But we ain't.

	//	A media kit message will never be bigger than B_MEDIA_MESSAGE_SIZE.
	//	Avoid wasing stack space by dynamically allocating at start.
	char* msg = new (nothrow) char[B_MEDIA_MESSAGE_SIZE];
	if (msg == NULL)
		return;
	//	Make sure we clean up this data when we exit the function.
	ArrayDeleter<char> _(msg);
	int bad = 0;
	while (true) {
		//	Call read_port_etc() with a timeout derived from a virtual function,
		//	to allow clients to do special processing if necessary.
		bigtime_t timeout = Timeout();
		int32 code = 0;
		status_t err = read_port_etc(m_port, &code, msg, B_MEDIA_MESSAGE_SIZE,
			B_TIMEOUT, timeout);
		MESSAGE(stderr, "SoundConsumer::ServiceThread() port %ld message "
			"%#010lx\n", m_port, code);
		//	If we received a message, err will be the size of the message
		//	(including 0).
		if (err >= B_OK) {
			//	Real messages reset the timeout time.
			m_trTimeout = 0;
			bad = 0;
			if (code == MSG_QUIT_NOW) {
				//	Check for our private stop message.
				if (m_notifyHook)
					(*m_notifyHook)(m_cookie, B_NODE_DIES, 0);
				else
					Notify(B_NODE_DIES, 0);
				break;
			} else if (code == MSG_CHANGE_HOOKS) {
			//	Else check for our private change-hooks message.
				DoHookChange(msg);
				//	Write acknowledge to waiting thread.
				write_port(((set_hooks_q *)msg)->reply, 0, 0, 0);
			} else {
				//	Else it has to be a regular media kit message;
				//	go ahead and dispatch it.
				HandleMessage(code, msg, err);
			}
		} else if (err == B_TIMED_OUT) {
			//	Timing out means that there was no buffer. Tell the interested
			//	party.
			if (m_notifyHook)
				(*m_notifyHook)(m_cookie, B_OP_TIMED_OUT, timeout);
			else
				Notify(B_OP_TIMED_OUT, timeout);
		} else {
			//	Other errors are bad.
			MESSAGE(stderr, "SoundConsumer: error %#010lx\n", err);
			bad++;
			//	If we receive three bad reads with no good messages inbetween,
			//	things are probably not going to improve (like the port
			//	disappeared or something) so we call it a day.
			if (bad > 3) {
				if (m_notifyHook)
					(*m_notifyHook)(m_cookie, B_NODE_DIES, bad, err, code, msg);
				else
					Notify(B_NODE_DIES, bad, err, code, msg);
				break;
			}
		}
	}
}


bigtime_t
SoundConsumer::Timeout()
{
	//	Timeout() is called for each call to read_port_etc() in the service
	//	thread to figure out a reasonable time-out value. The default behaviour
	//	we've picked is to exponentially back off from one second and upwards.
	//	While it's true that 44 back-offs will run us out of precision in a
	//	bigtime_t, the time to actually reach 44 consecutive back-offs is longer
	//	than the expected market longevity of just about any piece of real
	//	estate. Is that the sound of an impending year-fifteen-million software
	//	problem? :-)
	m_trTimeout = (m_trTimeout < 1000000) ? 1000000 : m_trTimeout*2;
	return m_trTimeout;
}


bigtime_t
SoundConsumer::ProcessingLatency()
{
	//	We're saying it takes us 500 us to process each buffer. If all we do is
	//	copy the data, it probably takes much less than that, but it doesn't
	//	hurt to be slightly conservative. 
	return 500LL;
}


bigtime_t
SoundConsumer::TotalLatency()
{
	//	Had we been a producer that passes buffers on, we'd have to
	//	include downstream latency in this value. But we are not.
	return ProcessingLatency();
}


// #pragma mark -


void
SoundConsumer::Record(bigtime_t /*time*/, const void* /*data*/,
	size_t /*size*/, const media_raw_audio_format& /*format*/)
{
	//	If there is no record hook installed, we instead call this function
	//	for received buffers.
}


void
SoundConsumer::Notify(int32 /*cause*/, ...)
{
	//	If there is no notification hook installed, we instead call this
	//	function for giving notification of various events.
}

}
