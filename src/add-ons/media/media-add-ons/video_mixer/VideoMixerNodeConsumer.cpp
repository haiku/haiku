/*
* Copyright (C) 2009-2010 David McPaul
*
* All rights reserved. Distributed under the terms of the MIT License.
* VideoMixerNode.cpp
*
* The VideoMixerNode class
* takes in multiple video streams and supplies
* a single stream as the output.
* each stream is converted to the same colourspace
*/

#include "VideoMixerNode.h"


// -------------------------------------------------------- //
// implemention of BBufferConsumer
// -------------------------------------------------------- //

// Check to make sure the format is okay, then remove
// any wildcards corresponding to our requirements.
status_t VideoMixerNode::AcceptFormat(
				const media_destination &dest,
				media_format *format)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::AcceptFormat\n");
	
	if (fInitialInput.destination != dest) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION");
		return B_MEDIA_BAD_DESTINATION; // none of our inputs matched the dest
	}
	
	media_format myFormat;

	GetInputFormat(&myFormat);

	AddRequirements(format);

	return B_OK;	
}

status_t VideoMixerNode::GetNextInput(
				int32 *cookie,
				media_input *out_input)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::GetNextInput (%ld)\n",*cookie);
	
	// Cookie 0 is the connecting input, all others are connected inputs
	if (*cookie == fConnectedInputs.size()) {
		*out_input = fInitialInput;
	} else {
		out_input = GetInput(*cookie);
		
		if (out_input == NULL) {
			fprintf(stderr,"<- B_ERROR (no more inputs)\n");
			return B_ERROR;
		}
	}
	
	// so next time they won't get the same input again
	(*cookie)++;

	return B_OK;
}

void VideoMixerNode::DisposeInputCookie(
				int32 cookie)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::DisposeInputCookie\n");
	// nothing to do since our cookies are just integers
}

void VideoMixerNode::BufferReceived(BBuffer *buffer)
{
	switch (buffer->Header()->type) {
//		case B_MEDIA_PARAMETERS:
//			{
//			status_t status = ApplyParameterData(buffer->Data(),buffer->SizeUsed());
//			if (status != B_OK) {
//				fprintf(stderr,"ApplyParameterData in MediaDemultiplexerNode::BufferReceived failed\n");
//			}			
//			buffer->Recycle();
//			}
//			break;
		case B_MEDIA_RAW_VIDEO:
			if (buffer->Flags() & BBuffer::B_SMALL_BUFFER) {
				fprintf(stderr,"NOT IMPLEMENTED: B_SMALL_BUFFER in VideoMixerNode::BufferReceived\n");
				// XXX: implement this part
				buffer->Recycle();			
			} else {
				media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
										buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				status_t status = EventQueue()->AddEvent(event);
				if (status != B_OK) {
					fprintf(stderr,"EventQueue()->AddEvent(event) in VideoMixerNode::BufferReceived failed\n");
					buffer->Recycle();
				}
			}
			break;
		default: 
			fprintf(stderr,"unexpected buffer type in VideoMixerNode::BufferReceived\n");
			buffer->Recycle();
			break;
	}
}

void VideoMixerNode::ProducerDataStatus(
				const media_destination &for_whom,
				int32 status,
				bigtime_t at_performance_time)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::ProducerDataStatus\n");
	media_input *input = GetInput(for_whom);
	
	if (input == NULL) {
		fprintf(stderr,"invalid destination received in VideoMixerNode::ProducerDataStatus\n");
		return;
	}
	
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS,
			&input, BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
	EventQueue()->AddEvent(event);	
}

status_t VideoMixerNode::GetLatencyFor(
				const media_destination &for_whom,
				bigtime_t *out_latency,
				media_node_id *out_timesource)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::GetLatencyFor\n");
	if ((out_latency == 0) || (out_timesource == 0)) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	media_input *input = GetInput(for_whom);
	
	if (input == NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	*out_latency = EventLatency();
	*out_timesource = TimeSource()->ID();
	
	return B_OK;
}

status_t VideoMixerNode::Connected(
				const media_source &producer,	/* here's a good place to request buffer group usage */
				const media_destination &where,
				const media_format &with_format,
				media_input *out_input)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::Connected\n");
	
	if (fInitialInput.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	media_input *input = CreateInput(fConnectedInputs.size());
	fConnectedInputs.push_back(input);

	// Specialise the output?

	// compute the latency or just guess
	fInternalLatency = 500; // just a guess
	fprintf(stderr,"  internal latency guessed = %lld\n", fInternalLatency);
	
	SetEventLatency(fInternalLatency);

	// record the agreed upon values
	input->destination = where;
	input->source = producer;
	input->format = with_format;
	
	*out_input = *input;
	
	// Reset the Initial Input
	ClearInput(&fInitialInput);
	fInitialInput.destination.id = fConnectedInputs.size();
	fInitialInput.destination.port = ControlPort();

	return B_OK;
}

void VideoMixerNode::Disconnected(
				const media_source &producer,
				const media_destination &where)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::Disconnected\n");

	media_input *input = GetInput(where);
	
	if (input == NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}

	if (input->source != producer) {
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	
	bufferMixer.RemoveBuffer(input->destination.id);
	
	// disconnected but not deleted (important)
	input->source = media_source::null;
	GetInputFormat(&input->format);
}

/* The notification comes from the upstream producer, so he's already cool with */
/* the format; you should not ask him about it in here. */
status_t VideoMixerNode::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::FormatChanged\n");
	
	media_input *input = GetInput(producer);
	
	if (input == NULL) {
		return B_MEDIA_BAD_SOURCE;
	}

	if (input->destination != consumer) {
		return B_MEDIA_BAD_DESTINATION;
	}
	
	input->format = format;
	return B_OK;
}

/* Given a performance time of some previous buffer, retrieve the remembered tag */
/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
/* idea being that flags can be added later, and the understood flags returned in */
/* *out_flags. */
status_t VideoMixerNode::SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags)
{
	fprintf(stderr,"VideoMixerNode(BBufferConsumer)::SeekTagRequested\n");
	// XXX: implement this
	return BBufferConsumer::SeekTagRequested(destination,in_target_time, in_flags,
											out_seek_tag, out_tagged_time, out_flags);
}
