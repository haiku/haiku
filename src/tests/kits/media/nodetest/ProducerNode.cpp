#include <TimeSource.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include "ProducerNode.h"
#include "misc.h"

ProducerNode::ProducerNode() : 
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	BMediaNode("ProducerNode"),
	mBufferGroup(0),
	mBufferProducerSem(-1),
	mBufferProducer(-1),
	mOutputEnabled(false)
{
	out("ProducerNode::ProducerNode\n");
	mBufferGroup = new BBufferGroup(4096,3);
}

ProducerNode::~ProducerNode()
{	
	out("ProducerNode::~ProducerNode\n");
	Quit();
	delete mBufferGroup;
}

void
ProducerNode::NodeRegistered()
{
	out("ProducerNode::NodeRegistered\n");
	InitializeOutput();
	SetPriority(108); 
	Run(); 
}


status_t 
ProducerNode::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	out("ProducerNode::FormatSuggestionRequested\n");

	if (type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format->u.raw_audio.channel_count = 1;
	format->u.raw_audio.frame_rate = 44100;
	format->u.raw_audio.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;

	return B_OK;
}

status_t 
ProducerNode::FormatProposal(
				const media_source & output,
				media_format * format)
{
	out("ProducerNode::FormatProposal\n");

	if (format == NULL)
		return B_BAD_VALUE;

	if (output != mOutput.source)
		return B_MEDIA_BAD_SOURCE;
	
	return B_OK;
}

status_t 
ProducerNode::FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_)
{
	out("ProducerNode::FormatChangeRequested\n");
	return B_ERROR;
}

status_t 
ProducerNode::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	out("ProducerNode::GetNextOutput\n");
	if (++(*cookie) > 1)
		return B_BAD_INDEX;
		
	mOutput.node = Node();
	*out_output = mOutput;
	return B_OK;
}

status_t 
ProducerNode::DisposeOutputCookie(
				int32 cookie)
{
	out("ProducerNode::DisposeOutputCookie\n");
	return B_OK;
}

/*********************************************************
* In this function, you should either pass on the group to your upstream guy,
* or delete your current group and hang on to this group. Deleting the previous
* group (unless you passed it on with the reclaim flag set to false) is very
* important, else you will 1) leak memory and 2) block someone who may want
* to reclaim the buffers living in that group.
*/
status_t 
ProducerNode::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	out("ProducerNode::SetBufferGroup\n");
	
	if (for_source != mOutput.source)
		return B_MEDIA_BAD_SOURCE;

#if 0
	if (mBufferGroup != NULL && mBufferGroup != mOwnBufferGroup) { 
		// fixme! really delete if it isn't ours ?
		trace("deleting buffer group!...\n");
		delete mBufferGroup;
		trace("done!\n");
	}

	/* release the previous buffer group */
	if (mOwnBufferGroup != NULL) {
		delete_own_buffer_group();
	}

	mBufferGroup = group;

	/* allocate new buffer group if necessary */
	if (mBufferGroup == NULL) {
		create_own_buffer_group();
		mBufferGroup = mOwnBufferGroup;	
	}
	return B_OK;
#endif	
	
	return B_ERROR;
}

status_t 
ProducerNode::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	out("ProducerNode::VideoClippingChanged\n");
	return B_ERROR;
}
				
status_t 
ProducerNode::GetLatency(
				bigtime_t * out_lantency)
{
	out("ProducerNode::GetLatency\n");
	*out_lantency = 23000;
	return B_OK;
}

status_t 
ProducerNode::PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	out("ProducerNode::PrepareToConnect\n");

	if (mOutput.source != what)
		return B_MEDIA_BAD_SOURCE;
	
	if (mOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	if (format == NULL || out_source == NULL || out_name == NULL)
		return B_BAD_VALUE;

#if 0		
	ASSERT(mOutputEnabled == false);

	trace("old format:\n");
	dump_format(format);

	status_t status;
	
	status = specialize_format_to_inputformat(format);
	if (status != B_OK)
		return status;

#endif


	*out_source = mOutput.source;
	strcpy(out_name,mOutput.name);
	//mOutput.destination = where; //really now? fixme

	return B_OK;
}

void
ProducerNode::Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name)
{
	out("ProducerNode::Connect\n");

	if (error != B_OK) {
		InitializeOutput();
		return;
	}
/*
	if (mOutput.destination != destination) { //if connected in PrepareToConnect fixme?
		trace("error mOutput.destination != destination\n");
		return;
	}
*/	
	mOutput.destination = destination;

	if (mOutput.source != source) {
		out("error mOutput.source != source\n");
		return;
	}	
		
	strcpy(io_name,mOutput.name);

#if 0
	trace("format (final and approved):\n");
	dump_format(&format);
#endif

	mOutputEnabled = true;

	return;
}

void
ProducerNode::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	out("ProducerNode::Disconnect\n");
	mOutputEnabled = false;

	// unreserve connection
	InitializeOutput();
}

void
ProducerNode::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
	out("ProducerNode::LateNoticeReceived\n");
	return;
}

void
ProducerNode::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
	out("ProducerNode::EnableOutput\n");
	mOutputEnabled = enabled;
	return;
}

BMediaAddOn* 
ProducerNode::AddOn(int32 * internal_id) const
{
	out("ProducerNode::AddOn\n");
	return NULL;
}

void 
ProducerNode::HandleEvent(const media_timed_event *event,
						 bigtime_t lateness,
						 bool realTimeEvent)
{
	out("ProducerNode::HandleEvent\n");
	switch (event->type)
	{
	case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			out("B_HANDLE_BUFFER (should not happen)\n");
		}
		break;

	case BTimedEventQueue::B_PARAMETER:
		{
			out("B_PARAMETER\n");
		}
		break;

	case BTimedEventQueue::B_START:
		{
			out("B_START\n");
			if (mBufferProducer != -1) {
				out("already running\n");
				break;
			}
			mBufferProducerSem = create_sem(0,"producer blocking sem");
			mBufferProducer = spawn_thread(_bufferproducer,"Buffer Producer",B_NORMAL_PRIORITY,this);
			resume_thread(mBufferProducer);
		}
		break;

	case BTimedEventQueue::B_STOP:
		{
			out("B_STOP\n");
			if (mBufferProducer == -1) {
				out("not running\n");
				break;
			}
			status_t err;
			delete_sem(mBufferProducerSem);
			wait_for_thread(mBufferProducer,&err);
			mBufferProducer = -1;
			mBufferProducerSem = -1;
		}
		// stopping implies not handling any more buffers.  So, we flush all pending
		// buffers out of the event queue before returning to the event loop.
		EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
		break;

	case BTimedEventQueue::B_SEEK:
		{
			out("B_SEEK\n");
		}
		break;

	case BTimedEventQueue::B_WARP:
		{
			out("B_WARP\n");
		}
		// similarly, time warps aren't meaningful to the logger, so just record it and return
		//mLogger->Log(LOG_WARP_HANDLED, logMsg);
		break;

	case BTimedEventQueue::B_DATA_STATUS:
		{
			out("B_DATA_STATUS\n");
		}
		break;

	default:
		{
			out("default\n");
		}
		break;
	}
}


status_t 
ProducerNode::HandleMessage(int32 message,const void *data, size_t size)
{
	out("ProducerNode::HandleMessage %lx\n",message);
	if (B_OK == BBufferProducer::HandleMessage(message,data,size))
		return B_OK;
	if (B_OK == BMediaEventLooper::HandleMessage(message,data,size))
		return B_OK;
	return BMediaNode::HandleMessage(message,data,size);
}

void 
ProducerNode::AdditionalBufferRequested(
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	out("ProducerNode::AdditionalBufferRequested\n");
	release_sem(mBufferProducerSem);
}
				
void 
ProducerNode::LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags)
{
	out("ProducerNode::LatencyChanged\n");
}

void 
ProducerNode::InitializeOutput()
{
	out("ConsumerNode::InitializeOutput()\n");
	mOutput.source.port = ControlPort();
	mOutput.source.id = 0;
	mOutput.destination = media_destination::null;
	mOutput.node = Node();
	mOutput.format.type = B_MEDIA_RAW_AUDIO;
	mOutput.format.u.raw_audio = media_raw_audio_format::wildcard;
	mOutput.format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mOutput.format.u.raw_audio.channel_count = 1;
	mOutput.format.u.raw_audio.frame_rate = 44100;
	mOutput.format.u.raw_audio.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	strcpy(mOutput.name, "this way out");
}

int32 
ProducerNode::_bufferproducer(void *arg)
{
	((ProducerNode *)arg)->BufferProducer();
	return 0;
}

#define DELAY 2000000

void
ProducerNode::BufferProducer()
{
	// this thread produces one buffer each two seconds,
	// and shedules it to be handled one second later than produced
	// assuming a realtime timesource

	status_t rv;
	for (;;) {
		rv = acquire_sem_etc(mBufferProducerSem,1,B_RELATIVE_TIMEOUT,DELAY);
		if (rv == B_INTERRUPTED) {
			continue;
		} else if (rv == B_OK) {
			// triggered by AdditionalBufferRequested
			release_sem(mBufferProducerSem);
		} else if (rv != B_TIMED_OUT) {
			// triggered by deleting the semaphore (stop request)
			break;
		}
		if (!mOutputEnabled)
			continue;
			
		BBuffer *buffer;
//		out("ProducerNode: RequestBuffer\n");
		buffer = mBufferGroup->RequestBuffer(2048);
		if (!buffer) {
		}
		buffer->Header()->start_time = TimeSource()->Now() + DELAY / 2;
		out("ProducerNode: SendBuffer, sheduled time = %5.4f\n",buffer->Header()->start_time / 1E6);
		rv = SendBuffer(buffer, mOutput.destination);
		if (rv != B_OK) {
		}
	}
}
