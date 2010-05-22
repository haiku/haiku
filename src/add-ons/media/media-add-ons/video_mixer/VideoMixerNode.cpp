/*
* Copyright (C) 2009-2010 David McPaul
*
* All rights reserved. Distributed under the terms of the MIT License.
* VideoMixerNode.cpp
*
* The VideoMixerNode class takes in multiple video streams and supplies
* a single stream as the output.
* each stream is converted to the same colourspace and should match 
* either the primary input OR the requested colourspace from the output
* destination.
*
* The first input is considered the primary input
* subsequent input framesize should match the primary input framesize
* The output framerate will be the same as the primary input
*
*/

#include <stdio.h>
#include <string.h>

#include "VideoMixerNode.h"

VideoMixerNode::~VideoMixerNode(void)
{
	fprintf(stderr,"VideoMixerNode::~VideoMixerNode\n");
	// Stop the BMediaEventLooper thread
	Quit();
}

VideoMixerNode::VideoMixerNode(
				const flavor_info *info = 0,
				BMessage *config = 0,
				BMediaAddOn *addOn = 0)
	: BMediaNode("VideoMixerNode"),
  	  BBufferConsumer(B_MEDIA_RAW_VIDEO),	// Raw video buffers in
  	  BBufferProducer(B_MEDIA_RAW_VIDEO),	// Raw video buffers out
	  BMediaEventLooper()
{
	fprintf(stderr,"VideoMixerNode::VideoMixerNode\n");
	// keep our creator around for AddOn calls later
	fAddOn = addOn;
	// NULL out our latency estimates
	fDownstreamLatency = 0;
	fInternalLatency = 0;
	
	// Start with 1 input and 1 output
	ClearInput(&fInitialInput);

	strncpy(fOutput.name,"VideoMixer Output", B_MEDIA_NAME_LENGTH-1);
	fOutput.name[B_MEDIA_NAME_LENGTH-1] = '\0';

	// initialize the output
	fOutput.node = media_node::null;               // until registration
	fOutput.destination = media_destination::null;
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;

	GetOutputFormat(&fOutput.format);

	fInitCheckStatus = B_OK;
}

void VideoMixerNode::NodeRegistered(void)
{
	fprintf(stderr,"VideoMixerNode::NodeRegistered\n");

	// for every node created so far set to this Node;
	for (uint32 i=0;i<fConnectedInputs.size();i++) {
		fConnectedInputs[i]->node = Node();
		fConnectedInputs[i]->destination.id = i;
		fConnectedInputs[i]->destination.port = ControlPort();
	}

	fInitialInput.node = Node();
	fInitialInput.destination.id = fConnectedInputs.size();
	fInitialInput.destination.port = ControlPort();
	
	GetOutputFormat(&fOutput.format);
	fOutput.node = Node();

	// start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();
}

media_input *
VideoMixerNode::CreateInput(uint32 inputID) {
	media_input *input = new media_input();

	ClearInput(input);

	// don't overwrite available space, and be sure to terminate
	sprintf(input->name, "VideoMixer Input %ld", inputID);
	
	return input;
}

void
VideoMixerNode::ClearInput(media_input *input) {

	// initialize the input
	sprintf(input->name, "VideoMixer Input");
	input->node = Node();
	input->source = media_source::null;
	input->destination = media_destination::null;

	GetInputFormat(&input->format);
}

media_input *
VideoMixerNode::GetInput(const media_source &source) {

	vector<media_input *>::iterator each;

	for (each=fConnectedInputs.begin(); each<fConnectedInputs.end(); each++) {
		if ((*each)->source == source) {
			return *each;
		}
	}

	return NULL;
}

media_input *
VideoMixerNode::GetInput(const media_destination &destination) {

	vector<media_input *>::iterator each;

	for (each=fConnectedInputs.begin(); each<fConnectedInputs.end(); each++) {
		if ((*each)->destination == destination) {
			return *each;
		}
	}

	return NULL;
}

media_input *
VideoMixerNode::GetInput(const int32 id) {

	vector<media_input *>::iterator each;

	for (each=fConnectedInputs.begin(); each<fConnectedInputs.end(); each++) {
		if ((*each)->destination.id == id) {
			return *each;
		}
	}

	return NULL;
}

status_t VideoMixerNode::InitCheck(void) const
{
	fprintf(stderr,"VideoMixerNode::InitCheck\n");
	return fInitCheckStatus;
}

status_t VideoMixerNode::GetConfigurationFor(
				BMessage *into_message)
{
	fprintf(stderr,"VideoMixerNode::GetConfigurationFor\n");
	return B_OK;
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn *VideoMixerNode::AddOn(
				int32 *internal_id) const
{
	fprintf(stderr,"VideoMixerNode::AddOn\n");
	// BeBook says this only gets called if we were in an add-on.
	if (fAddOn != NULL) {
		// If we get a null pointer then we just won't write.
		if (internal_id != NULL) {
			internal_id = 0;
		}
	}
	return fAddOn;
}

void VideoMixerNode::Start(bigtime_t performance_time)
{
	fprintf(stderr,"VideoMixerNode::Start(pt=%lld)\n", performance_time);
	BMediaEventLooper::Start(performance_time);
}

void VideoMixerNode::Stop(
				bigtime_t performance_time,
				bool immediate)
{
	if (immediate) {
		fprintf(stderr,"VideoMixerNode::Stop(pt=%lld,<immediate>)\n", performance_time);
	} else {
		fprintf(stderr,"VideoMixerNode::Stop(pt=%lld,<scheduled>)\n", performance_time);
	}
	BMediaEventLooper::Stop(performance_time, immediate);
}

void VideoMixerNode::Seek(
				bigtime_t media_time,
				bigtime_t performance_time)
{
	fprintf(stderr,"VideoMixerNode::Seek(mt=%lld,pt=%lld)\n", media_time,performance_time);
	BMediaEventLooper::Seek(media_time, performance_time);
}

void VideoMixerNode::SetRunMode(run_mode mode)
{
	fprintf(stderr,"VideoMixerNode::SetRunMode(%i)\n", mode);
	BMediaEventLooper::SetRunMode(mode);
}

void VideoMixerNode::TimeWarp(
				bigtime_t at_real_time,
				bigtime_t to_performance_time)
{
	fprintf(stderr,"VideoMixerNode::TimeWarp(rt=%lld,pt=%lld)\n", at_real_time, to_performance_time);
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

void VideoMixerNode::Preroll(void)
{
	fprintf(stderr,"VideoMixerNode::Preroll\n");
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

void VideoMixerNode::SetTimeSource(BTimeSource *time_source)
{
	fprintf(stderr,"VideoMixerNode::SetTimeSource\n");
	BMediaNode::SetTimeSource(time_source);
}

status_t VideoMixerNode::HandleMessage(
				int32 message,
				const void *data,
				size_t size)
{
	fprintf(stderr,"VideoMixerNode::HandleMessage\n");
	status_t status = B_OK;
	switch (message) {
		// no special messages for now
		default:
			status = BBufferConsumer::HandleMessage(message, data, size);
			if (status == B_OK) {
				break;
			}
			status = BBufferProducer::HandleMessage(message, data, size);
			if (status == B_OK) {
				break;
			}
			status = BMediaNode::HandleMessage(message, data, size);
			if (status == B_OK) {
				break;
			}
			BMediaNode::HandleBadMessage(message, data, size);
			status = B_ERROR;
			break;
	}
	return status;
}

status_t VideoMixerNode::RequestCompleted(const media_request_info &info)
{
	fprintf(stderr,"VideoMixerNode::RequestCompleted\n");
	return BMediaNode::RequestCompleted(info);
}

status_t VideoMixerNode::DeleteHook(BMediaNode *node)
{
	fprintf(stderr,"VideoMixerNode::DeleteHook\n");
	return BMediaEventLooper::DeleteHook(node);
}

status_t VideoMixerNode::GetNodeAttributes(
				media_node_attribute *outAttributes,
				size_t inMaxCount)
{
	fprintf(stderr,"VideoMixerNode::GetNodeAttributes\n");
	return BMediaNode::GetNodeAttributes(outAttributes, inMaxCount);
}

status_t VideoMixerNode::AddTimer(
					bigtime_t at_performance_time,
					int32 cookie)
{
	fprintf(stderr,"VideoMixerNode::AddTimer\n");
	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

// -------------------------------------------------------- //
// VideoMixerNode specific functions
// -------------------------------------------------------- //

// public:

void VideoMixerNode::GetFlavor(flavor_info *outInfo, int32 id)
{
	fprintf(stderr,"VideoMixerNode::GetFlavor\n");

	if (outInfo != NULL) {
		outInfo->internal_id = id;
		outInfo->name = "Haiku VideoMixer";
		outInfo->info = "A VideoMixerNode node mixes multiple video streams into a single stream.";
		outInfo->kinds = B_BUFFER_CONSUMER | B_BUFFER_PRODUCER;
		outInfo->flavor_flags = B_FLAVOR_IS_LOCAL;
		outInfo->possible_count = INT_MAX;	// no limit
		outInfo->in_format_count = 1;
		media_format *inFormats = new media_format[outInfo->in_format_count];
		GetInputFormat(&inFormats[0]);
		outInfo->in_formats = inFormats;
		outInfo->out_format_count = 1; // single output
		media_format *outFormats = new media_format[outInfo->out_format_count];
		GetOutputFormat(&outFormats[0]);
		outInfo->out_formats = outFormats;
	}
}

void VideoMixerNode::GetInputFormat(media_format *outFormat)
{
	fprintf(stderr,"VideoMixerNode::GetInputFormat\n");

	if (outFormat != NULL) {
		outFormat->type = B_MEDIA_RAW_VIDEO;
		outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		outFormat->u.raw_video = media_raw_video_format::wildcard;
	}
}

void VideoMixerNode::GetOutputFormat(media_format *outFormat)
{
	fprintf(stderr,"VideoMixerNode::GetOutputFormat\n");
	if (outFormat != NULL) {
		outFormat->type = B_MEDIA_RAW_VIDEO;
		outFormat->require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		outFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;	
		outFormat->u.raw_video = media_raw_video_format::wildcard;
	}
}

// protected:

status_t VideoMixerNode::AddRequirements(media_format *format)
{
	fprintf(stderr,"VideoMixerNode::AddRequirements\n");
	return B_OK;
}
