/*
 * Copyright 2010-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Marcus Overhagen, <Marcus@Overhagen.de>.
 * Distributed under the terms of the MIT License.
 */


#include <Buffer.h>
#include <BufferConsumer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>

#include "MediaDebug.h"
#include "DataExchange.h"
#include "MediaMisc.h"


// #pragma mark - protected BBufferProducer


BBufferProducer::~BBufferProducer()
{
	CALLED();
}


// #pragma mark - public BBufferProducer


/*static*/ status_t
BBufferProducer::ClipDataToRegion(int32 format, int32 size, const void* data,
	BRegion* region)
{
	CALLED();

	if (format != B_CLIP_SHORT_RUNS)
		return B_MEDIA_BAD_CLIP_FORMAT;

	return clip_shorts_to_region((const int16*)data, size / sizeof(int16),
		region);
}


media_type
BBufferProducer::ProducerType()
{
	CALLED();
	return fProducerType;
}


// #pragma mark - protected BBufferProducer


BBufferProducer::BBufferProducer(media_type producer_type)
	:
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
BBufferProducer::VideoClippingChanged(const media_source& source,
	int16 numShorts, int16* clipData, const media_video_display_info& display,
	int32* /*_deprecated_*/)
{
	CALLED();
	// may be implemented by derived classes
	return B_ERROR;
}


status_t
BBufferProducer::GetLatency(bigtime_t* _latency)
{
	CALLED();
	// The default implementation of GetLatency() finds the maximum
	// latency of your currently-available outputs by iterating over
	// them, and returns that value in outLatency

	int32 cookie;
	bigtime_t latency;
	media_output output;
	media_node_id unused;

	*_latency = 0;
	cookie = 0;
	while (GetNextOutput(&cookie, &output) == B_OK) {
		if (output.destination == media_destination::null)
			continue;

		if (output.node.node == fNodeID) {
			// avoid port writes (deadlock) if loopback connection
			if (fConsumerThis == NULL)
				fConsumerThis = dynamic_cast<BBufferConsumer*>(this);
			if (fConsumerThis == NULL)
				continue;

			latency = 0;
			if (fConsumerThis->GetLatencyFor(output.destination, &latency,
					&unused) == B_OK && latency > *_latency) {
				*_latency = latency;
			}
		} else if (FindLatencyFor(output.destination, &latency, &unused)
				== B_OK &&  latency > *_latency) {
			*_latency = latency;
		}
	}
	printf("BBufferProducer::GetLatency: node %" B_PRId32 ", name \"%s\" has "
		"max latency %" B_PRId64 "\n", fNodeID, fName, *_latency);
	return B_OK;
}


status_t
BBufferProducer::SetPlayRate(int32 numer, int32 denom)
{
	CALLED();
	// may be implemented by derived classes
	return B_ERROR;
}


status_t
BBufferProducer::HandleMessage(int32 message, const void* data, size_t size)
{
	PRINT(4, "BBufferProducer::HandleMessage %#lx, node %ld\n", message,
		fNodeID);

	switch (message) {
		case PRODUCER_SET_RUN_MODE_DELAY:
		{
			const producer_set_run_mode_delay_command* command
				= static_cast<const producer_set_run_mode_delay_command*>(data);
			// when changing this, also change NODE_SET_RUN_MODE
			fDelay = command->delay;
			fRunMode = command->mode;

			TRACE("PRODUCER_SET_RUN_MODE_DELAY: fDelay now %" B_PRId64 "\n",
				fDelay);

			SetRunMode(fRunMode);
			return B_OK;
		}

		case PRODUCER_FORMAT_SUGGESTION_REQUESTED:
		{
			const producer_format_suggestion_requested_request* request
				= static_cast<
					const producer_format_suggestion_requested_request*>(data);
			producer_format_suggestion_requested_reply reply;
			status_t status = FormatSuggestionRequested(request->type,
				request->quality, &reply.format);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_FORMAT_PROPOSAL:
		{
			const producer_format_proposal_request* request
				= static_cast<const producer_format_proposal_request*>(data);
			producer_format_proposal_reply reply;
			reply.format = request->format;
			status_t status = FormatProposal(request->output, &reply.format);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_PREPARE_TO_CONNECT:
		{
			const producer_prepare_to_connect_request* request
				= static_cast<const producer_prepare_to_connect_request*>(data);
			producer_prepare_to_connect_reply reply;
			reply.format = request->format;
			reply.out_source = request->source;
			memcpy(reply.name, request->name, B_MEDIA_NAME_LENGTH);
			status_t status = PrepareToConnect(request->source,
				request->destination, &reply.format, &reply.out_source,
				reply.name);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_CONNECT:
		{
			const producer_connect_request* request
				= static_cast<const producer_connect_request*>(data);
			producer_connect_reply reply;
			memcpy(reply.name, request->name, B_MEDIA_NAME_LENGTH);
			Connect(request->error, request->source, request->destination,
				request->format, reply.name);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISCONNECT:
		{
			const producer_disconnect_request* request
				= static_cast<const producer_disconnect_request*>(data);
			producer_disconnect_reply reply;
			Disconnect(request->source, request->destination);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_INITIAL_LATENCY:
		{
			const producer_get_initial_latency_request* request
				= static_cast<
					const producer_get_initial_latency_request*>(data);
			producer_get_initial_latency_reply reply;
			reply.initial_latency = fInitialLatency;
			reply.flags = fInitialFlags;
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_SET_PLAY_RATE:
		{
			const producer_set_play_rate_request* request
				= static_cast<const producer_set_play_rate_request*>(data);
			producer_set_play_rate_reply reply;
			status_t status = SetPlayRate(request->numer, request->denom);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_LATENCY:
		{
			const producer_get_latency_request* request
				= static_cast<const producer_get_latency_request*>(data);
			producer_get_latency_reply reply;
			status_t status = GetLatency(&reply.latency);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_GET_NEXT_OUTPUT:
		{
			const producer_get_next_output_request* request
				= static_cast<const producer_get_next_output_request*>(data);
			producer_get_next_output_reply reply;
			reply.cookie = request->cookie;
			status_t status = GetNextOutput(&reply.cookie, &reply.output);
			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_DISPOSE_OUTPUT_COOKIE:
		{
			const producer_dispose_output_cookie_request*request
				= static_cast<
					const producer_dispose_output_cookie_request*>(data);
			producer_dispose_output_cookie_reply reply;
			DisposeOutputCookie(request->cookie);
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case PRODUCER_SET_BUFFER_GROUP:
		{
			const producer_set_buffer_group_command* command
				= static_cast<const producer_set_buffer_group_command*>(data);
			node_request_completed_command replycommand;
			BBufferGroup *group;
			group = command->buffer_count != 0
				? new BBufferGroup(command->buffer_count, command->buffers)
				: NULL;

			if (group != NULL && group->InitCheck() != B_OK) {
				ERROR("BBufferProducer::HandleMessage PRODUCER_SET_BUFFER_GROUP"
					" group InitCheck() failed.\n");
				delete group;
				group = NULL;
			}
			status_t status = SetBufferGroup(command->source, group);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what
				= media_request_info::B_SET_OUTPUT_BUFFERS_FOR;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = status;
			replycommand.info.cookie = group;
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED,
				&replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_FORMAT_CHANGE_REQUESTED:
		{
			const producer_format_change_requested_command* command
				= static_cast<
					const producer_format_change_requested_command*>(data);
			node_request_completed_command replycommand;
			replycommand.info.format = command->format;
			status_t status = FormatChangeRequested(command->source,
				command->destination, &replycommand.info.format, NULL);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what
				= media_request_info::B_REQUEST_FORMAT_CHANGE;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = status;
			//replycommand.info.cookie
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED,
				&replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_VIDEO_CLIPPING_CHANGED:
		{
			const producer_video_clipping_changed_command* command
				= static_cast<
					const producer_video_clipping_changed_command*>(data);
			node_request_completed_command replycommand;
			status_t status = VideoClippingChanged(command->source,
				command->short_count, (int16 *)command->shorts,
				command->display, NULL);
			if (command->destination == media_destination::null)
				return B_OK;
			replycommand.info.what
				= media_request_info::B_SET_VIDEO_CLIPPING_FOR;
			replycommand.info.change_tag = command->change_tag;
			replycommand.info.status = status;
			//replycommand.info.cookie
			replycommand.info.user_data = command->user_data;
			replycommand.info.source = command->source;
			replycommand.info.destination = command->destination;
			replycommand.info.format.type = B_MEDIA_RAW_VIDEO;
			replycommand.info.format.u.raw_video.display = command->display;
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED,
				&replycommand, sizeof(replycommand));
			return B_OK;
		}

		case PRODUCER_ADDITIONAL_BUFFER_REQUESTED:
		{
			const producer_additional_buffer_requested_command* command
				= static_cast<
					const producer_additional_buffer_requested_command*>(data);
			AdditionalBufferRequested(command->source, command->prev_buffer,
				command->prev_time, command->has_seek_tag
					? &command->prev_tag : NULL);
			return B_OK;
		}

		case PRODUCER_LATENCY_CHANGED:
		{
			const producer_latency_changed_command* command
				= static_cast<const producer_latency_changed_command*>(data);
			LatencyChanged(command->source, command->destination,
				command->latency, command->flags);
			return B_OK;
		}

		case PRODUCER_LATE_NOTICE_RECEIVED:
		{
			const producer_late_notice_received_command* command
				= static_cast<
					const producer_late_notice_received_command*>(data);
			LateNoticeReceived(command->source, command->how_much,
				command->performance_time);
			return B_OK;
		}

		case PRODUCER_ENABLE_OUTPUT:
		{
			const producer_enable_output_command* command
				= static_cast<const producer_enable_output_command*>(data);
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
			SendToPort(command->destination.port, NODE_REQUEST_COMPLETED,
				&replycommand, sizeof(replycommand));
			return B_OK;
		}
	}

	return B_ERROR;
}


void
BBufferProducer::AdditionalBufferRequested(const media_source& source,
	media_buffer_id previousBuffer, bigtime_t previousTime,
	const media_seek_tag* previousTag)
{
	CALLED();
	// may be implemented by derived classes
}


void
BBufferProducer::LatencyChanged(const media_source& source,
	const media_destination& destination, bigtime_t newLatency, uint32 flags)
{
	CALLED();
	// may be implemented by derived classes
}


status_t
BBufferProducer::SendBuffer(BBuffer* buffer, const media_source& source,
	const media_destination& destination)
{
	CALLED();
	if (destination == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	if (source == media_source::null)
		return B_MEDIA_BAD_SOURCE;
	if (buffer == NULL)
		return B_BAD_VALUE;

	consumer_buffer_received_command command;
	command.buffer = buffer->ID();
	command.header = *buffer->Header();
	command.header.buffer = command.buffer;
	command.header.source_port = source.port;
	command.header.source = source.id;
	command.header.destination = destination.id;
	command.header.owner = 0; // XXX fill with "buffer owner info area"
	command.header.start_time += fDelay;
		// time compensation as set by BMediaRoster::SetProducerRunModeDelay()

	//printf("BBufferProducer::SendBuffer     node %2ld, buffer %2ld, start_time %12Ld with lateness %6Ld\n", ID(), buffer->Header()->buffer, command.header.start_time, TimeSource()->Now() - command.header.start_time);

	return SendToPort(destination.port, CONSUMER_BUFFER_RECEIVED, &command,
		sizeof(command));
}


status_t
BBufferProducer::SendDataStatus(int32 status,
	const media_destination& destination, bigtime_t atTime)
{
	CALLED();
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_producer_data_status_command command;
	command.for_whom = destination;
	command.status = status;
	command.at_performance_time = atTime;

	return SendToPort(destination.port, CONSUMER_PRODUCER_DATA_STATUS, &command,
		sizeof(command));
}


status_t
BBufferProducer::ProposeFormatChange(media_format* format,
	const media_destination& destination)
{
	CALLED();
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_accept_format_request request;
	consumer_accept_format_reply reply;

	request.dest = destination;
	request.format = *format;
	status_t status = QueryPort(destination.port, CONSUMER_ACCEPT_FORMAT,
		&request, sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		return status;

	*format = reply.format;
	return B_OK;
}


status_t
BBufferProducer::ChangeFormat(const media_source& source,
	const media_destination& destination, media_format* format)
{
	CALLED();
	if (IS_INVALID_SOURCE(source))
		return B_MEDIA_BAD_SOURCE;
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_format_changed_request request;
	consumer_format_changed_reply reply;

	request.producer = source;
	request.consumer = destination;
	request.format = *format;

	// we use a request/reply to make this synchronous
	return QueryPort(destination.port, CONSUMER_FORMAT_CHANGED, &request,
		sizeof(request), &reply, sizeof(reply));
}


status_t
BBufferProducer::FindLatencyFor(const media_destination& destination,
	bigtime_t* _latency, media_node_id* _timesource)
{
	CALLED();
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_get_latency_for_request request;
	consumer_get_latency_for_reply reply;

	request.for_whom = destination;

	status_t status = QueryPort(destination.port, CONSUMER_GET_LATENCY_FOR,
		&request, sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		return status;

	*_latency = reply.latency;
	*_timesource = reply.timesource;
	return B_OK;
}


status_t
BBufferProducer::FindSeekTag(const media_destination& destination,
	bigtime_t targetTime, media_seek_tag* _tag, bigtime_t* _tagged_time,
	uint32* _flags, uint32 flags)
{
	CALLED();
	if (IS_INVALID_DESTINATION(destination))
		return B_MEDIA_BAD_DESTINATION;

	consumer_seek_tag_requested_request request;
	consumer_seek_tag_requested_reply reply;

	request.destination = destination;
	request.target_time = targetTime;
	request.flags = flags;

	status_t status = QueryPort(destination.port, CONSUMER_SEEK_TAG_REQUESTED,
		&request, sizeof(request), &reply, sizeof(reply));
	if (status != B_OK)
		return status;

	*_tag = reply.seek_tag;
	*_tagged_time = reply.tagged_time;
	*_flags = reply.flags;
	return B_OK;
}


void
BBufferProducer::SetInitialLatency(bigtime_t initialLatency, uint32 flags)
{
	fInitialLatency = initialLatency;
	fInitialFlags = flags;
}


// #pragma mark - private BBufferProducer


/*
private unimplemented
BBufferProducer::BBufferProducer()
BBufferProducer::BBufferProducer(const BBufferProducer &clone)
BBufferProducer & BBufferProducer::operator=(const BBufferProducer &clone)
*/

status_t BBufferProducer::_Reserved_BufferProducer_0(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_1(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_2(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_3(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_4(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_5(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_6(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_7(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_8(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_9(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_10(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_11(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_12(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_13(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_14(void*) { return B_ERROR; }
status_t BBufferProducer::_Reserved_BufferProducer_15(void*) { return B_ERROR; }


//! Deprecated.
status_t
BBufferProducer::SendBuffer(BBuffer* buffer,
	const media_destination& destination)
{
	CALLED();

	// Try to find the source - this is the best we can do
	media_output output;
	int32 cookie = 0;
	status_t status = GetNextOutput(&cookie, &output);
	if (status != B_OK)
		return status;

	return SendBuffer(buffer, output.source, destination);
}


status_t
BBufferProducer::clip_shorts_to_region(const int16* data, int count,
	BRegion* output)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t
BBufferProducer::clip_region_to_shorts(const BRegion* input, int16* data,
	int maxCount, int* _count)
{
	UNIMPLEMENTED();
	return B_ERROR;
}
