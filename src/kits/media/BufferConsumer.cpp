/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: BufferConsumer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include <malloc.h>
#include "debug.h"
#include "../server/headers/ServerInterface.h"
#include "BufferIdCache.h"

/*************************************************************
 * protected BBufferConsumer
 *************************************************************/

/* virtual */
BBufferConsumer::~BBufferConsumer()
{
	CALLED();
	delete fBufferCache;
	if (fDeleteBufferGroup)
		delete fDeleteBufferGroup;
}


/*************************************************************
 * public BBufferConsumer
 *************************************************************/

media_type
BBufferConsumer::ConsumerType()
{
	CALLED();
	return fConsumerType;
}


/* static */ status_t
BBufferConsumer::RegionToClipData(const BRegion *region,
								  int32 *format,
								  int32 *ioSize,
								  void *data)
{
	CALLED();
	
	status_t rv;
	int count;

	count = *ioSize / sizeof(int16);
	rv = BBufferProducer::clip_region_to_shorts(region, (int16 *)data, count, &count);
	*ioSize	= count * sizeof(int16);
	*format = BBufferProducer::B_CLIP_SHORT_RUNS;
	
	return rv;
}

/*************************************************************
 * protected BBufferConsumer
 *************************************************************/

/* explicit */
BBufferConsumer::BBufferConsumer(media_type consumer_type) :
	BMediaNode("called by BBufferConsumer"),
	fConsumerType(consumer_type),
	fBufferCache(new _buffer_id_cache),
	fDeleteBufferGroup(0)
{
	CALLED();
	
	AddNodeKind(B_BUFFER_CONSUMER);
}


/* static */ void
BBufferConsumer::NotifyLateProducer(const media_source &what_source,
									bigtime_t how_much,
									bigtime_t performance_time)
{
	CALLED();
	if (what_source == media_source::null)
		return;

	xfer_producer_late_notice_received data;
	data.source = what_source;
	data.how_much = how_much;
	data.performance_time = performance_time;
	
	write_port(what_source.port, PRODUCER_LATE_NOTICE_RECEIVED, &data, sizeof(data));
}


status_t
BBufferConsumer::SetVideoClippingFor(const media_source &output,
									 const media_destination &destination,
									 const int16 *shorts,
									 int32 short_count,
									 const media_video_display_info &display,
									 void *user_data,
									 int32 *change_tag,
									 void *_reserved_)
{
	CALLED();
	if (output == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (short_count > int(B_MEDIA_MESSAGE_SIZE - sizeof(xfer_producer_video_clipping_changed)) / 2)
		debugger("BBufferConsumer::SetVideoClippingFor short_count too large (8000 limit)\n");
		
	xfer_producer_video_clipping_changed *data;
	size_t size;
	status_t rv;

	size = sizeof(xfer_producer_video_clipping_changed) + short_count * 2;
	data = (xfer_producer_video_clipping_changed *) malloc(size);
	data->source = output;
	data->destination = destination;
	data->display = display;
	data->user_data = user_data;
	data->change_tag = NewChangeTag();
	data->short_count = short_count;
	memcpy(data->shorts, shorts, short_count * 2);
	if (change_tag != NULL)
		*change_tag = data->change_tag;
	
	rv = write_port(output.port, PRODUCER_VIDEO_CLIPPING_CHANGED, data, size);
	free(data);
	return rv;
}


status_t
BBufferConsumer::SetOutputEnabled(const media_source &source,
								  const media_destination &destination,
								  bool enabled,
								  void *user_data,
								  int32 *change_tag,
								  void *_reserved_)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_enable_output data;
	
	data.source = source;
	data.destination = destination;
	data.enabled = enabled;
	data.user_data = user_data;
	data.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = data.change_tag;
	
	return write_port(source.port, PRODUCER_ENABLE_OUTPUT, &data, sizeof(data));
}


status_t
BBufferConsumer::RequestFormatChange(const media_source &source,
									 const media_destination &destination,
									 const media_format &to_format,
									 void *user_data,
									 int32 *change_tag,
									 void *_reserved_)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_format_change_requested data;
	
	data.source = source;
	data.destination = destination;
	data.format = to_format;
	data.user_data = user_data;
	data.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = data.change_tag;
	
	return write_port(source.port, PRODUCER_FORMAT_CHANGE_REQUESTED, &data, sizeof(data));
}


status_t
BBufferConsumer::RequestAdditionalBuffer(const media_source &source,
										 BBuffer *prev_buffer,
										 void *_reserved)
{
	CALLED();
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_additional_buffer_requested data;
	
	data.source = source;
	data.prev_buffer = prev_buffer->ID();
	data.prev_time = 0;
	data.has_seek_tag = false;
	//data.prev_tag = 
	
	return write_port(source.port, PRODUCER_ADDITIONAL_BUFFER_REQUESTED, &data, sizeof(data));
}


status_t
BBufferConsumer::RequestAdditionalBuffer(const media_source &source,
										 bigtime_t start_time,
										 void *_reserved)
{
	CALLED();
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_additional_buffer_requested data;
	
	data.source = source;
	data.prev_buffer = 0;
	data.prev_time = start_time;
	data.has_seek_tag = false;
	//data.prev_tag = 
	
	return write_port(source.port, PRODUCER_ADDITIONAL_BUFFER_REQUESTED, &data, sizeof(data));
}


status_t
BBufferConsumer::SetOutputBuffersFor(const media_source &source,
									 const media_destination &destination,
									 BBufferGroup *group,
									 void *user_data,
									 int32 *change_tag,
									 bool will_reclaim,
									 void *_reserved_)
{
	CALLED();

	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	
	xfer_producer_set_buffer_group *data;
	BBuffer **buffers;
	int32 buffer_count;	
	size_t size;
	status_t rv;

	if (group == 0) {
		buffer_count = 0;
	} else {
		if (B_OK != group->CountBuffers(&buffer_count))
			return B_ERROR;
	}
	
	if (buffer_count != 0) {	
		buffers = new BBuffer * [buffer_count];
		if (B_OK != group->GetBufferList(buffer_count,buffers)) {
			delete buffers;
			return B_ERROR;
		}
	} else {
		buffers = NULL;
	}

	size = sizeof(xfer_producer_set_buffer_group) + buffer_count * sizeof(media_buffer_id);
	data = (xfer_producer_set_buffer_group *) malloc(size);
	data->source = source;
	data->destination = destination;
	data->user_data = user_data;
	data->change_tag = NewChangeTag();
	data->buffer_count = buffer_count;
	for (int32 i = 0; i < buffer_count; i++)
		data->buffers[i] = buffers[i]->ID();
	if (change_tag != NULL)
		*change_tag = data->change_tag;
	
	rv = write_port(source.port, PRODUCER_SET_BUFFER_GROUP, data, size);
	free(data);
	delete [] buffers;

	if (rv == B_OK) {	
		if (fDeleteBufferGroup)
			delete fDeleteBufferGroup;
		fDeleteBufferGroup = will_reclaim ? group : NULL;
	}
	return rv;
}


status_t
BBufferConsumer::SendLatencyChange(const media_source &source,
								   const media_destination &destination,
								   bigtime_t my_new_latency,
								   uint32 flags)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_latency_changed data;
	
	data.source = source;
	data.destination = destination;
	data.latency = my_new_latency;
	data.flags = flags;
	
	return write_port(source.port, PRODUCER_LATENCY_CHANGED, &data, sizeof(data));
}

/*************************************************************
 * protected BBufferConsumer
 *************************************************************/

/* virtual */ status_t
BBufferConsumer::HandleMessage(int32 message,
							   const void *rawdata,
							   size_t size)
{
	CALLED();
	switch (message) {
		case CONSUMER_ACCEPT_FORMAT:
		{
			const xfer_consumer_accept_format *data = (const xfer_consumer_accept_format *)rawdata;
			xfer_consumer_accept_format_reply reply;
			reply.format = data->format;
			reply.result = AcceptFormat(data->dest, &reply.format);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_GET_NEXT_INPUT:
		{
			const xfer_consumer_get_next_input *data = (const xfer_consumer_get_next_input *)rawdata;
			xfer_consumer_get_next_input_reply reply;
			reply.cookie = data->cookie;
			reply.result = GetNextInput(&reply.cookie, &reply.input);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_DISPOSE_INPUT_COOKIE:
		{
			const xfer_consumer_dispose_input_cookie *data = (const xfer_consumer_dispose_input_cookie *)rawdata;
			DisposeInputCookie(data->cookie);
			return B_OK;
		}

		case CONSUMER_BUFFER_RECEIVED:
		{
			const xfer_consumer_buffer_received *data = (const xfer_consumer_buffer_received *)rawdata;
			BBuffer *buffer;
			buffer = fBufferCache->GetBuffer(data->buffer);
			buffer->SetHeader(&data->header);
			BufferReceived(buffer);
			return B_OK;
		}

		case CONSUMER_PRODUCER_DATA_STATUS:
		{
			const xfer_consumer_producer_data_status *data = (const xfer_consumer_producer_data_status *)rawdata;
			ProducerDataStatus(data->for_whom, data->status, data->at_performance_time);
			return B_OK;
		}

		case CONSUMER_GET_LATENCY_FOR:
		{
			const xfer_consumer_get_latency_for *data = (const xfer_consumer_get_latency_for *)rawdata;
			xfer_consumer_get_latency_for_reply reply;
			reply.result = GetLatencyFor(data->for_whom, &reply.latency, &reply.timesource);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_CONNECTED:
		{
			const xfer_consumer_connected *data = (const xfer_consumer_connected *)rawdata;
			xfer_consumer_connected_reply reply;
			reply.result = Connected(data->producer, data->where, data->with_format, &reply.input);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}
				
		case CONSUMER_DISCONNECTED:
		{
			const xfer_consumer_disconnected *data = (const xfer_consumer_disconnected *)rawdata;
			Disconnected(data->producer, data->where);
			return B_OK;
		}

		case CONSUMER_FORMAT_CHANGED:
		{
			const xfer_consumer_format_changed *data = (const xfer_consumer_format_changed *)rawdata;
			xfer_consumer_format_changed_reply reply;
			reply.result = FormatChanged(data->producer, data->consumer, data->change_tag, data->format);
			write_port(data->reply_port, 0, &reply, sizeof(reply));

			// XXX is this RequestCompleted() correct?
			xfer_node_request_completed completed;
			completed.info.what = media_request_info::B_FORMAT_CHANGED;
			completed.info.change_tag = data->change_tag;
			completed.info.status = reply.result;
			//completed.info.cookie
			completed.info.user_data = 0;
			completed.info.source = data->producer;
			completed.info.destination = data->consumer;
			completed.info.format = data->format;
			write_port(data->consumer.port, NODE_REQUEST_COMPLETED, &completed, sizeof(completed));
			return B_OK;
		}

		case CONSUMER_SEEK_TAG_REQUESTED:
		{
			const xfer_consumer_seek_tag_requested *data = (const xfer_consumer_seek_tag_requested *)rawdata;
			xfer_consumer_seek_tag_requested_reply reply;
			reply.result = SeekTagRequested(data->destination, data->target_time, data->flags, &reply.seek_tag, &reply.tagged_time, &reply.flags);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

	};
	return B_ERROR;
}

status_t
BBufferConsumer::SeekTagRequested(const media_destination &destination,
								  bigtime_t in_target_time,
								  uint32 in_flags,
								  media_seek_tag *out_seek_tag,
								  bigtime_t *out_tagged_time,
								  uint32 *out_flags)
{
	CALLED();
	// may be implemented by derived classes
	return B_ERROR;
}

/*************************************************************
 * private BBufferConsumer
 *************************************************************/

/*
not implemented:
BBufferConsumer::BBufferConsumer()
BBufferConsumer::BBufferConsumer(const BBufferConsumer &clone)
BBufferConsumer & BBufferConsumer::operator=(const BBufferConsumer &clone)
*/

/* deprecated function for R4 */
/* static */ status_t
BBufferConsumer::SetVideoClippingFor(const media_source &output,
									 const int16 *shorts,
									 int32 short_count,
									 const media_video_display_info &display,
									 int32 *change_tag)
{
	CALLED();
	if (output == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (short_count > int(B_MEDIA_MESSAGE_SIZE - sizeof(xfer_producer_video_clipping_changed)) / 2)
		debugger("BBufferConsumer::SetVideoClippingFor short_count too large (8000 limit)\n");
		
	xfer_producer_video_clipping_changed *data;
	size_t size;
	status_t rv;

	size = sizeof(xfer_producer_video_clipping_changed) + short_count * 2;
	data = (xfer_producer_video_clipping_changed *) malloc(size);
	data->source = output;
	data->destination = media_destination::null;
	data->display = display;
	data->user_data = 0;
	data->change_tag = NewChangeTag();
	data->short_count = short_count;
	memcpy(data->shorts, shorts, short_count * 2);
	if (change_tag != NULL)
		*change_tag = data->change_tag;
	
	rv = write_port(output.port, PRODUCER_VIDEO_CLIPPING_CHANGED, data, size);
	free(data);
	return rv;
}


/* deprecated function for R4 */
/* static */ status_t
BBufferConsumer::RequestFormatChange(const media_source &source,
									 const media_destination &destination,
									 media_format *in_to_format,
									 int32 *change_tag)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_format_change_requested data;
	
	data.source = source;
	data.destination = destination;
	data.format = *in_to_format;
	data.user_data = 0;
	data.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = data.change_tag;
	
	return write_port(source.port, PRODUCER_FORMAT_CHANGE_REQUESTED, &data, sizeof(data));
}


/* deprecated function for R4 */
/* static */ status_t
BBufferConsumer::SetOutputEnabled(const media_source &source,
								  bool enabled,
								  int32 *change_tag)
{
	CALLED();
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_enable_output data;
	
	data.source = source;
	data.destination = media_destination::null;
	data.enabled = enabled;
	data.user_data = 0;
	data.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = data.change_tag;
	
	return write_port(source.port, PRODUCER_ENABLE_OUTPUT, &data, sizeof(data));
}


status_t BBufferConsumer::_Reserved_BufferConsumer_0(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_1(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_2(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_3(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_4(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_5(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_6(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_7(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_8(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_9(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_10(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_11(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_12(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_13(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_14(void *) { return B_ERROR; }
status_t BBufferConsumer::_Reserved_BufferConsumer_15(void *) { return B_ERROR; }

