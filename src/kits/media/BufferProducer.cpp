/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <BufferProducer.h>
#include <BufferConsumer.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include "debug.h"
#include "MediaMisc.h"
#include "DataExchange.h"

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
	fInitialFlags(0),
	fDelay(0)
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
	CALLED();
	// The default implementation of GetLatency() finds the maximum
	// latency of your currently-available outputs by iterating over 
	// them, and returns that value in outLatency
	
	int32 cookie;
	bigtime_t latency;
	media_output output;
	media_node_id unused;
	
	*out_lantency = 0;
	cookie = 0;
	while (B_OK == GetNextOutput(&cookie, &output)) {
		
		if (output.destination == media_destination::null)
			continue;
		
		if (output.node.node == fNodeID) { // avoid port writes (deadlock) if loopback connection
			if (!fConsumerThis)
				fConsumerThis = dynamic_cast<BBufferConsumer *>(this);
			if (!fConsumerThis)
				continue;
			latency = 0;
			if (B_OK == fConsumerThis->GetLatencyFor(output.destination, &latency, &unused)) {
				if (latency > *out_lantency) {
					*out_lantency = latency;
				}
			}
		} else {
			if (B_OK == FindLatencyFor(output.destination, &latency, &unused)) {
				if (latency > *out_lantency) {
					*out_lantency = latency;
				}
			}
		}
	}
	printf("BBufferProducer::GetLatency: node %ld, name \"%s\" has max latency %Ld\n", fNodeID, fName, *out_lantency);
	return B_OK;
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
							   const void *data,
							   size_t size)
{
	PRINT(4, "BBufferProducer::HandleMessage %#lx, node %ld\n", message, fNodeID);
	status_t rv;
	switch (message) {
		case PRODUCER_SET_RUN_MODE_DELAY:
		{
			const producer_set_run_mode_delay_command *command = static_cast<const producer_set_run_mode_delay_command *>(data);
			// when changing this, also change NODE_SET_RUN_MODE
			fDelay = command->delay;
			fRunMode = command->mode;
			TRACE("PRODUCER_SET_RUN_MODE_DELAY: fDelay now %Ld\n", fDelay);
			SetRunMode(fRunMode);
			return B_OK;
		}

		case PRODUCER_FORMAT_SUGGESTION_REQUESTED:
		{
			const producer_format_suggestion_requested_request *request = static_cast<const producer_format_suggestion_requested_request *>(data);
			producer_format_suggestion_requested_reply reply;
			rv = FormatSuggestionRequested(request->type, request->quality, &reply.format);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_FORMAT_PROPOSAL:
		{
			const producer_format_proposal_request *request = static_cast<const producer_format_proposal_request *>(data);
			producer_format_proposal_reply reply;
			reply.format = request->format;
			rv = FormatProposal(request->output, &reply.format);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_PREPARE_TO_CONNECT:
		{
			const producer_prepare_to_connect_request *request = static_cast<const producer_prepare_to_connect_request *>(data);
			producer_prepare_to_connect_reply reply;
			reply.format = request->format;
			reply.out_source = request->source;
			memcpy(reply.name, request->name, B_MEDIA_NAME_LENGTH);
			rv = PrepareToConnect(request->source, request->destination, &reply.format, &reply.out_source, reply.name);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_CONNECT:
		{
			const producer_connect_request *request = static_cast<const producer_connect_request *>(data);
			producer_connect_reply reply;
			memcpy(reply.name, request->name, B_MEDIA_NAME_LENGTH);
			Connect(request->error, request->source, request->destination, request->format, reply.name);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISCONNECT:
		{
			const producer_disconnect_request *request = static_cast<const producer_disconnect_request *>(data);
			producer_disconnect_reply reply;
			Disconnect(request->source, request->destination);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_INITIAL_LATENCY:
		{
			const producer_get_initial_latency_request *request = static_cast<const producer_get_initial_latency_request *>(data);
			producer_get_initial_latency_reply reply;
			reply.initial_latency = fInitialLatency;
			reply.flags = fInitialFlags;
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_SET_PLAY_RATE:
		{
			const producer_set_play_rate_request *request = static_cast<const producer_set_play_rate_request *>(data);
			producer_set_play_rate_reply reply;
			rv = SetPlayRate(request->numer, request->denom);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_LATENCY:
		{
			const producer_get_latency_request *request = static_cast<const producer_get_latency_request *>(data);
			producer_get_latency_reply reply;
			rv = GetLatency(&reply.latency);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_NEXT_OUTPUT:
		{
			const producer_get_next_output_request *request = static_cast<const producer_get_next_output_request *>(data);
			producer_get_next_output_reply reply;
			reply.cookie = request->cookie;
			rv = GetNextOutput(&reply.cookie, &reply.output);
			request->SendReply(rv, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISPOSE_OUTPUT_COOKIE:
		{
			const producer_dispose_output_cookie_request *request = static_cast<const producer_dispose_output_cookie_request *>(data);
			producer_dispose_output_cookie_reply reply;
			DisposeOutputCookie(request->cookie);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_SET_BUFFER_GROUP:
		{
			const producer_set_buffer_group_command *command = static_cast<const producer_set_buffer_group_command *>(data);
			node_request_completed_command replycommand;
			BBufferGroup *group;
			group = command->buffer_count != 0 ? new BBufferGroup(command->buffer_count, command->buffers) : NULL;
			rv = SetBufferGroup(command->source, group);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what = media_request_info::B_SET_OUTPUT_BUFFERS_FOR;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = rv;
			replycommand.info.cookie = (int32)group;
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED, &replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_FORMAT_CHANGE_REQUESTED:
		{
			const producer_format_change_requested_command *command = static_cast<const producer_format_change_requested_command *>(data);
			node_request_completed_command replycommand;
			replycommand.info.format = command->format;
			rv = FormatChangeRequested(command->source, command->destination, &replycommand.info.format, NULL);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what = media_request_info::B_REQUEST_FORMAT_CHANGE;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = rv;
			//replycommand.info.cookie
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED, &replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_VIDEO_CLIPPING_CHANGED:
		{
			const producer_video_clipping_changed_command *command = static_cast<const producer_video_clipping_changed_command *>(data);
			node_request_completed_command replycommand;
			rv = VideoClippingChanged(command->source, command->short_count, (int16 *)command->shorts, command->display, NULL);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what = media_request_info::B_SET_VIDEO_CLIPPING_FOR;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = rv;
			//replycommand.info.cookie
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			replycommand.info.format.type = B_MEDIA_RAW_VIDEO;
			replycommand.info.format.u.raw_video.display = command->display;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED, &replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_ADDITIONAL_BUFFER_REQUESTED:
		{
			const producer_additional_buffer_requested_command *command = static_cast<const producer_additional_buffer_requested_command *>(data);
			AdditionalBufferRequested(command->source, command->prev_buffer, command->prev_time, command->has_seek_tag ? &command->prev_tag : NULL);
			return B_OK;
		}
	
		case PRODUCER_LATENCY_CHANGED:
		{
			const producer_latency_changed_command *command = static_cast<const producer_latency_changed_command *>(data);
			LatencyChanged(command->source, command->destination, command->latency, command->flags);
			return B_OK;
		}

		case PRODUCER_LATE_NOTICE_RECEIVED:
		{
			const producer_late_notice_received_command *command = static_cast<const producer_late_notice_received_command *>(data);
			LateNoticeReceived(command->source, command->how_much, command->performance_time);
			return B_OK;
		}

		case PRODUCER_ENABLE_OUTPUT:
		{
			const producer_enable_output_command *command = static_cast<const producer_enable_output_command *>(data);
			node_request_completed_command replycommand;
			EnableOutput(command->source, command->enabled, NULL);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what = media_request_info::B_SET_OUTPUT_ENABLED;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = B_OK;
			//replycommand.info.cookie
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			//replycommand.info.format
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED, &replycommand, sizeof(replycommand));
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

	consumer_buffer_received_command command;
	command.buffer = buffer->ID();
	command.header = *(buffer->Header());
	command.header.buffer = command.buffer; // buffer->ID();
	command.header.destination = destination.id;
	command.header.owner = 0; // XXX fill with "buffer owner info area"
	command.header.start_time += fDelay; // time compensation as set by BMediaRoster::SetProducerRunModeDelay()

	//printf("BBufferProducer::SendBuffer     node %2ld, buffer %2ld, start_time %12Ld with lateness %6Ld\n", ID(), buffer->Header()->buffer, command.header.start_time, TimeSource()->Now() - command.header.start_time);

	return SendToPort(destination.port, CONSUMER_BUFFER_RECEIVED, &command, sizeof(command));
}


status_t
BBufferProducer::SendDataStatus(int32 status,
								const media_destination &destination,
								bigtime_t at_time)
{
	CALLED();
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;
		
	consumer_producer_data_status_command command;
	command.for_whom = destination;
	command.status = status;
	command.at_performance_time = at_time;

	return SendToPort(destination.port, CONSUMER_PRODUCER_DATA_STATUS, &command, sizeof(command));
}


status_t
BBufferProducer::ProposeFormatChange(media_format *format,
									 const media_destination &for_destination)
{
	CALLED();
	if (IS_INVALID_DESTINATION(for_destination))
		return B_MEDIA_BAD_DESTINATION;
		
	consumer_accept_format_request request;
	consumer_accept_format_reply reply;
	status_t rv;
	
	request.dest = for_destination;
	request.format = *format;
	rv = QueryPort(for_destination.port, CONSUMER_ACCEPT_FORMAT, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*format = reply.format;
	return B_OK;
}


status_t
BBufferProducer::ChangeFormat(const media_source &for_source,
							  const media_destination &for_destination,
							  media_format *format)
{
	CALLED();
	if (IS_INVALID_SOURCE(for_source))
		return B_MEDIA_BAD_SOURCE;
	if (IS_INVALID_DESTINATION(for_destination))
		return B_MEDIA_BAD_DESTINATION;
		
	consumer_format_changed_request request;
	consumer_format_changed_reply reply;

	request.producer = for_source;
	request.consumer = for_destination;
	request.format = *format;
	
	// we use a request/reply to make this synchronous
	return QueryPort(for_destination.port, CONSUMER_FORMAT_CHANGED, &request, sizeof(request), &reply, sizeof(reply));
}


status_t
BBufferProducer::FindLatencyFor(const media_destination &for_destination,
								bigtime_t *out_latency,
								media_node_id *out_timesource)
{
	CALLED();
	if (IS_INVALID_DESTINATION(for_destination))
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	consumer_get_latency_for_request request;
	consumer_get_latency_for_reply reply;

	request.for_whom = for_destination;
	
	rv = QueryPort(for_destination.port, CONSUMER_GET_LATENCY_FOR, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*out_latency = reply.latency;
	*out_timesource = reply.timesource;
	return rv;
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
	if (IS_INVALID_DESTINATION(for_destination))
		return B_MEDIA_BAD_DESTINATION;
		
	status_t rv;
	consumer_seek_tag_requested_request request;
	consumer_seek_tag_requested_reply reply;

	request.destination = for_destination;
	request.target_time = in_target_time;
	request.flags = in_flags;
	
	rv = QueryPort(for_destination.port, CONSUMER_SEEK_TAG_REQUESTED, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		return rv;

	*out_tag = reply.seek_tag;
	*out_tagged_time = reply.tagged_time;
	*out_flags = reply.flags;
	return rv;
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

