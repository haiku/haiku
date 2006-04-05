/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
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
		
	memset(&fmt, 0, sizeof(fmt));
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

	fVideoNode->SetOverlayEnabled(true);
	for (int i = 0; cspaces_overlay[i] != B_NO_COLOR_SPACE; i++) {
		printf("trying connect with colorspace 0x%08x\n", cspaces_overlay[i]);
		memset(&fmt, 0, sizeof(fmt));
		fmt.type = B_MEDIA_RAW_VIDEO;
		fmt.u.raw_video.display.format = cspaces_overlay[i];
		err = gMediaRoster->Connect(output.source, input.destination, &fmt, &video_output, &video_input);
		if (err == B_OK)
			break;
	}
	if (err) {
		fVideoNode->SetOverlayEnabled(false);
		for (int i = 0; cspaces_bitmap[i] != B_NO_COLOR_SPACE; i++) {
			printf("trying connect with colorspace 0x%08x\n", cspaces_bitmap[i]);
			memset(&fmt, 0, sizeof(fmt));
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

