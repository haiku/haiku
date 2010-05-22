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
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void VideoMixerNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	switch (event->type) {
		case BTimedEventQueue::B_START:
			HandleStart(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_SEEK:
			HandleSeek(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_WARP:
			HandleWarp(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			if (RunState() == BMediaEventLooper::B_STARTED) {
				HandleBuffer(event,lateness,realTimeEvent);
			}
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			HandleDataStatus(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_PARAMETER:
			HandleParameter(event,lateness,realTimeEvent);
			break;
		default:
			fprintf(stderr,"  unknown event type: %ld\n",event->type);
			break;
	}
}

/* override to clean up custom events you have added to your queue */
void VideoMixerNode::CleanUpEvent(
				const media_timed_event *event)
{
	BMediaEventLooper::CleanUpEvent(event);
}
		
/* called from Offline mode to determine the current time of the node */
/* update your internal information whenever it changes */
bigtime_t VideoMixerNode::OfflineTime()
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::OfflineTime\n");
	return BMediaEventLooper::OfflineTime();
// XXX: do something else?
}

/* override only if you know what you are doing! */
/* otherwise much badness could occur */
/* the actual control loop function: */
/* 	waits for messages, Pops events off the queue and calls DispatchEvent */
void VideoMixerNode::ControlLoop() {
	BMediaEventLooper::ControlLoop();
}

// protected:

status_t VideoMixerNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleStart()\n");
	if (RunState() != B_STARTED) {
		media_timed_event firstBufferEvent(event->event_time, BTimedEventQueue::B_HANDLE_BUFFER);
		HandleEvent(&firstBufferEvent, 0, false);
		EventQueue()->AddEvent(firstBufferEvent);
	}
	return B_OK;
}

status_t VideoMixerNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleSeek(t=%lld,d=%ld,bd=%lld)\n",event->event_time, event->data, event->bigdata);
	return B_OK;
}
						
status_t VideoMixerNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleWarp\n");
	return B_OK;
}

status_t VideoMixerNode::HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleStop\n");
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
	return B_OK;
}

status_t VideoMixerNode::HandleBuffer(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{	
	if (event->type != BTimedEventQueue::B_HANDLE_BUFFER) {
		fprintf(stderr,"HandleBuffer called on non buffer event type\n");
		return B_BAD_VALUE;
	}
	
	BBuffer *buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);
	if (buffer == NULL) {
		fprintf(stderr,"NO BUFFER PASSED\n");
		return B_BAD_VALUE;
	}
	
	media_input *input = GetInput(buffer->Header()->destination);
	
	if (input == NULL) {
		fprintf(stderr,"<- B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	
	if (fOutput.format.u.raw_video == media_raw_video_format::wildcard) {
		fprintf(stderr,"<- B_MEDIA_NOT_CONNECTED\n");
		return B_MEDIA_NOT_CONNECTED;
	}

	status_t status = B_OK;

	if (input == *fConnectedInputs.begin()) {
		if (bufferMixer.isBufferAvailable()) {
			status = SendBuffer(bufferMixer.GetOutputBuffer(), fOutput.source, fOutput.destination);
		}
		bufferMixer.AddBuffer(input->destination.id, buffer, true);
	} else {
		bufferMixer.AddBuffer(input->destination.id, buffer, false);
	}

	return status;
}

status_t VideoMixerNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleDataStatus");
	SendDataStatus(event->data, fOutput.destination, event->event_time);
	return B_OK;
}

status_t VideoMixerNode::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	fprintf(stderr,"VideoMixerNode(BMediaEventLooper)::HandleParameter");
	return B_OK;
}
