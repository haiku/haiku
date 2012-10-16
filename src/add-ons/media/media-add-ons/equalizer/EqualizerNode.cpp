/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 

#include <ByteOrder.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <TimeSource.h>
#include <ParameterWeb.h>
#include <String.h>

#include <stdio.h>
#include <string.h>

#include "EqualizerNode.h"

//EqualizerNode
EqualizerNode::~EqualizerNode()
{
	Quit();
}


EqualizerNode::EqualizerNode(BMediaAddOn* addon)
	:
	BMediaNode("10 Band Equalizer"),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),
	fAddOn(addon),
	fProcessLatency(0),
	fDownstreamLatency(0),
	fOutputMediaEnabled(true)
{
}


//BMediaNode
BMediaAddOn*
EqualizerNode::AddOn(int32* id) const 
{
	if (fAddOn)
		*id = 0;
	return fAddOn;
}


status_t 
EqualizerNode::HandleMessage(int32 message, const void *data, size_t size)
{
	if ((BControllable::HandleMessage(message, data, size) != B_OK) &&
		(BBufferConsumer::HandleMessage(message, data, size) != B_OK) &&
		(BBufferProducer::HandleMessage(message, data, size) != B_OK) &&
		(BControllable::HandleMessage(message, data, size) != B_OK)) {
		BMediaNode::HandleMessage(message, data, size);
		return B_OK;
	}
	BMediaNode::HandleBadMessage(message, data, size);
	return B_ERROR;	
}


void
EqualizerNode::NodeRegistered()
{
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio.buffer_size = BUFF_SIZE;
	fPreferredFormat.u.raw_audio = media_raw_audio_format::wildcard;
	fPreferredFormat.u.raw_audio.channel_count =
		media_raw_audio_format::wildcard.channel_count;
	fPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	
	fFormat.type = B_MEDIA_RAW_AUDIO;
	fFormat.u.raw_audio = media_raw_audio_format::wildcard;
	
	fInputMedia.destination.port = ControlPort();
	fInputMedia.destination.id = ID_AUDIO_INPUT;
	fInputMedia.node = Node();
	fInputMedia.source = media_source::null;
	fInputMedia.format = fFormat;
	strncpy(fInputMedia.name, "Audio Input", B_MEDIA_NAME_LENGTH);

	fOutputMedia.source.port = ControlPort();
	fOutputMedia.source.id = ID_AUDIO_OUTPUT;
	fOutputMedia.node = Node();
	fOutputMedia.destination = media_destination::null;
	fOutputMedia.format = fFormat;
	strncpy(fOutputMedia.name, "Audio Output", B_MEDIA_NAME_LENGTH);

	InitParameterValues();
	InitParameterWeb();
}


//BControllable
status_t 
EqualizerNode::GetParameterValue(int32 id, bigtime_t* lastChangeTime,
	void* value, size_t* size)
{
	if (*size < sizeof(float))
		return B_NO_MEMORY;

	if (id == P_MUTE) {
		*(int32*)value = fMute;
		*lastChangeTime = fMuteLastChanged;
		*size = sizeof(int32);
	} else if (id == P_BYPASS) {
		*(int32*)value = fByPass;
		*lastChangeTime = fByPassLastChanged;
		*size = sizeof(int32);
	} else if (id == P_PREAMP) {
		*(float*)value = (float)fEqualizer.PreAmp();
		*lastChangeTime = fPreAmpLastChanged;
		*size = sizeof(float);
	} else if (id >= P_BANDS && id < P_BANDS + fEqualizer.BandCount()) {
		int band = id - P_BANDS;
		*(float*)value = (float)fEqualizer.Band(band);
		*lastChangeTime = fBandsLastChanged[band];
		*size = sizeof(float);
	} else
		return B_ERROR;
	return B_OK;
}


void
EqualizerNode::SetParameterValue(int32 id, bigtime_t time, const void* value,
	size_t size)
{
	if (id == P_PREAMP || id == P_BYPASS || id == P_MUTE
		|| (id >= P_BANDS && id < P_BANDS + fEqualizer.BandCount())) {
		media_timed_event ev(time, BTimedEventQueue::B_PARAMETER, (void*)value,
			BTimedEventQueue::B_NO_CLEANUP, size, id, (char*)"EQ");
		//dirty hack for parameter processing (mediakit bug????)
		ParameterEventProcessing(&ev);
		EventQueue()->AddEvent(ev);		
	}
}


//BBufferConsumer
void
EqualizerNode::BufferReceived(BBuffer* buffer)
{
	if (buffer->Header()->destination != fInputMedia.destination.id) {
		buffer->Recycle();
		return;
	}

	if (fOutputMedia.destination == media_destination::null
		|| !fOutputMediaEnabled) {
		buffer->Recycle();
		return;
	}

	FilterBuffer(buffer);

	status_t err = SendBuffer(buffer, fOutputMedia.source,
		fOutputMedia.destination);
	if (err < B_OK)
		buffer->Recycle();
}


status_t 
EqualizerNode::AcceptFormat(const media_destination &dst, media_format* format)
{
	if (dst != fInputMedia.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	ValidateFormat((fFormat.u.raw_audio.format
			!= media_raw_audio_format::wildcard.format) ?
		fFormat : fPreferredFormat, *format);

	return B_OK;
}


status_t 
EqualizerNode::GetNextInput(int32* cookie, media_input* input)
{
	if (*cookie)
		return B_BAD_INDEX;

	++*cookie;
	*input = fInputMedia;
	return B_OK;
}


void 
EqualizerNode::DisposeInputCookie(int32 cookie)
{
}


status_t 
EqualizerNode::FormatChanged(const media_source &src,
	const media_destination &dst, int32 changeTag, const media_format &format)
{
	return B_MEDIA_BAD_FORMAT;
}


void 
EqualizerNode::ProducerDataStatus(const media_destination &dst, int32 status,
	bigtime_t when)
{
	if (fOutputMedia.destination != media_destination::null)
		SendDataStatus(status, fOutputMedia.destination, when);
}


status_t 
EqualizerNode::GetLatencyFor(const media_destination &dst, bigtime_t* latency, 
	media_node_id* outTimeSource)
{

	if (dst != fInputMedia.destination)
		return B_MEDIA_BAD_DESTINATION;

	*latency = fDownstreamLatency + fProcessLatency;
	*outTimeSource = TimeSource()->ID();
	return B_OK;
}


status_t
EqualizerNode::Connected(const media_source& source,
	const media_destination& destination, const media_format& format,
	media_input* poInput)
{
	if (destination != fInputMedia.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (fInputMedia.source != media_source::null)
		return B_MEDIA_ALREADY_CONNECTED;

	fInputMedia.source = source;
	fInputMedia.format = format;
	*poInput = fInputMedia;
	fFormat = format;

	return B_OK;
}


void 
EqualizerNode::Disconnected(const media_source &src,
	const media_destination &dst)
{
	if (fInputMedia.source != src || dst != fInputMedia.destination)
		return;

	fInputMedia.source = media_source::null;

	if (fOutputMedia.destination == media_destination::null)
		fFormat.u.raw_audio = media_raw_audio_format::wildcard;

	fInputMedia.format = fFormat;
}


//BBufferProducer
status_t 
EqualizerNode::FormatSuggestionRequested(media_type type, int32 quality,
	media_format* format) 
{
	if (type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	if (fFormat.u.raw_audio.format != media_raw_audio_format::wildcard.format)
		*format = fFormat;	
	else
		*format = fPreferredFormat;
	return B_OK;
}


status_t 
EqualizerNode::FormatProposal(const media_source &src, media_format* format)
{
	if (src != fOutputMedia.source)
		return B_MEDIA_BAD_SOURCE;

	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	ValidateFormat((fFormat.u.raw_audio.format
			!= media_raw_audio_format::wildcard.format) ?
		fFormat:fPreferredFormat, *format);

	return B_OK;
}


status_t 
EqualizerNode::FormatChangeRequested(const media_source &src,
	const media_destination &dst, media_format* format, int32* _deprecated_)
{
	return B_MEDIA_BAD_FORMAT;
}


void 
EqualizerNode::LateNoticeReceived(const media_source &src, bigtime_t late,
	bigtime_t when)
{
	if (src != fOutputMedia.source || fInputMedia.source == media_source::null)
		return;

	NotifyLateProducer(fInputMedia.source, late, when);
}


status_t
EqualizerNode::GetNextOutput(int32* cookie, media_output* output)
{
	if (*cookie)
		return B_BAD_INDEX;

	++*cookie;
	*output = fOutputMedia;
	return B_OK;
}


status_t 
EqualizerNode::DisposeOutputCookie(int32 cookie) 
{
	return B_OK;
}


status_t
EqualizerNode::SetBufferGroup(const media_source &src, BBufferGroup* group)
{
	int32 changeTag;
	status_t ret = B_OK;

	if (src != fOutputMedia.source)
		return B_MEDIA_BAD_SOURCE;

	if (fInputMedia.source == media_source::null)
		return B_ERROR;

	ret = SetOutputBuffersFor(fInputMedia.source, fInputMedia.destination,
		group, 0, &changeTag);
	return ret;
}


status_t
EqualizerNode::PrepareToConnect(const media_source &src,
	const media_destination &dst, media_format* format, media_source* outSource,
	char* outName)
{
	if (src != fOutputMedia.source)
		return B_MEDIA_BAD_SOURCE;

	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
	
	if (fOutputMedia.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	status_t err = ValidateFormat((fFormat.u.raw_audio.format
			!= media_raw_audio_format::wildcard.format) ? fFormat
		: fPreferredFormat, *format);
	
	if (err < B_OK)
		return err;

	SetOutputFormat(*format);

	fOutputMedia.destination = dst;
	fOutputMedia.format = *format;

	*outSource = fOutputMedia.source;
	strncpy(outName, fOutputMedia.name, B_MEDIA_NAME_LENGTH);

	return B_OK;
}


void
EqualizerNode::Connect(status_t status, const media_source &src,
	const media_destination &dst, const media_format &format, char* name)
{
	if (status < B_OK) {
		fOutputMedia.destination = media_destination::null;
		return;
	}

	strncpy(name, fOutputMedia.name, B_MEDIA_NAME_LENGTH);
	fOutputMedia.destination = dst;
	fFormat = format;

	media_node_id timeSource;
	FindLatencyFor(fOutputMedia.destination, &fDownstreamLatency, &timeSource);
	
	InitFilter();

	fProcessLatency = GetFilterLatency();
	SetEventLatency(fDownstreamLatency + fProcessLatency);

	if (fInputMedia.source != media_source::null) {
		SendLatencyChange(fInputMedia.source, fInputMedia.destination, 
			EventLatency() + SchedulingLatency());
	}

	bigtime_t duration = 0;
	
	int sample_size = (fFormat.u.raw_audio.format & 0xf)
		* fFormat.u.raw_audio.channel_count;
	
	if (fFormat.u.raw_audio.buffer_size > 0
		&& fFormat.u.raw_audio.frame_rate > 0 && sample_size > 0) {
		duration = (bigtime_t)(((fFormat.u.raw_audio.buffer_size / sample_size)
			/ fFormat.u.raw_audio.frame_rate) * 1000000.0);
	}

	SetBufferDuration(duration);
}


void 
EqualizerNode::Disconnect(const media_source &src, const media_destination &dst) 
{
	if (src != fOutputMedia.source)
		return;

	if (dst != fOutputMedia.destination)
		return;

	fOutputMedia.destination = media_destination::null;

	if (fInputMedia.source == media_source::null)
		fFormat.u.raw_audio = media_raw_audio_format::wildcard;

	fOutputMedia.format = fFormat;
}


void 
EqualizerNode::EnableOutput(const media_source &src, bool enabled,
	int32* _deprecated_)
{
	if (src != fOutputMedia.source)
		return;
	fOutputMediaEnabled = enabled;
}


status_t 
EqualizerNode::GetLatency(bigtime_t* latency)
{
	*latency = EventLatency() + SchedulingLatency();
	return B_OK;
}


void 
EqualizerNode::LatencyChanged(const media_source &src,
	const media_destination &dst, bigtime_t latency, uint32 flags)
{
	if (src != fOutputMedia.source || dst != fOutputMedia.destination)
		return;

	fDownstreamLatency = latency;
	SetEventLatency(fDownstreamLatency + fProcessLatency);

	if (fInputMedia.source != media_source::null) {
		SendLatencyChange(fInputMedia.source, 
			fInputMedia.destination,EventLatency() + SchedulingLatency());
	}
}


//BMediaEventLooper
bigtime_t 
EqualizerNode::OfflineTime()
{
	return 0LL;
}


//EqualizerNode
void
EqualizerNode::HandleEvent(const media_timed_event* event, bigtime_t late,
	bool realTime)
{	
	if (event->type == BTimedEventQueue::B_PARAMETER)
		ParameterEventProcessing(event);
}


void 
EqualizerNode::ParameterEventProcessing(const media_timed_event* event)
{
	float value = 0.0;
	int32 value32 = 0;
	
	int32 id = event->bigdata;
	size_t size = event->data;
	bigtime_t now = TimeSource()->Now();

	type_code v_type = B_FLOAT_TYPE;
	
	BParameter* web_param;	

	for (int i = 0; i < fWeb->CountParameters(); i++) {
		web_param = fWeb->ParameterAt(i);
		if (web_param->ID() == id) {
			v_type=web_param->ValueType();
			break;
		}
	}
	
	if (v_type == B_FLOAT_TYPE)
		value = *((float*)event->pointer);
	else if (v_type == B_INT32_TYPE) {		
		value32 = *((int32*)event->pointer);
		value = (float)value32;
	}
	
	if (id == P_MUTE) {
		fMute = value32;
		fMuteLastChanged = now;
		BroadcastNewParameterValue(now,	id,	event->pointer, size);
	} else if (id == P_BYPASS) {
		fByPass = value32;
		fByPassLastChanged = now;
		BroadcastNewParameterValue(now,	id,	event->pointer, size);
	} else if (id == P_PREAMP) {
		if (value != fEqualizer.PreAmp()) {
			fEqualizer.SetPreAmp(value);
			fPreAmpLastChanged = now;
			BroadcastNewParameterValue(now,	id,	&value,	size);
		}
	} else if (id >= P_BANDS && id < P_BANDS + fEqualizer.BandCount()) {
		int band = id - P_BANDS;
		if (value != fEqualizer.Band(band)) {
			fEqualizer.SetBand(band, value);
			fBandsLastChanged[band] = now;
			BroadcastNewParameterValue(now,	id,	&value,	size);			
		}
	}
}


status_t
EqualizerNode::ValidateFormat(const media_format &preferred_format,
							media_format &proposed_format)
{
	status_t ret = B_OK;
		
	if (proposed_format.type != B_MEDIA_RAW_AUDIO) {
		proposed_format = preferred_format;
		return B_MEDIA_BAD_FORMAT;
	}

	media_raw_audio_format &wild = media_raw_audio_format::wildcard;
	media_raw_audio_format &f = proposed_format.u.raw_audio;
	const media_raw_audio_format &pref = preferred_format.u.raw_audio;

	if(pref.frame_rate != wild.frame_rate && f.frame_rate != pref.frame_rate) {
		if(f.frame_rate != wild.frame_rate)
			ret = B_MEDIA_BAD_FORMAT;
		f.frame_rate = pref.frame_rate;
	}

	if(pref.channel_count != wild.channel_count &&
		f.channel_count != pref.channel_count) {
		if(f.channel_count != wild.channel_count)
			ret = B_MEDIA_BAD_FORMAT;
		f.channel_count = pref.channel_count;
	}

	if(pref.format != wild.format && f.format != pref.format) {
		if(f.format != wild.format)
			ret = B_MEDIA_BAD_FORMAT;
		f.format = pref.format;
	}

	if(pref.byte_order != wild.byte_order &&
		f.byte_order != pref.byte_order) {
		if(f.byte_order != wild.byte_order)
			ret = B_MEDIA_BAD_FORMAT;
		f.byte_order = pref.byte_order;
	}

	if(pref.buffer_size != wild.buffer_size &&
		f.buffer_size != pref.buffer_size) {
		if(f.buffer_size != wild.buffer_size)
			ret = B_MEDIA_BAD_FORMAT;
		f.buffer_size = pref.buffer_size;
	}
	
	return ret;
}


void
EqualizerNode::SetOutputFormat(media_format &format)
{
	media_raw_audio_format &f = format.u.raw_audio;
	media_raw_audio_format &w = media_raw_audio_format::wildcard;

	if (f.frame_rate == w.frame_rate) {
		f.frame_rate = 44100.0;
	}
	if (f.channel_count == w.channel_count) {
		if(fInputMedia.source != media_source::null)
			f.channel_count = fInputMedia.format.u.raw_audio.channel_count;
		else
			f.channel_count = 2;
	}

	if (f.format == w.format)
		f.format = media_raw_audio_format::B_AUDIO_FLOAT;

	if (f.byte_order == w.format) {
		f.byte_order = (B_HOST_IS_BENDIAN) ?
			B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	}

	if (f.buffer_size == w.buffer_size)
		f.buffer_size = BUFF_SIZE;
}


void 
EqualizerNode::InitParameterValues()
{		
	fMute = 0;
	fByPass = 0;
	fMuteLastChanged = 0LL;
	fByPassLastChanged = 0LL;
	fPreAmpLastChanged = 0LL;
	
	for (int i = 0; i < EQ_BANDS; i++)
		fBandsLastChanged[i] = 0LL;
	
	fEqualizer.CleanUp();
}


void 
EqualizerNode::InitParameterWeb(void)
{
	fWeb = new BParameterWeb();
	
	BParameterGroup* fParamGroup = fWeb->MakeGroup("EqualizerNode Parameters");
	BParameterGroup* fFControlGroup = fParamGroup->MakeGroup("FilterControl");

	fFControlGroup->MakeDiscreteParameter(P_MUTE,B_MEDIA_NO_TYPE,"Mute",
		B_ENABLE);
	fFControlGroup->MakeDiscreteParameter(P_BYPASS,B_MEDIA_NO_TYPE,"ByPass",
		B_ENABLE);	

	BNullParameter* label;
	BParameterGroup* group;
	BContinuousParameter* value;

	group = fParamGroup->MakeGroup("Pre Amp");
	label = group->MakeNullParameter(P_PREAMP_LABEL, B_MEDIA_NO_TYPE, "Pre Amp",
		B_GENERIC);
	value = group->MakeContinuousParameter(P_PREAMP, B_MEDIA_NO_TYPE, "",
		B_GAIN, "dB", -8.0, 8.0, 0.1);
	label->AddOutput(value);
	value->AddInput(label);
	
	for (int i = 0; i < fEqualizer.BandCount(); i++) {
		char freq[32];
		sprintf(freq,"%gHz",fEqualizer.BandFrequency(i));
		group = fParamGroup->MakeGroup(freq);
		label = group->MakeNullParameter(P_BAND_LABELS + i, B_MEDIA_NO_TYPE,
			freq, B_GENERIC);
		value = group->MakeContinuousParameter(P_BANDS + i, B_MEDIA_NO_TYPE,
			"", B_GAIN, "dB", -16.0, 16.0, 0.1);
		label->AddOutput(value);
		value->AddInput(label);
	}
	
	SetParameterWeb(fWeb);
}


void 
EqualizerNode::InitFilter(void)
{
	fEqualizer.SetFormat(fFormat.u.raw_audio.channel_count,
		fFormat.u.raw_audio.frame_rate);
}


bigtime_t 
EqualizerNode::GetFilterLatency(void)
{
	if (fOutputMedia.destination == media_destination::null)
		return 0LL;

	BBufferGroup* test_group =
		new BBufferGroup(fOutputMedia.format.u.raw_audio.buffer_size, 1);

	BBuffer* buffer =
		test_group->RequestBuffer(fOutputMedia.format.u.raw_audio.buffer_size);
	buffer->Header()->type = B_MEDIA_RAW_AUDIO;
	buffer->Header()->size_used = fOutputMedia.format.u.raw_audio.buffer_size;

	bigtime_t begin = system_time();
	FilterBuffer(buffer);
	bigtime_t latency = system_time() - begin;

	buffer->Recycle();
	delete test_group;

	InitFilter();
	
	return latency;
}


void 
EqualizerNode::FilterBuffer(BBuffer* buffer)
{
	uint32 m_frameSize = (fFormat.u.raw_audio.format & 0x0f)
		* fFormat.u.raw_audio.channel_count;
	uint32 samples = buffer->Header()->size_used / m_frameSize;		
	uint32 channels = fFormat.u.raw_audio.channel_count;
	if (fMute != 0)		
		memset(buffer->Data(), 0, buffer->Header()->size_used);
	else if (fByPass == 0)
		fEqualizer.ProcessBuffer((float*)buffer->Data(), samples * channels);		
}
