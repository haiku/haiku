/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: BufferProducer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <BufferProducer.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include "PortPool.h"
#include "debug.h"
#include "../server/headers/ServerInterface.h"

/*************************************************************
 * protected BBufferProducer
 *************************************************************/

BBufferProducer::~BBufferProducer()
{
	CALLED();
}

/*************************************************************
 * public BBufferProducer
 *************************************************************/

/* static */ status_t
BBufferProducer::ClipDataToRegion(int32 format,
								  int32 size,
								  const void *data,
								  BRegion *region)
{
	CALLED();
	
	if (format != B_CLIP_SHORT_RUNS)
		return B_MEDIA_BAD_CLIP_FORMAT;
	
	return clip_shorts_to_region((const int16 *)data, size / sizeof(int16), region);
}

media_type
BBufferProducer::ProducerType()
{
	CALLED();
	return fProducerType;
}

/*************************************************************
 * protected BBufferProducer
 *************************************************************/

/* explicit */
BBufferProducer::BBufferProducer(media_type producer_type) :
	BMediaNode("called by BBufferProducer"),
	fProducerType(producer_type),
	fInitialLatency(0),
	fInitialFlags(0)
{
	CALLED();

	AddNodeKind(B_BUFFER_PRODUCER);
}


status_t
BBufferProducer::VideoClippingChanged(const media_source &for_source,
									  int16 num_shorts,
									  int16 *clip_data,
									  const media_video_display_info &display,
									  int32 *_deprecated_)
{
	CALLED();
	// may be implemented by derived classes
	return B_ERROR;
}


status_t
BBufferProducer::GetLatency(bigtime_t *out_lantency)
{
	UNIMPLEMENTED();

	// XXX The default implementation of GetLatency() finds the maximum
	// latency of your currently-available outputs by iterating over 
	// them, and returns that value in outLatency

	return B_ERROR;
}


status_t
BBufferProducer::SetPlayRate(int32 numer,
							 int32 denom)
{
	CALLED();
	// may be implemented by derived classes
	return B_ERROR;
}

		
status_t
BBufferProducer::HandleMessage(int32 message,
							   const void *rawdata,
							   size_t size)
{
	CALLED();
	switch (message) {

		case PRODUCER_FORMAT_SUGGESTION_REQUESTED:
		{
			const xfer_producer_format_suggestion_requested *data = (const xfer_producer_format_suggestion_requested *)rawdata;
			xfer_producer_format_suggestion_requested_reply reply;
			reply.result = FormatSuggestionRequested(data->type, data->quality, &reply.format);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_FORMAT_PROPOSAL:
		{
			const xfer_producer_format_proposal *data = (const xfer_producer_format_proposal *)rawdata;
			xfer_producer_format_proposal_reply reply;
			reply.result = FormatProposal(data->output, (media_format *)&data->format);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_PREPARE_TO_CONNECT:
		{
			const xfer_producer_prepare_to_connect *data = (const xfer_producer_prepare_to_connect *)rawdata;
			xfer_producer_prepare_to_connect_reply reply;
			reply.format = data->format;
			reply.name[0] = 0;
			reply.result = PrepareToConnect(data->source, data->destination, &reply.format, &reply.out_source, reply.name);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_CONNECT:
		{
			const xfer_producer_connect *data = (const xfer_producer_connect *)rawdata;
			xfer_producer_connect_reply reply;
			memcpy(reply.name, data->name, B_MEDIA_NAME_LENGTH);
			Connect(data->error, data->source, data->destination, data->format, reply.name);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISCONNECT:
		{
			const xfer_producer_disconnect *data = (const xfer_producer_disconnect *)rawdata;
			Disconnect(data->source, data->destination);
			return B_OK;
		}

		case PRODUCER_GET_INITIAL_LATENCY:
		{
			const xfer_producer_get_initial_latency *data = (const xfer_producer_get_initial_latency *)rawdata;
			xfer_producer_get_initial_latency_reply reply;
			reply.initial_latency = fInitialLatency;
			reply.flags = fInitialFlags;
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_SET_PLAY_RATE:
		{
			const xfer_producer_set_play_rate *data = (const xfer_producer_set_play_rate *)rawdata;
			xfer_producer_set_play_rate_reply reply;
			reply.result = SetPlayRate(data->numer, data->denom);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_LATENCY:
		{
			const xfer_producer_get_latency *data = (const xfer_producer_get_latency *)rawdata;
			xfer_producer_get_latency_reply reply;
			reply.result = GetLatency(&reply.latency);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_NEXT_OUTPUT:
		{
			const xfer_producer_get_next_output *data = (const xfer_producer_get_next_output *)rawdata;
			xfer_producer_get_next_output_reply reply;
			reply.cookie = data->cookie;
			reply.result = GetNextOutput(&reply.cookie, &reply.output);
			write_port(data->reply_port, 0, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISPOSE_OUTPUT_COOKIE:
		{
			const xfer_producer_dispose_output_cookie *data = (const xfer_producer_dispose_output_cookie *)rawdata;
			DisposeOutputCookie(data->cookie);
			return B_OK;
		}

		case PRODUCER_SET_BUFFER_GROUP:
		{
			const xfer_producer_set_buffer_group *data = (const xfer_producer_set_buffer_group *)rawdata;
			xfer_node_request_completed reply;
			BBufferGroup *group;
			status_t rv;
			group = data->buffer_count != 0 ? new BBufferGroup(data->buffer_count, data->buffers) : NULL;
			rv = SetBufferGroup(data->source, group);
			if (data->destination == media_destination::null)
				return B_OK;
			reply.info.what = media_request_info::B_SET_OUTPUT_BUFFERS_FOR;
			reply.info.change_tag = data->change_tag;
			reply.info.status = rv;
			reply.info.cookie = (int32)group;
			reply.info.user_data = data->user_data;
			reply.info.source = data->source;
			reply.info.destination = data->destination;
			write_port(data->destination.port, NODE_REQUEST_COMPLETED, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_FORMAT_CHANGE_REQUESTED:
		{
			const xfer_producer_format_change_requested *data = (const xfer_producer_format_change_requested *)rawdata;
			xfer_node_request_completed reply;
			status_t rv;
			reply.info.format = data->format;
			rv = FormatChangeRequested(data->source, data->destination, &reply.info.format, NULL);
			if (data->destination == media_destination::null)
				return B_OK;
			reply.info.what = media_request_info::B_REQUEST_FORMAT_CHANGE;
			reply.info.change_tag = data->change_tag;
			reply.info.status = rv;
			//reply.info.cookie
			reply.info.user_data = data->user_data;
			reply.info.source = data->source;
			reply.info.destination = data->destination;
			write_port(data->destination.port, NODE_REQUEST_COMPLETED, &reply, sizeof(reply));
			return B_OK;
		}


		case PRODUCER_VIDEO_CLIPPING_CHANGED:
		{
			const xfer_producer_video_clipping_changed *data = (const xfer_producer_video_clipping_changed *)rawdata;
			xfer_node_request_completed reply;
			status_t rv;
			rv = VideoClippingChanged(data->source, data->short_count, (int16 *)data->shorts, data->display, NULL);
			if (data->destination == media_destination::null)
				return B_OK;
			reply.info.what = media_request_info::B_SET_VIDEO_CLIPPING_FOR;
			reply.info.change_tag = data->change_tag;
			reply.info.status = rv;
			//reply.info.cookie
			reply.info.user_data = data->user_data;
			reply.info.source = data->source;
			reply.info.destination = data->destination;
			reply.info.format.type = B_MEDIA_RAW_VIDEO;
			reply.info.format.u.raw_video.display = data->display;
			write_port(data->destination.port, NODE_REQUEST_COMPLETED, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_ADDITIONAL_BUFFER_REQUESTED:
		{
			const xfer_producer_additional_buffer_requested *data = (const xfer_producer_additional_buffer_requested *)rawdata;
			AdditionalBufferRequested(data->source, data->prev_buffer, data->prev_time, data->has_seek_tag ? &data->prev_tag : NULL);
			return B_OK;
		}
	
		case PRODUCER_LATENCY_CHANGED:
		{
			const xfer_producer_latency_changed *data = (const xfer_producer_latency_changed *)rawdata;
			LatencyChanged(data->source, data->destination, data->latency, data->flags);
			return B_OK;
		}

		case PRODUCER_LATE_NOTICE_RECEIVED:
		{
			const xfer_producer_late_notice_received *data = (const xfer_producer_late_notice_received *)rawdata;
			LateNoticeReceived(data->source, data->how_much, data->performance_time);
			return B_OK;
		}

		case PRODUCER_ENABLE_OUTPUT:
		{
			const xfer_producer_enable_output *data = (const xfer_producer_enable_output *)rawdata;
			xfer_node_request_completed reply;
			EnableOutput(data->source, data->enabled, NULL);
			if (data->destination == media_destination::null)
				return B_OK;
			reply.info.what = media_request_info::B_SET_OUTPUT_ENABLED;
			reply.info.change_tag = data->change_tag;
			reply.info.status = B_OK;
			//reply.info.cookie
			reply.info.user_data = data->user_data;
			reply.info.source = data->source;
			reply.info.destination = data->destination;
			//reply.info.format
			write_port(data->destination.port, NODE_REQUEST_COMPLETED, &reply, sizeof(reply));
			return B_OK;
		}
				
	};
	return B_ERROR;
}


void
BBufferProducer::AdditionalBufferRequested(const media_source &source,
										   media_buffer_id prev_buffer,
										   bigtime_t prev_time,
										   const media_seek_tag *prev_tag)
{
	CALLED();
	// may be implemented by derived classes
}


void
BBufferProducer::LatencyChanged(const media_source &source,
								const media_destination &destination,
								bigtime_t new_latency,
								uint32 flags)
{
	CALLED();
	// may be implemented by derived classes
}


status_t
BBufferProducer::SendBuffer(BBuffer *buffer,
							const media_destination &destination)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (buffer == NULL)
		return B_BAD_VALUE;

	xfer_consumer_buffer_received data;
	data.buffer = buffer->ID();
	data.header = *(buffer->Header());
	data.header.buffer = data.buffer;
	data.header.destination = destination.id;

	return write_port(destination.port, CONSUMER_BUFFER_RECEIVED, &data, sizeof(data));
}


status_t
BBufferProducer::SendDataStatus(int32 status,
								const media_destination &destination,
								bigtime_t at_time)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
		
	xfer_consumer_producer_data_status data;
	data.for_whom = destination;
	data.status = status;
	data.at_performance_time = at_time;

	return write_port(destination.port, CONSUMER_PRODUCER_DATA_STATUS, &data, sizeof(data));
}


status_t
BBufferProducer::ProposeFormatChange(media_format *format,
									 const media_destination &for_destination)
{
	CALLED();
	if (for_destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	int32 code;
	xfer_consumer_accept_format data;
	xfer_consumer_accept_format_reply reply;

	data.dest = for_destination;
	data.format = *format;
	data.reply_port = _PortPool->GetPort();
	
	rv = write_port(for_destination.port, CONSUMER_ACCEPT_FORMAT, &data, sizeof(data));
	if (rv != B_OK) {
		_PortPool->PutPort(data.reply_port);
		return rv;
	}

	rv = read_port(data.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(data.reply_port);
	if (rv < B_OK)
		return rv;
	
	*format = reply.format;

	return reply.result;
}


status_t
BBufferProducer::ChangeFormat(const media_source &for_source,
							  const media_destination &for_destination,
							  media_format *format)
{
	CALLED();
	if (for_source == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (for_destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	int32 code;
	xfer_consumer_format_changed data;
	xfer_consumer_format_changed_reply reply;

	data.producer = for_source;
	data.consumer = for_destination;
	data.format = *format;
	data.reply_port = _PortPool->GetPort();
	
	rv = write_port(for_destination.port, CONSUMER_FORMAT_CHANGED, &data, sizeof(data));
	if (rv != B_OK) {
		_PortPool->PutPort(data.reply_port);
		return rv;
	}

	rv = read_port(data.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(data.reply_port);
	if (rv < B_OK)
		return rv;
	
	return reply.result;
}


status_t
BBufferProducer::FindLatencyFor(const media_destination &for_destination,
								bigtime_t *out_latency,
								media_node_id *out_timesource)
{
	CALLED();
	if (for_destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	int32 code;
	xfer_consumer_get_latency_for data;
	xfer_consumer_get_latency_for_reply reply;

	data.for_whom = for_destination;
	data.reply_port = _PortPool->GetPort();
	
	rv = write_port(for_destination.port, CONSUMER_GET_LATENCY_FOR, &data, sizeof(data));
	if (rv != B_OK) {
		_PortPool->PutPort(data.reply_port);
		return rv;
	}

	rv = read_port(data.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(data.reply_port);
	if (rv < B_OK)
		return rv;
	
	*out_latency = reply.latency;
	*out_timesource = reply.timesource;

	return reply.result;
}


status_t
BBufferProducer::FindSeekTag(const media_destination &for_destination,
							 bigtime_t in_target_time,
							 media_seek_tag *out_tag,
							 bigtime_t *out_tagged_time,
							 uint32 *out_flags,
							 uint32 in_flags)
{
	CALLED();
	if (for_destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	int32 code;
	xfer_consumer_seek_tag_requested data;
	xfer_consumer_seek_tag_requested_reply reply;

	data.destination = for_destination;
	data.target_time = in_target_time;
	data.flags = in_flags;
	data.reply_port = _PortPool->GetPort();
	
	rv = write_port(for_destination.port, CONSUMER_SEEK_TAG_REQUESTED, &data, sizeof(data));
	if (rv != B_OK) {
		_PortPool->PutPort(data.reply_port);
		return rv;
	}

	rv = read_port(data.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(data.reply_port);
	if (rv < B_OK)
		return rv;

	*out_tag = reply.seek_tag;
	*out_tagged_time = reply.tagged_time;
	*out_flags = reply.flags;
	
	return reply.result;
}


void
BBufferProducer::SetInitialLatency(bigtime_t inInitialLatency,
								   uint32 flags)
{
	fInitialLatency = inInitialLatency;
	fInitialFlags = flags;
}

/*************************************************************
 * private BBufferProducer
 *************************************************************/

/*
private unimplemented
BBufferProducer::BBufferProducer()
BBufferProducer::BBufferProducer(const BBufferProducer &clone)
BBufferProducer & BBufferProducer::operator=(const BBufferProducer &clone)
*/

status_t BBufferProducer::_Reserved_BufferProducer_0(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_1(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_2(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_3(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_4(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_5(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_6(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_7(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_8(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_9(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_10(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_11(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_12(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_13(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_14(void *) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_15(void *) { return B_ERROR; }


status_t
BBufferProducer::clip_shorts_to_region(const int16 *data,
									   int count,
									   BRegion *output)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t
BBufferProducer::clip_region_to_shorts(const BRegion *input,
									   int16 *data,
									   int max_count,
									   int *out_count)
{
	UNIMPLEMENTED();

	return B_ERROR;
}

