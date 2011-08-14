/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// AudioAdapterNode.cpp

#include "AudioAdapterNode.h"
#include "AudioAdapterParams.h"

#include "SoundUtils.h"

#include <cstdio>
#include <cstring>

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

_AudioAdapterNode::~_AudioAdapterNode() {}

_AudioAdapterNode::_AudioAdapterNode(
	const char*									name,
	IAudioOpFactory*						opFactory,
	BMediaAddOn*								addOn) :
	
	BMediaNode(name),
	AudioFilterNode(name, opFactory, addOn) {
	
//	PRINT((
//		"\n"
//		"--*-- _AudioAdapterNode() [%s] --*--\n\n",
//		__BUILD_DATE__));
}

// -------------------------------------------------------- //
// AudioFilterNode
// -------------------------------------------------------- //

status_t _AudioAdapterNode::getRequiredInputFormat(
	media_format&								ioFormat) {
		
	status_t err = getPreferredInputFormat(ioFormat);
	if(err < B_OK)
		return err;

	// 16sep99: input byte-swapping now supported		
	ioFormat.u.raw_audio.byte_order = media_raw_audio_format::wildcard.byte_order;
		
//	ioFormat.u.raw_audio.format = media_raw_audio_format::wildcard.format;
//	ioFormat.u.raw_audio.channel_count = media_raw_audio_format::wildcard.channel_count;
	_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
	ASSERT(p);
	
//	media_raw_audio_format& w = media_raw_audio_format::wildcard;
	
	// copy user preferences
	ioFormat.u.raw_audio.format = p->inputFormat.format;
	ioFormat.u.raw_audio.channel_count = p->inputFormat.channel_count;


	// don't require a buffer size until format & channel_count are known [16sep99]
	ioFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;

	if(output().destination == media_destination::null) {
		// frame rate isn't constrained yet
		ioFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
	}
				
	return B_OK;
}

// +++++ 17sep99: use parameter data!

status_t _AudioAdapterNode::getPreferredInputFormat(
	media_format&								ioFormat) {

	status_t err = _inherited::getPreferredInputFormat(ioFormat);
	if(err < B_OK)
		return err;
		
	_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
	ASSERT(p);
	
	media_raw_audio_format& f = ioFormat.u.raw_audio;
	media_raw_audio_format& w = media_raw_audio_format::wildcard;
	
	// copy user preferences
	if(p->inputFormat.format != w.format)
		f.format = p->inputFormat.format;
	if(p->inputFormat.channel_count != w.channel_count)
		f.channel_count = p->inputFormat.channel_count;
	
//	// if one end is connected, prefer not to do channel conversions [15sep99]
//	if(output().destination != media_destination::null)
//		ioFormat.u.raw_audio.channel_count = output().format.u.raw_audio.channel_count;	

	// if output connected, constrain:
	//   buffer_size
	//   frame_rate
	if(output().destination != media_destination::null) {
		// if the user doesn't care, default to the output's frame format
		if(f.format == w.format)
			f.format = output().format.u.raw_audio.format;
		if(f.channel_count == w.channel_count)
			f.channel_count = output().format.u.raw_audio.channel_count;

		f.buffer_size =
			bytes_per_frame(f) *
				frames_per_buffer(output().format.u.raw_audio);
		f.frame_rate = output().format.u.raw_audio.frame_rate;		
	}

	return B_OK;
}
	
status_t _AudioAdapterNode::getRequiredOutputFormat(
	media_format&								ioFormat) {
		
	status_t err = getPreferredOutputFormat(ioFormat);
	if(err < B_OK)
		return err;
		
	ioFormat.u.raw_audio.format = media_raw_audio_format::wildcard.format;
	ioFormat.u.raw_audio.channel_count = media_raw_audio_format::wildcard.channel_count;

	_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
	ASSERT(p);
	
//	media_raw_audio_format& w = media_raw_audio_format::wildcard;
	
	// copy user preferences
	ioFormat.u.raw_audio.format = p->outputFormat.format;
	ioFormat.u.raw_audio.channel_count = p->outputFormat.channel_count;

	// don't require a buffer size until format & channel_count are known [16sep99]
	ioFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;
		
	if(input().source == media_source::null) {
		// frame rate isn't constrained yet
		ioFormat.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
	}
		
	return B_OK;
}
	
// +++++ 17sep99: use parameter data!

status_t _AudioAdapterNode::getPreferredOutputFormat(
	media_format&								ioFormat) {

	status_t err = _inherited::getPreferredOutputFormat(ioFormat);
	if(err < B_OK)
		return err;

	_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
	ASSERT(p);
	
	media_raw_audio_format& w = media_raw_audio_format::wildcard;
	
	// copy user preferences
	if(p->outputFormat.format != w.format)
		ioFormat.u.raw_audio.format = p->outputFormat.format;
	if(p->outputFormat.channel_count != w.channel_count)
		ioFormat.u.raw_audio.channel_count = p->outputFormat.channel_count;

////	// if one end is connected, prefer not to do channel conversions [15sep99]
////	if(input().source != media_source::null)
////		ioFormat.u.raw_audio.channel_count = input().format.u.raw_audio.channel_count;
		
	// if input connected, constrain:
	//   buffer_size
	//   frame_rate
	if(input().source != media_source::null) {		
		// if the user doesn't care, default to the input's frame format
		if(ioFormat.u.raw_audio.format == w.format)
			ioFormat.u.raw_audio.format = input().format.u.raw_audio.format;
		if(ioFormat.u.raw_audio.channel_count == w.channel_count)
			ioFormat.u.raw_audio.channel_count = input().format.u.raw_audio.channel_count;

		ioFormat.u.raw_audio.buffer_size =
			bytes_per_frame(ioFormat.u.raw_audio) *
				frames_per_buffer(input().format.u.raw_audio);
		PRINT(("##### preferred output buffer_size: %ld (%x)\n", ioFormat.u.raw_audio.buffer_size, ioFormat.u.raw_audio.buffer_size));
		ioFormat.u.raw_audio.frame_rate = input().format.u.raw_audio.frame_rate;

	}


	return B_OK;
}
	
status_t _AudioAdapterNode::validateProposedInputFormat(
	const media_format&					preferredFormat,
	media_format&								ioProposedFormat) {
		
	status_t err = _inherited::validateProposedInputFormat(
		preferredFormat, ioProposedFormat);
		
	media_raw_audio_format& w = media_raw_audio_format::wildcard;
		
	if(output().destination != media_destination::null) {

		// an output connection exists; constrain the input format
		
		// is there enough information to suggest a buffer size?
		if(
			ioProposedFormat.u.raw_audio.format != w.format &&
			ioProposedFormat.u.raw_audio.channel_count != w.channel_count) {
					
			size_t target_buffer_size = 
				bytes_per_frame(ioProposedFormat.u.raw_audio) *
					frames_per_buffer(output().format.u.raw_audio);
	
			if(ioProposedFormat.u.raw_audio.buffer_size != target_buffer_size) {
				if(ioProposedFormat.u.raw_audio.buffer_size != w.buffer_size)
					err = B_MEDIA_BAD_FORMAT;

				ioProposedFormat.u.raw_audio.buffer_size = target_buffer_size;
			}
		}
			
		// require output frame rate
		if(ioProposedFormat.u.raw_audio.frame_rate != output().format.u.raw_audio.frame_rate) {
			if(ioProposedFormat.u.raw_audio.frame_rate != w.frame_rate)
				err = B_MEDIA_BAD_FORMAT;

			ioProposedFormat.u.raw_audio.frame_rate = output().format.u.raw_audio.frame_rate;
		}
	}
		
	char fmt_string[256];
	string_for_format(ioProposedFormat, fmt_string, 255);
		PRINT((
		"### _AudioAdapterNode::validateProposedInputFormat():\n"
		"    %s\n", fmt_string));
	return err;
}

status_t _AudioAdapterNode::validateProposedOutputFormat(
	const media_format&					preferredFormat,
	media_format&								ioProposedFormat) {
		
	status_t err = _inherited::validateProposedOutputFormat(
		preferredFormat, ioProposedFormat);
		
	media_raw_audio_format& w = media_raw_audio_format::wildcard;
		
	if(input().source != media_source::null) {

		// an input connection exists; constrain the output format

		// is there enough information to suggest a buffer size?
		if(
			ioProposedFormat.u.raw_audio.format != w.format &&
			ioProposedFormat.u.raw_audio.channel_count != w.channel_count) {
					
			size_t target_buffer_size = 
				bytes_per_frame(ioProposedFormat.u.raw_audio) *
					frames_per_buffer(input().format.u.raw_audio);

			if(ioProposedFormat.u.raw_audio.buffer_size != target_buffer_size) {
				if(ioProposedFormat.u.raw_audio.buffer_size != w.buffer_size)
					err = B_MEDIA_BAD_FORMAT;

				ioProposedFormat.u.raw_audio.buffer_size = target_buffer_size;
			}
		}
		
		// require same frame rate as input
		if(ioProposedFormat.u.raw_audio.frame_rate != input().format.u.raw_audio.frame_rate) {
			if(ioProposedFormat.u.raw_audio.frame_rate != w.frame_rate)
				err = B_MEDIA_BAD_FORMAT;

			ioProposedFormat.u.raw_audio.frame_rate = input().format.u.raw_audio.frame_rate;
		}
	}
	
	char fmt_string[256];
	string_for_format(ioProposedFormat, fmt_string, 255);
	PRINT((
		"### _AudioAdapterNode::validateProposedOutputFormat():\n"
		"    %s\n", fmt_string));
	return err;
}

void 
_AudioAdapterNode::SetParameterValue(int32 id, bigtime_t changeTime, const void *value, size_t size)
{
	switch(id) {
	case _AudioAdapterParams::P_INPUT_FORMAT:
		if(input().source != media_source::null) {
			media_multi_audio_format f = input().format.u.raw_audio;
			if(size != 4)
				return;
			f.format = *(uint32*)value;
			_attemptInputFormatChange(f);
			return;
		}
		break;
	case _AudioAdapterParams::P_INPUT_CHANNEL_COUNT:
		if(input().source != media_source::null) {
			media_multi_audio_format f = input().format.u.raw_audio;
			if(size != 4)
				return;
			f.channel_count = *(uint32*)value;
			_attemptInputFormatChange(f);
			return;
		}
		break;
	case _AudioAdapterParams::P_OUTPUT_FORMAT:
		if(output().source != media_source::null) {
			media_multi_audio_format f = output().format.u.raw_audio;
			if(size != 4)
				return;
			f.format = *(uint32*)value;
			_attemptOutputFormatChange(f);
			return;
		}
		break;
	case _AudioAdapterParams::P_OUTPUT_CHANNEL_COUNT:
		if(output().source != media_source::null) {
			media_multi_audio_format f = output().format.u.raw_audio;
			if(size != 4)
				return;
			f.channel_count = *(uint32*)value;
			_attemptOutputFormatChange(f);
			return;
		}
		break;
	}
		
	return _inherited::SetParameterValue(id, changeTime, value, size);
}


// -------------------------------------------------------- //
// BBufferProducer/Consumer
// -------------------------------------------------------- //

status_t _AudioAdapterNode::Connected(
	const media_source&					source,
	const media_destination&		destination,
	const media_format&					format,
	media_input*								outInput) {

	status_t err = _inherited::Connected(
		source, destination, format, outInput);
	
	if(err == B_OK) {
		_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
		ASSERT(p);	
		p->inputFormat = format.u.raw_audio;

		_broadcastInputFormatParams();
	}
	
	return err;
}	

void _AudioAdapterNode::Connect(
	status_t										status,
	const media_source&					source,
	const media_destination&		destination,
	const media_format&					format,
	char*												ioName) {

	if(status == B_OK) {
		_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
		ASSERT(p);
		p->outputFormat = format.u.raw_audio;

		_broadcastOutputFormatParams();
	}	
	
	_inherited::Connect(
		status, source, destination, format, ioName);
}
		

void _AudioAdapterNode::_attemptInputFormatChange(
	const media_multi_audio_format& format) {
	// +++++
	
	char fmtString[256];
	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	f.u.raw_audio = format;
	string_for_format(f, fmtString, 256);
	PRINT((
		"_AudioAdapterNode::attemptInputFormatChange():\n  '%s'\n",
		fmtString));
		


	// +++++ reject attempt: broadcast params for current format
	_broadcastInputFormatParams();
}

void _AudioAdapterNode::_attemptOutputFormatChange(
	const media_multi_audio_format& format) {

	// +++++
	char fmtString[256];
	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	f.u.raw_audio = format;
	string_for_format(f, fmtString, 256);
	PRINT((
		"_AudioAdapterNode::attemptOutputFormatChange():\n  '%s'\n",
		fmtString));
	
	media_destination dest = output().destination;
	if(dest == media_destination::null) {
	PRINT((
		"! output not connected!\n"));
		return;
	}

	_AudioAdapterParams* p = dynamic_cast<_AudioAdapterParams*>(parameterSet());
	ASSERT(p);
	status_t err;
	
	// disallow wildcards
	if(format.format == media_raw_audio_format::wildcard.format ||
		format.channel_count == media_raw_audio_format::wildcard.channel_count) {
		PRINT((
			"! wildcards not allowed\n"));
		goto broadcast;
	}

	err = prepareFormatChange(f);
	if(err < B_OK)
	{
		PRINT((
			"! format not supported\n"));
		goto broadcast;
	}

	err = ProposeFormatChange(&f, dest);
	if(err < B_OK)
	{
		PRINT((
			"! format rejected\n"));
		goto broadcast;
	}

	err = ChangeFormat(
		output().source,
		dest,
		&f);
		
	if(err < B_OK) {
		PRINT(("! ChangeFormat(): %s\n", strerror(err)));
		goto broadcast;
	}

	// store new format
	p->outputFormat = format;
	
	// inform AudioFilterNode of format change
	doFormatChange(f);

broadcast:
	_broadcastOutputFormatParams();
} 


void 
_AudioAdapterNode::_broadcastInputFormatParams()
{
	PRINT(("_AudioAdapterNode::_broadcastInputFormatParams()\n"));
	BroadcastNewParameterValue(
		0LL,
		_AudioAdapterParams::P_INPUT_FORMAT,
		(void*)&input().format.u.raw_audio.format,
		4);
	BroadcastNewParameterValue(
		0LL,
		_AudioAdapterParams::P_INPUT_CHANNEL_COUNT,
		(void*)&input().format.u.raw_audio.channel_count,
		4);
//	BroadcastChangedParameter(_AudioAdapterParams::P_INPUT_FORMAT);
//	BroadcastChangedParameter(_AudioAdapterParams::P_INPUT_CHANNEL_COUNT);
}

void 
_AudioAdapterNode::_broadcastOutputFormatParams()
{
	PRINT(("_AudioAdapterNode::_broadcastOutputFormatParams()\n"));
	
	BroadcastNewParameterValue(
		0LL,
		_AudioAdapterParams::P_OUTPUT_FORMAT,
		(void*)&output().format.u.raw_audio.format,
		4);
	BroadcastNewParameterValue(
		0LL,
		_AudioAdapterParams::P_OUTPUT_CHANNEL_COUNT,
		(void*)&output().format.u.raw_audio.channel_count,
		4);
//	BroadcastChangedParameter(_AudioAdapterParams::P_OUTPUT_FORMAT);
//	BroadcastChangedParameter(_AudioAdapterParams::P_OUTPUT_CHANNEL_COUNT);
}



// END -- AudioAdapterNode.cpp --
