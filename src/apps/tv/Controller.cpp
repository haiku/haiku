/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <ParameterWeb.h>
#include <TimeSource.h>

#include "Controller.h"
#include "DeviceRoster.h"
#include "VideoView.h"
#include "VideoNode.h"

extern bool gOverlayDisabled;

status_t MediaRoster_Disconnect(const media_output &output, const media_input &input);



media_node		dvb_node;
media_node		audio_mixer_node;
media_node		video_window_node;
media_node		time_node;

media_input		audio_input;
media_output	audio_output;
media_input		video_input;
media_output	video_output;

BMediaRoster *gMediaRoster;

void 
HandleError(const char *text, status_t err)
{
	if (err != B_OK) { 
		printf("%s. error 0x%08x (%s)\n",text, (int)err, strerror(err));
		fflush(NULL);
		exit(1);
	}
}


Controller::Controller()
 :	fCurrentInterface(-1)
 ,	fCurrentChannel(-1)
 ,	fVideoView(NULL)
 ,	fVideoNode(NULL)
 ,	fWeb(NULL)
 ,	fChannelParam(NULL)
 ,	fConnected(false)
 , 	fInput()
 ,	fOutput()
{
	gMediaRoster = BMediaRoster::Roster();
}


Controller::~Controller()
{
	delete fWeb;
}


void
Controller::SetVideoView(VideoView *view)
{
	fVideoView = view;
}


void
Controller::SetVideoNode(VideoNode *node)
{
	fVideoNode = node;
}


void
Controller::DisconnectInterface()
{
	DisconnectNodes();
	fCurrentInterface = -1;
	fCurrentChannel = -1;
	delete fWeb;
	fWeb = 0;
	fChannelParam = 0;
}


status_t
Controller::ConnectInterface(int i)
{
	if (i < 0) {
		printf("Controller::ConnectInterface: wrong index\n");
		return B_ERROR;
	}
	if (fCurrentInterface != -1) {
		printf("Controller::ConnectInterface: already connected\n");
		return B_ERROR;
	}

	BParameterWeb *web;
	status_t err;

	err = gDeviceRoster->MediaRoster()->GetParameterWebFor(gDeviceRoster->DeviceNode(i), &web);
	if (err != B_OK) {
		printf("Controller::ConnectInterface: can't get parameter web\n");
		return B_ERROR;
	}

	delete fWeb;
	fWeb = web;
	fCurrentInterface = i;

	// XXX we may need to monitor for parameter web changes
	// and reassing fWeb and fChannelParam on demand.

	// find the channel control	and assign it to fChannelParam
	fChannelParam = NULL;
	int count = fWeb->CountParameters();
	for (int i = 0; i < count; i++) {
		BParameter *parameter = fWeb->ParameterAt(i);

		printf("parameter %d\n", i);
		printf("  name '%s'\n", parameter->Name());
		printf("  kind '%s'\n", parameter->Kind());
		printf("  unit '%s'\n", parameter->Unit());
		printf("  flags 0x%08" B_PRIx32 "\n", parameter->Flags());

		// XXX TODO: matching on Name is weak
		if (strcmp(parameter->Name(), "Channel") == 0 || strcmp(parameter->Kind(), B_TUNER_CHANNEL) == 0) {
			fChannelParam = dynamic_cast<BDiscreteParameter *>(parameter);
			if (fChannelParam)
				break;
		}
	}
	if (!fChannelParam) {
		printf("Controller::ConnectInterface: can't find channel parameter control\n");
		fCurrentChannel = -1;
	} else {
	if (fChannelParam->CountItems() == 0) {
			fCurrentChannel = -1;
			printf("Controller::ConnectInterface: channel control has 0 items\n");
		} else {
			int32 index;
			size_t size;
			status_t err;
			bigtime_t when;
			size = sizeof(index);
			err = fChannelParam->GetValue(&index, &size, &when);
			if (err == B_OK && size == sizeof(index)) {
				fCurrentChannel = index;
				printf("Controller::ConnectInterface: selected channel is %d\n", fCurrentChannel);
			} else {
				fCurrentChannel = -1;
				printf("Controller::ConnectInterface: can't get channel control value\n");
			}
		}
	}

	ConnectNodes();

	return B_OK;
}


bool
Controller::IsInterfaceAvailable(int i)
{
	return gDeviceRoster->IsRawAudioOutputFree(i) && gDeviceRoster->IsRawVideoOutputFree(i);
}


int
Controller::CurrentInterface()
{
	return fCurrentInterface;
}


int
Controller::CurrentChannel()
{
	return fCurrentChannel;
}


status_t
Controller::SelectChannel(int i)
{
	if (!fChannelParam)
		return B_ERROR;

	int32 index;
	status_t err;
	index = i;
	err = fChannelParam->SetValue(&index, sizeof(index), 0);
	if (err != B_OK) {
		printf("Controller::SelectChannel %d failed\n", i);
		return err;
	}

	fCurrentChannel = i;
	return B_OK;
}


int
Controller::ChannelCount()
{
	if (fCurrentInterface == -1)
		return 0;

	if (!fChannelParam)
		return 0;

	return fChannelParam->CountItems();
}


const char *
Controller::ChannelName(int i)
{
	if (fCurrentInterface == -1)
		return NULL;

	if (!fChannelParam)
		return NULL;

	return fChannelParam->ItemNameAt(i);
}


void
Controller::VolumeUp()
{
}


void
Controller::VolumeDown()
{
}


status_t
Controller::ConnectNodes()
{
	status_t err;

//	dvb_node = gDeviceRoster->DeviceNode(fCurrentInterface);

	err = gMediaRoster->GetNodeFor(gDeviceRoster->DeviceNode(fCurrentInterface).node, &dvb_node);
	HandleError("GetNodeFor failed", err);

	video_window_node = fVideoNode->Node();

	err = gMediaRoster->GetAudioMixer(&audio_mixer_node);
	HandleError("GetAudioMixer failed", err);

	media_input		input;
	media_output	output;
	media_format	fmt;
	int32			count;

	// Connect audio

	err = gMediaRoster->GetFreeOutputsFor(dvb_node, &output, 1, &count, B_MEDIA_RAW_AUDIO);
	HandleError("Can't find free audio output", err);
	if (count < 1)
		HandleError("No free audio output", -1);

	err = gMediaRoster->GetFreeInputsFor(audio_mixer_node, &input, 1, &count, B_MEDIA_RAW_AUDIO);
	HandleError("Can't find free audio input", err);
	if (count < 1)
		HandleError("No free audio input", -1);

	err = gMediaRoster->Connect(output.source, input.destination, &fmt, &audio_output, &audio_input);
	HandleError("Can't connect audio", err);

	// Connect video

	err = gMediaRoster->GetFreeOutputsFor(dvb_node, &output, 1, &count, B_MEDIA_RAW_VIDEO);
	HandleError("Can't find free video output", err);
	if (count < 1)
		HandleError("No free video output", -1);

	err = gMediaRoster->GetFreeInputsFor(video_window_node, &input, 1, &count, B_MEDIA_RAW_VIDEO);
	HandleError("Can't find free video input", err);
	if (count < 1)
		HandleError("No free video input", -1);

	color_space cspaces_overlay[] = { B_YCbCr422, B_RGB32, B_NO_COLOR_SPACE };
	color_space cspaces_bitmap[] = { B_RGB32, B_NO_COLOR_SPACE };

	if (gOverlayDisabled) {
		err = B_ERROR;
	} else {
		fVideoNode->SetOverlayEnabled(true);
		for (int i = 0; cspaces_overlay[i] != B_NO_COLOR_SPACE; i++) {
			printf("trying connect with colorspace 0x%08x\n", cspaces_overlay[i]);
			fmt.Clear();
			fmt.type = B_MEDIA_RAW_VIDEO;
			fmt.u.raw_video.display.format = cspaces_overlay[i];
			err = gMediaRoster->Connect(output.source, input.destination, &fmt, &video_output, &video_input);
			if (err == B_OK)
				break;
		}
	}
	if (err) {
		fVideoNode->SetOverlayEnabled(false);
		for (int i = 0; cspaces_bitmap[i] != B_NO_COLOR_SPACE; i++) {
			printf("trying connect with colorspace 0x%08x\n", cspaces_bitmap[i]);
			fmt.Clear();
			fmt.type = B_MEDIA_RAW_VIDEO;
			fmt.u.raw_video.display.format = cspaces_bitmap[i];
			err = gMediaRoster->Connect(output.source, input.destination, &fmt, &video_output, &video_input);
			if (err == B_OK)
				break;
		}
	}
	HandleError("Can't connect video", err);

	// set time sources

	err = gMediaRoster->GetTimeSource(&time_node);
	HandleError("Can't get time source", err);

	BTimeSource *ts = gMediaRoster->MakeTimeSourceFor(time_node);

	err = gMediaRoster->SetTimeSourceFor(dvb_node.node, time_node.node);
	HandleError("Can't set dvb time source", err);

	err = gMediaRoster->SetTimeSourceFor(audio_mixer_node.node, time_node.node);
	HandleError("Can't set audio mixer time source", err);

	err = gMediaRoster->SetTimeSourceFor(video_window_node.node, time_node.node);
	HandleError("Can't set video window time source", err);

	// Add a delay of (2 video frames) to the buffers send by the DVB node,
	// because as a capture device in B_RECORDING run mode it's supposed to
	// deliver buffers that were captured in the past (and does so).
	// It is important that the audio buffer size used by the connection with
	// the DVB node is smaller than this, optimum is the same length as one 
	// video frame (40 ms). However, this is not yet guaranteed.
	err = gMediaRoster->SetProducerRunModeDelay(dvb_node, 80000);
	HandleError("Can't set DVB producer delay", err);

	bigtime_t start_time = ts->Now() + 50000;

	ts->Release();

	// start nodes

	err = gMediaRoster->StartNode(dvb_node, start_time);
	HandleError("Can't start dvb node", err);

	err = gMediaRoster->StartNode(audio_mixer_node, start_time);
	HandleError("Can't start audio mixer node", err);

	err = gMediaRoster->StartNode(video_window_node, start_time);
	HandleError("Can't start video window node", err);

	printf("running...\n");

	fConnected = true;

	return B_OK;
}


status_t
Controller::DisconnectNodes()
{
	printf("stopping...\n");

	if (!fConnected)
		return B_OK;

	status_t err;

	// stop nodes

	err = gMediaRoster->StopNode(dvb_node, 0, true);
	HandleError("Can't stop dvb node", err);

	err = gMediaRoster->StopNode(audio_mixer_node, 0, true);
	HandleError("Can't stop audio mixer node", err);

	err = gMediaRoster->StopNode(video_window_node, 0, true);
	HandleError("Can't stop video window node", err);

	// disconnect nodes

	err = MediaRoster_Disconnect(video_output, video_input);
	HandleError("Can't disconnect video", err);

	err = MediaRoster_Disconnect(audio_output, audio_input);
	HandleError("Can't disconnect audio", err);

	// disable overlay, or erase image

	fVideoView->RemoveVideoDisplay();

	// release other nodes

	err = gMediaRoster->ReleaseNode(audio_mixer_node);
	HandleError("Can't release audio mixer node", err);

//	err = gMediaRoster->ReleaseNode(video_window_node);
//	HandleError("Can't release video window node", err);

//	err = gMediaRoster->ReleaseNode(time_node);
//	HandleError("Can't release time source node", err);

	// release dvb

	err = gMediaRoster->ReleaseNode(dvb_node);
	HandleError("Can't release DVB node", err);

	fConnected = false;

	return B_OK;
}


status_t 
MediaRoster_Disconnect(const media_output &output, const media_input &input)
{
	if (output.node.node <= 0) {
		printf("MediaRoster_Disconnect: output.node.node %d invalid\n",
			(int)output.node.node);
		return B_MEDIA_BAD_NODE;
	}
	if (input.node.node <= 0) {
		printf("MediaRoster_Disconnect: input.node.node %d invalid\n",
			(int)input.node.node);
		return B_MEDIA_BAD_NODE;
	}
	if (!(output.node.kind & B_BUFFER_PRODUCER)) {
		printf("MediaRoster_Disconnect: output.node.kind 0x%x is no B_BUFFER_PRODUCER\n",
			(int)output.node.kind);
		return B_MEDIA_BAD_NODE;
	}
	if (!(input.node.kind & B_BUFFER_CONSUMER)) {
		printf("MediaRoster_Disconnect: input.node.kind 0x%x is no B_BUFFER_PRODUCER\n",
			(int)input.node.kind);
		return B_MEDIA_BAD_NODE;
	}
	if (input.source.port != output.source.port) {
		printf("MediaRoster_Disconnect: input.source.port %d doesn't match output.source.port %d\n",
			(int)input.source.port, (int)output.source.port);
		return B_MEDIA_BAD_NODE;
	}
	if (input.source.id != output.source.id) {
		printf("MediaRoster_Disconnect: input.source.id %d doesn't match output.source.id %d\n",
			(int)input.source.id, (int)output.source.id);
		return B_MEDIA_BAD_NODE;
	}
	if (input.destination.port != output.destination.port) {
		printf("MediaRoster_Disconnect: input.destination.port %d doesn't match output.destination.port %d\n",
			(int)input.destination.port, (int)output.destination.port);
		return B_MEDIA_BAD_NODE;
	}
	if (input.destination.id != output.destination.id) {
		printf("MediaRoster_Disconnect: input.destination.id %d doesn't match output.destination.id %d\n",
			(int)input.destination.id, (int)output.destination.id);
		return B_MEDIA_BAD_NODE;
	}
	return BMediaRoster::Roster()->Disconnect(output.node.node, output.source, input.node.node, input.destination);
}
