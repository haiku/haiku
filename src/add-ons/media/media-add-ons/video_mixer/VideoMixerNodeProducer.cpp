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
// implemention of BBufferProducer
// -------------------------------------------------------- //

// They are asking us to make the first offering.
// So, we get a fresh format and then add requirements
status_t VideoMixerNode::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::FormatSuggestionRequested\n");

	if (format->type == B_MEDIA_NO_TYPE) {
		format->type = B_MEDIA_RAW_VIDEO;
	}

	if (format->type != B_MEDIA_RAW_VIDEO) {
		return B_MEDIA_BAD_FORMAT;
	}

	GetOutputFormat(format);

	return B_OK;
}

// They made an offer to us.  We should make sure that the offer is
// acceptable, and then we can add any requirements we have on top of
// that.  We leave wildcards for anything that we don't care about.
status_t VideoMixerNode::FormatProposal(
				const media_source &output_source,
				media_format *format)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::FormatProposal\n");

	fOutput.source = output_source;
	
	// If we have an input then set our output as the same except for color_space
	if (fConnectedInputs.size() > 0) {
		if (fOutput.format.u.raw_video == media_raw_video_format::wildcard) {
			// First proposal
			fOutput.format = fConnectedInputs[0]->format;
			fOutput.format.u.raw_video.display.format = B_NO_COLOR_SPACE;
		} else {
			// Second proposal
			fOutput.format = fConnectedInputs[0]->format;
			fOutput.format.u.raw_video.display.format = B_RGBA32;
		}
	}
	
	*format = fOutput.format;

	return B_OK;
}

// Presumably we have already agreed with them that this format is
// okay.  But just in case, we check the offer. (and complain if it
// is invalid)  Then as the last thing we do, we get rid of any
// remaining wilcards.
status_t VideoMixerNode::FormatChangeRequested(
				const media_source &source,
				const media_destination &destination,
				media_format *io_format,
				int32 * _deprecated_)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::FormatChangeRequested\n");

	if (fOutput.source != source) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	fOutput.destination = destination;
	fOutput.format = *io_format;

	return B_OK;
}

status_t VideoMixerNode::GetNextOutput(	/* cookie starts as 0 */
				int32 *cookie,
				media_output *out_output)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::GetNextOutput (%ld)\n",*cookie);
	
	// only 1 output
	if (*cookie != 0) {
		fprintf(stderr,"<- B_ERROR (no more outputs)\n");
		return B_ERROR;
	}

	*out_output = fOutput;
	*cookie = 1;

	return B_OK;
}

status_t VideoMixerNode::DisposeOutputCookie(int32 cookie)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::DisposeOutputCookie\n");
	// nothing to do since our cookies are part of the vector iterator
	return B_OK;
}

status_t VideoMixerNode::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::SetBufferGroup\n");

	if (fOutput.source != for_source) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	return B_OK;
}

	/* Format of clipping is (as int16-s): <from line> <npairs> <startclip> <endclip>. */
	/* Repeat for each line where the clipping is different from the previous line. */
	/* If <npairs> is negative, use the data from line -<npairs> (there are 0 pairs after */
	/* a negative <npairs>. Yes, we only support 32k*32k frame buffers for clipping. */
	/* Any non-0 field of 'display' means that that field changed, and if you don't support */
	/* that change, you should return an error and ignore the request. Note that the buffer */
	/* offset values do not have wildcards; 0 (or -1, or whatever) are real values and must */
	/* be adhered to. */
status_t VideoMixerNode::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	return BBufferProducer::VideoClippingChanged(for_source, num_shorts, clip_data, display, _deprecated_);
}

status_t VideoMixerNode::GetLatency(
				bigtime_t *out_latency)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::GetLatency\n");
	if (out_latency == NULL) {
		fprintf(stderr,"<- B_BAD_VALUE\n");
		return B_BAD_VALUE;
	}
	
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t VideoMixerNode::PrepareToConnect(
				const media_source &what,
				const media_destination &where,
				media_format *format,
				media_source *out_source,
				char *out_name)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::PrepareToConnect\n");

	if (fOutput.source != what) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}
	
	*out_source = fOutput.source;
	strcpy(out_name, fOutput.name);

	fOutput.destination = where;
	
	return B_OK;
}

void VideoMixerNode::Connect(
				status_t error, 
				const media_source &source,
				const media_destination &destination,
				const media_format &format,
				char *io_name)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::Connect\n");

	if (fOutput.source != source) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}
	
	if (error != B_OK) {
		fprintf(stderr,"<- error already\n");
		fOutput.destination = media_destination::null;
		fOutput.format.u.raw_video = media_raw_video_format::wildcard;
		return;
	}

	// calculate the downstream latency
	// must happen before itr->Connect
	bigtime_t downstreamLatency;
	media_node_id id;
	FindLatencyFor(fOutput.destination, &downstreamLatency, &id);

	// record the agreed upon values
	fOutput.format = format;
	fOutput.destination = destination;
	strcpy(io_name, fOutput.name);

	// compute the internal latency
	// must happen after itr->Connect
	if (fInternalLatency == 0) {
		fInternalLatency = 100; // temporary until we finish computing it
		ComputeInternalLatency();
	}

	// If the downstream latency for this output is larger
	// than our current downstream latency, we have to increase
	// our current downstream latency to be the larger value.
	if (downstreamLatency > fDownstreamLatency) {
		SetEventLatency(fDownstreamLatency + fInternalLatency);
	}
}

void VideoMixerNode::ComputeInternalLatency() {
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::ComputeInternalLatency\n");
	fInternalLatency = 100; // just guess
	fprintf(stderr,"  internal latency guessed = %lld\n",fInternalLatency);
}

void VideoMixerNode::Disconnect(
				const media_source & what,
				const media_destination & where)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::Disconnect\n");

	if (fOutput.source != what) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	if (fOutput.destination != where) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}

	fOutput.destination = media_destination::null;
	GetOutputFormat(&fOutput.format);
}

void VideoMixerNode::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::LateNoticeReceived\n");

	if (fOutput.source != what) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	switch (RunMode()) {
		case B_OFFLINE:
		    // nothing to do
			break;
		case B_RECORDING:
		    // nothing to do
			break;
		case B_INCREASE_LATENCY:
			fInternalLatency += how_much;
			SetEventLatency(fDownstreamLatency + fInternalLatency);
			break;
		case B_DECREASE_PRECISION:
			// XXX: try to catch up by producing buffers faster
			break;
		case B_DROP_DATA:
			// XXX: should we really drop buffers?  just for that output?
			break;
		default:
			fprintf(stderr,"VideoMixerNode::LateNoticeReceived with unexpected run mode.\n");
			break;
	}
}

void VideoMixerNode::EnableOutput(
				const media_source &what,
				bool enabled,
				int32 *_deprecated_)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::EnableOutput\n");

	if (fOutput.source != what) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	status_t status = B_OK;
	if (status != B_OK) {
		fprintf(stderr,"  error in itr->EnableOutput\n");
	}
}

status_t VideoMixerNode::SetPlayRate(
				int32 numer,
				int32 denom)
{
	BBufferProducer::SetPlayRate(numer, denom); // XXX: do something intelligent later
	return B_OK;
}

void VideoMixerNode::AdditionalBufferRequested(			//	used to be Reserved 0
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::AdditionalBufferRequested\n");

	if (fOutput.source != source) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

//	BBuffer * buffer;
//	status_t status = itr->AdditionalBufferRequested(prev_buffer, prev_time, prev_tag);
//	if (status != B_OK) {
//		fprintf(stderr,"  itr->AdditionalBufferRequested returned an error.\n");
//	}
}

void VideoMixerNode::LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags)
{
	fprintf(stderr,"VideoMixerNode(BBufferProducer)::LatencyChanged\n");

	if (fOutput.source != source) {
		// we don't have that output
		fprintf(stderr,"<- B_MEDIA_BAD_SOURCE\n");
		return;
	}

	if (fOutput.destination != destination) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return;
	}
	
	fDownstreamLatency = new_latency;
	SetEventLatency(fDownstreamLatency + fInternalLatency);
	
	// XXX: we may have to recompute the number of buffers that we are using
	// see SetBufferGroup
}
