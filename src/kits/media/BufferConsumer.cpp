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
#define DEBUG 3
#include "debug.h"
#include "DataExchange.h"
#include "ServerInterface.h"
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

	xfer_producer_late_notice_received request;
	request.source = what_source;
	request.how_much = how_much;
	request.performance_time = performance_time;
	
	write_port(what_source.port, PRODUCER_LATE_NOTICE_RECEIVED, &request, sizeof(request));
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
		
	xfer_producer_video_clipping_changed *request;
	size_t size;
	status_t rv;

	size = sizeof(xfer_producer_video_clipping_changed) + short_count * 2;
	request = (xfer_producer_video_clipping_changed *) malloc(size);
	request->source = output;
	request->destination = destination;
	request->display = display;
	request->user_data = user_data;
	request->change_tag = NewChangeTag();
	request->short_count = short_count;
	memcpy(request->shorts, shorts, short_count * 2);
	if (change_tag != NULL)
		*change_tag = request->change_tag;
	
	rv = write_port(output.port, PRODUCER_VIDEO_CLIPPING_CHANGED, request, size);
	free(request);
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

	xfer_producer_enable_output request;
	
	request.source = source;
	request.destination = destination;
	request.enabled = enabled;
	request.user_data = user_data;
	request.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = request.change_tag;
	
	return write_port(source.port, PRODUCER_ENABLE_OUTPUT, &request, sizeof(request));
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

	xfer_producer_format_change_requested request;
	
	request.source = source;
	request.destination = destination;
	request.format = to_format;
	request.user_data = user_data;
	request.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = request.change_tag;
	
	return write_port(source.port, PRODUCER_FORMAT_CHANGE_REQUESTED, &request, sizeof(request));
}


status_t
BBufferConsumer::RequestAdditionalBuffer(const media_source &source,
										 BBuffer *prev_buffer,
										 void *_reserved)
{
	CALLED();
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_additional_buffer_requested request;
	
	request.source = source;
	request.prev_buffer = prev_buffer->ID();
	request.prev_time = 0;
	request.has_seek_tag = false;
	//request.prev_tag = 
	
	return write_port(source.port, PRODUCER_ADDITIONAL_BUFFER_REQUESTED, &request, sizeof(request));
}


status_t
BBufferConsumer::RequestAdditionalBuffer(const media_source &source,
										 bigtime_t start_time,
										 void *_reserved)
{
	CALLED();
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;

	xfer_producer_additional_buffer_requested request;
	
	request.source = source;
	request.prev_buffer = 0;
	request.prev_time = start_time;
	request.has_seek_tag = false;
	//request.prev_tag = 
	
	return write_port(source.port, PRODUCER_ADDITIONAL_BUFFER_REQUESTED, &request, sizeof(request));
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
	
	xfer_producer_set_buffer_group *request;
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
	request = (xfer_producer_set_buffer_group *) malloc(size);
	request->source = source;
	request->destination = destination;
	request->user_data = user_data;
	request->change_tag = NewChangeTag();
	request->buffer_count = buffer_count;
	for (int32 i = 0; i < buffer_count; i++)
		request->buffers[i] = buffers[i]->ID();
	if (change_tag != NULL)
		*change_tag = request->change_tag;
	
	rv = write_port(source.port, PRODUCER_SET_BUFFER_GROUP, request, size);
	free(request);
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

	xfer_producer_latency_changed request;
	
	request.source = source;
	request.destination = destination;
	request.latency = my_new_latency;
	request.flags = flags;
	
	return write_port(source.port, PRODUCER_LATENCY_CHANGED, &request, sizeof(request));
}

/*************************************************************
 * protected BBufferConsumer
 *************************************************************/

/* virtual */ status_t
BBufferConsumer::HandleMessage(int32 message,
							   const void *rawdata,
							   size_t size)
{
//	CALLED();
	TRACE("BBufferConsumer::HandleMessage %#lx, node %ld\n", message, ID());
	status_t rv;
	switch (message) {
		case CONSUMER_ACCEPT_FORMAT:
		{
			const consumer_accept_format_request *request = (const consumer_accept_format_request *)rawdata;
			consumer_accept_format_reply reply;
			reply.format = request->format;
			rv = AcceptFormat(request->dest, &reply.format);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_GET_NEXT_INPUT:
		{
			const consumer_get_next_input_request *request = (const consumer_get_next_input_request *)rawdata;
			consumer_get_next_input_reply reply;
			reply.cookie = request->cookie;
			rv = GetNextInput(&reply.cookie, &reply.input);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_DISPOSE_INPUT_COOKIE:
		{
			const consumer_dispose_input_cookie_request *request = (const consumer_dispose_input_cookie_request *)rawdata;
			consumer_dispose_input_cookie_reply reply;
			DisposeInputCookie(request->cookie);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_BUFFER_RECEIVED:
		{
			const xfer_consumer_buffer_received *request = (const xfer_consumer_buffer_received *)rawdata;
			BBuffer *buffer;
			buffer = fBufferCache->GetBuffer(request->buffer);
			buffer->SetHeader(&request->header);
			BufferReceived(buffer);
			return B_OK;
		}

		case CONSUMER_PRODUCER_DATA_STATUS:
		{
			const xfer_consumer_producer_data_status *request = (const xfer_consumer_producer_data_status *)rawdata;
			ProducerDataStatus(request->for_whom, request->status, request->at_performance_time);
			return B_OK;
		}

		case CONSUMER_GET_LATENCY_FOR:
		{
			const xfer_consumer_get_latency_for *request = (const xfer_consumer_get_latency_for *)rawdata;
			xfer_consumer_get_latency_for_reply reply;
			reply.result = GetLatencyFor(request->for_whom, &reply.latency, &reply.timesource);
			write_port(request->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_CONNECTED:
		{
			const consumer_connected_request *request = (const consumer_connected_request *)rawdata;
			consumer_connected_reply reply;
			rv = Connected(request->producer, request->where, request->with_format, &reply.input);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}
				
		case CONSUMER_DISCONNECTED:
		{
			const consumer_disconnected_request *request = (const consumer_disconnected_request *)rawdata;
			consumer_disconnected_reply reply;
			Disconnected(request->source, request->destination);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case CONSUMER_FORMAT_CHANGED:
		{
			const xfer_consumer_format_changed *request = (const xfer_consumer_format_changed *)rawdata;
			xfer_consumer_format_changed_reply reply;
			reply.result = FormatChanged(request->producer, request->consumer, request->change_tag, request->format);
			write_port(request->reply_port, 0, &reply, sizeof(reply));

			// XXX is this RequestCompleted() correct?
			xfer_node_request_completed completed;
			completed.info.what = media_request_info::B_FORMAT_CHANGED;
			completed.info.change_tag = request->change_tag;
			completed.info.status = reply.result;
			//completed.info.cookie
			completed.info.user_data = 0;
			completed.info.source = request->producer;
			completed.info.destination = request->consumer;
			completed.info.format = request->format;
			write_port(request->consumer.port, NODE_REQUEST_COMPLETED, &completed, sizeof(completed));
			return B_OK;
		}

		case CONSUMER_SEEK_TAG_REQUESTED:
		{
			const xfer_consumer_seek_tag_requested *request = (const xfer_consumer_seek_tag_requested *)rawdata;
			xfer_consumer_seek_tag_requested_reply reply;
			reply.result = SeekTagRequested(request->destination, request->target_time, request->flags, &reply.seek_tag, &reply.tagged_time, &reply.flags);
			write_port(request->reply_port, 0, &reply, sizeof(reply));
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
		
	xfer_producer_video_clipping_changed *request;
	size_t size;
	status_t rv;

	size = sizeof(xfer_producer_video_clipping_changed) + short_count * 2;
	request = (xfer_producer_video_clipping_changed *) malloc(size);
	request->source = output;
	request->destination = media_destination::null;
	request->display = display;
	request->user_data = 0;
	request->change_tag = NewChangeTag();
	request->short_count = short_count;
	memcpy(request->shorts, shorts, short_count * 2);
	if (change_tag != NULL)
		*change_tag = request->change_tag;
	
	rv = write_port(output.port, PRODUCER_VIDEO_CLIPPING_CHANGED, request, size);
	free(request);
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

	xfer_producer_format_change_requested request;
	
	request.source = source;
	request.destination = destination;
	request.format = *in_to_format;
	request.user_data = 0;
	request.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = request.change_tag;
	
	return write_port(source.port, PRODUCER_FORMAT_CHANGE_REQUESTED, &request, sizeof(request));
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

	xfer_producer_enable_output request;
	
	request.source = source;
	request.destination = media_destination::null;
	request.enabled = enabled;
	request.user_data = 0;
	request.change_tag = NewChangeTag();
	if (change_tag != NULL)
		*change_tag = request.change_tag;
	
	return write_port(source.port, PRODUCER_ENABLE_OUTPUT, &request, sizeof(request));
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

