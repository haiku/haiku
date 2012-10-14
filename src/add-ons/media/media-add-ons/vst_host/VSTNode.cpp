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

#include "VSTNode.h"

//VSTNode
VSTNode::~VSTNode()
{
	Quit();
}

VSTNode::VSTNode(BMediaAddOn* addon, const char* name, const char* path)
				: BMediaNode(name),
				BBufferConsumer(B_MEDIA_RAW_AUDIO),
				BBufferProducer(B_MEDIA_RAW_AUDIO),
				BControllable(),
				BMediaEventLooper(),
				fAddOn(addon),
				fOutputMediaEnabled(true),
				fDownstreamLatency(0),
				fProcessLatency(0)
{
	fPlugin = new VSTPlugin();
	fPlugin->LoadModule(path);
}

//BMediaNode
BMediaAddOn* 
VSTNode::AddOn(int32* id) const 
{
	if(fAddOn) {
		*id = 0;
	}
	return fAddOn;
}

status_t 
VSTNode::HandleMessage(int32 message, const void* data, size_t size)
{
	if((BControllable::HandleMessage(message, data, size) != B_OK) &&
		(BBufferConsumer::HandleMessage(message, data, size) != B_OK) &&
		(BBufferProducer::HandleMessage(message, data, size) != B_OK) &&
		(BControllable::HandleMessage(message, data, size) != B_OK) ) {
   			BMediaNode::HandleMessage(message, data, size);
		return B_OK;
	}
	BMediaNode::HandleBadMessage(message, data, size);
	return B_ERROR;		
}

void
VSTNode::NodeRegistered()
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
VSTNode::GetParameterValue(int32 id, bigtime_t* lastChangeTime, void* value,
	size_t *size)
{
	if (*size < sizeof(float) ||
	    *size < sizeof(int32)) {
		return B_NO_MEMORY;
	}

	type_code v_type = B_FLOAT_TYPE;
	
	BParameter *param;	
	for(int i = 0; i < fWeb->CountParameters(); i++) {
		param = fWeb->ParameterAt(i);
		if(param->ID() == id) {
			v_type = param->ValueType();
			break;
		}
	}	

	*size = sizeof(float);

	if (id == P_MUTE) {
		*(int32*)value = fMute;
		*lastChangeTime = fMuteLastChanged;
		return B_OK;
	} else if (id == P_BYPASS) {
		*(int32*)value = fByPass;
		*lastChangeTime = fByPassLastChanged;
		return B_OK;
	} else {
		int32 idx = id - P_PARAM;
		if (idx >= 0 && idx < fPlugin->ParametersCount()) {
			VSTParameter *param = fPlugin->Parameter(idx);
			if (v_type == B_FLOAT_TYPE) {
				*(float*)value = param->Value();
			}
			if (v_type == B_INT32_TYPE) {
				*(int32*)value = (int32)ceil(param->Value());
			}
			*lastChangeTime = param->LastChangeTime();
			return B_OK;
		}
	}		
	return B_ERROR;
}

void
VSTNode::SetParameterValue(int32 id, bigtime_t time, const void* value,
	size_t size)
{		
	int32 idx = id - P_PARAM;
	if ((idx >= 0 && idx < fPlugin->ParametersCount()) || id == P_MUTE ||
		id == P_BYPASS) {
		media_timed_event ev(time, BTimedEventQueue::B_PARAMETER, (void*)value,
			BTimedEventQueue::B_NO_CLEANUP, size, id, "VSTParam");
		//dirty hack for parameter processing (mediakit bug????)
		ParameterEventProcessing(&ev);
		EventQueue()->AddEvent(ev);		
	}
}

//BBufferConsumer
void
VSTNode::BufferReceived(BBuffer* buffer)
{
	if (buffer->Header()->destination != fInputMedia.destination.id) {
		buffer->Recycle();
		return;
	}

	if (fOutputMedia.destination == media_destination::null ||
		!fOutputMediaEnabled) {
		buffer->Recycle();
		return;
	}

	FilterBuffer(buffer);

	status_t err = SendBuffer(buffer, fOutputMedia.source,
		fOutputMedia.destination);
		
	if (err < B_OK) {
		buffer->Recycle();
	}
}

status_t 
VSTNode::AcceptFormat(const media_destination &dst, media_format* format)
{
	if (dst != fInputMedia.destination) {
		return B_MEDIA_BAD_DESTINATION;
	}
	if (format->type != B_MEDIA_RAW_AUDIO) {
		return B_MEDIA_BAD_FORMAT;
	}

	ValidateFormat(
		(fFormat.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
		fFormat : fPreferredFormat, *format);
			
	return B_OK;
}

status_t 
VSTNode::GetNextInput(int32* cookie, media_input* input)
{
	if (*cookie) {
		return B_BAD_INDEX;
	}
	++*cookie;
	*input = fInputMedia;
	return B_OK;
}

void 
VSTNode::DisposeInputCookie(int32 cookie)
{
}

status_t 
VSTNode::FormatChanged(const media_source &src, const media_destination &dst,
							int32 changeTag, const media_format &format)
{
	return B_MEDIA_BAD_FORMAT;
}

void 
VSTNode::ProducerDataStatus(const media_destination &dst, int32 status,
	bigtime_t when)
{
	if (fOutputMedia.destination != media_destination::null) {
		SendDataStatus(status, fOutputMedia.destination, when);
	}
}

status_t 
VSTNode::GetLatencyFor( const media_destination &dst, bigtime_t* latency,
	media_node_id* outTimeSource)
{
	if (dst != fInputMedia.destination) {
		return B_MEDIA_BAD_DESTINATION;
	}

	*latency = fDownstreamLatency + fProcessLatency;
	*outTimeSource = TimeSource()->ID();
	return B_OK;
}

status_t
VSTNode::Connected(const media_source& source,
	const media_destination& destination, const media_format& format,
	media_input* input)
{
	if (destination != fInputMedia.destination) {
		return B_MEDIA_BAD_DESTINATION;
	}
	if (fInputMedia.source != media_source::null) {
		return B_MEDIA_ALREADY_CONNECTED;
	}

	fInputMedia.source = source;
	fInputMedia.format = format;
	*input = fInputMedia;
	fFormat = format;

	return B_OK;
}

void 
VSTNode::Disconnected(const media_source &src, const media_destination &dst)
{
	if(fInputMedia.source!=src || dst!=fInputMedia.destination) {
		return;
	}
	fInputMedia.source = media_source::null;
	if(fOutputMedia.destination == media_destination::null) {
		fFormat.u.raw_audio = media_raw_audio_format::wildcard;
	}
	fInputMedia.format = fFormat;
}

//BBufferProducer
status_t 
VSTNode::FormatSuggestionRequested(media_type type, int32 quality,
	media_format* format)
{
	if (type != B_MEDIA_RAW_AUDIO) {
		return B_MEDIA_BAD_FORMAT;
	}

	if (fFormat.u.raw_audio.format != media_raw_audio_format::wildcard.format) {
		*format = fFormat;	
	} else {
		*format = fPreferredFormat;
	}
	return B_OK;
}

status_t 
VSTNode::FormatProposal(const media_source &src, media_format* format)
{
	if (src != fOutputMedia.source) {
		return B_MEDIA_BAD_SOURCE;
	}

	if (format->type != B_MEDIA_RAW_AUDIO) {
		return B_MEDIA_BAD_FORMAT;
	}

	ValidateFormat(
		(fFormat.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
		fFormat : fPreferredFormat, *format);

	return B_OK;
}

status_t 
VSTNode::FormatChangeRequested(const media_source &src,
	const media_destination &dst, media_format* format, int32* _deprecated_)
{
	return B_MEDIA_BAD_FORMAT;
}

void 
VSTNode::LateNoticeReceived(const media_source &src, 
	bigtime_t late, bigtime_t when)
{
	if (src != fOutputMedia.source || fInputMedia.source == media_source::null) {
		return;
	}	
	NotifyLateProducer(fInputMedia.source, late, when);
}

status_t
VSTNode::GetNextOutput(int32 *cookie, media_output* output)
{
	if (*cookie) {
		return B_BAD_INDEX;
	}
	++*cookie;
	*output = fOutputMedia;
	return B_OK;
}

status_t 
VSTNode::DisposeOutputCookie(int32 cookie) 
{
	return B_OK;
}

status_t
VSTNode::SetBufferGroup(const media_source &src, BBufferGroup* group)
{
	status_t ret;
	int32 changeTag;

	if (src != fOutputMedia.source) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (fInputMedia.source == media_source::null) {
		return B_ERROR;
	}
	ret = SetOutputBuffersFor(fInputMedia.source, fInputMedia.destination,
		group, 0, &changeTag);
		
	return ret;
}

status_t
VSTNode::PrepareToConnect( const media_source &src, const media_destination &dst,
	media_format* format, media_source* out_source, char* name)
{
	status_t ret = B_OK;
	
	if (src != fOutputMedia.source) {
		return B_MEDIA_BAD_SOURCE;
	}

	if (format->type != B_MEDIA_RAW_AUDIO) {
		return B_MEDIA_BAD_FORMAT;
	}
	
	if (fOutputMedia.destination != media_destination::null) {
		return B_MEDIA_ALREADY_CONNECTED;
	}

	ret = ValidateFormat(
		(fFormat.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
		fFormat : fPreferredFormat, *format);
	
	if (ret < B_OK) {
		return ret;
	}

	SetOutputFormat(*format);

	fOutputMedia.destination = dst;
	fOutputMedia.format = *format;

	*out_source = fOutputMedia.source;
	strncpy(name, fOutputMedia.name, B_MEDIA_NAME_LENGTH);

	return B_OK;
}

void
VSTNode::Connect(status_t status, const media_source &src,
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
							EventLatency()+SchedulingLatency());
	}

	bigtime_t duration = 0;
	
	int sample_size = (fFormat.u.raw_audio.format & 0xf)*
						fFormat.u.raw_audio.channel_count;
	if (fFormat.u.raw_audio.buffer_size > 0 &&
		fFormat.u.raw_audio.frame_rate > 0 &&
		sample_size > 0) {
		duration = (bigtime_t)(((fFormat.u.raw_audio.buffer_size / sample_size) /
			fFormat.u.raw_audio.frame_rate) * 1000000.0);
	}
			
	SetBufferDuration(duration);
}

void 
VSTNode::Disconnect(const media_source &src, const media_destination &dst) 
{
	if (src != fOutputMedia.source) {
		return;
	}

	if (dst != fOutputMedia.destination) {
		return;
	}

	fOutputMedia.destination = media_destination::null;

	if (fInputMedia.source == media_source::null) {
		fFormat.u.raw_audio = media_raw_audio_format::wildcard;
	}

	fOutputMedia.format = fFormat;
}

void 
VSTNode::EnableOutput(const media_source &src, bool enabled, int32* _deprecated_)
{
	if (src != fOutputMedia.source) {
		return;
	}
	fOutputMediaEnabled = enabled;
}

status_t 
VSTNode::GetLatency(bigtime_t* latency)
{
	*latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

void 
VSTNode::LatencyChanged(const media_source &src, const media_destination &dst,
	bigtime_t latency, uint32 flags)
{
	if (src != fOutputMedia.source || dst != fOutputMedia.destination) {
		return;
	}

	fDownstreamLatency = latency;
	SetEventLatency(fDownstreamLatency + fProcessLatency);

	if (fInputMedia.source != media_source::null) {
		SendLatencyChange(fInputMedia.source, 
			fInputMedia.destination,EventLatency() + SchedulingLatency());
	}
}

//BMediaEventLooper
bigtime_t 
VSTNode::OfflineTime()
{
	return 0LL;
}

//VSTNode
void
VSTNode::HandleEvent(const media_timed_event *event, bigtime_t late,
	bool realTime)
{	
	if(event->type == BTimedEventQueue::B_PARAMETER) {
		ParameterEventProcessing(event);
	}
}

void 
VSTNode::ParameterEventProcessing(const media_timed_event* event)
{
	float value = 0.0;
	int32 value32 = 0;
	
	int32 id = event->bigdata;
	size_t size = event->data;
	bigtime_t now = TimeSource()->Now();
	
	type_code v_type = B_FLOAT_TYPE;
	
	BParameter* web_param;	
	for(int i = 0; i < fWeb->CountParameters(); i++) {
		web_param = fWeb->ParameterAt(i);
		if(web_param->ID() == id) {
			v_type = web_param->ValueType();
			break;
		}
	}
	
	if (v_type == B_FLOAT_TYPE)
		value = *((float*)event->pointer);
	if (v_type == B_INT32_TYPE) {		
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
	} else {
		int32 idx = id - P_PARAM;
		if (idx >= 0 && idx < fPlugin->ParametersCount()) {
			VSTParameter *param = fPlugin->Parameter(idx);
			param->SetValue(value);
			BroadcastNewParameterValue(now,	id,	&value,	size);
		}
	}
}

status_t
VSTNode::ValidateFormat(const media_format &preferredFormat,
							media_format &proposedFormat)
{
	status_t ret = B_OK;
		
	if (proposedFormat.type != B_MEDIA_RAW_AUDIO) {
		proposedFormat = preferredFormat;
		return B_MEDIA_BAD_FORMAT;
	}

	media_raw_audio_format &wild = media_raw_audio_format::wildcard;
	media_raw_audio_format &f = proposedFormat.u.raw_audio;
	const media_raw_audio_format &pref = preferredFormat.u.raw_audio;

	if(pref.frame_rate != wild.frame_rate && f.frame_rate != pref.frame_rate) {
		if(f.frame_rate != wild.frame_rate) {
			ret = B_MEDIA_BAD_FORMAT;
		}
		f.frame_rate = pref.frame_rate;
	}

	if(pref.channel_count != wild.channel_count &&
		f.channel_count != pref.channel_count) {
		if(f.channel_count != wild.channel_count) {
			ret = B_MEDIA_BAD_FORMAT;
		}
		f.channel_count = pref.channel_count;
	}

	if(pref.format != wild.format && f.format != pref.format) {
		if(f.format != wild.format) {
			ret = B_MEDIA_BAD_FORMAT;
		}
		f.format = pref.format;
	}

	if(pref.byte_order != wild.byte_order &&
		f.byte_order != pref.byte_order) {
		if(f.byte_order != wild.byte_order) {
			ret = B_MEDIA_BAD_FORMAT;
		}
		f.byte_order = pref.byte_order;
	}

	if(pref.buffer_size != wild.buffer_size &&
		f.buffer_size != pref.buffer_size) {
		if(f.buffer_size != wild.buffer_size) {
			ret = B_MEDIA_BAD_FORMAT;
		}
		f.buffer_size = pref.buffer_size;
	}
	
	return ret;
}


void
VSTNode::SetOutputFormat(media_format &format)
{
	media_raw_audio_format &f = format.u.raw_audio;
	media_raw_audio_format &w = media_raw_audio_format::wildcard;

	if (f.frame_rate == w.frame_rate) {
		f.frame_rate = 44100.0;
	}
	if (f.channel_count == w.channel_count) {
		if(fInputMedia.source != media_source::null) {
			f.channel_count = fInputMedia.format.u.raw_audio.channel_count;
		} else {
			f.channel_count = fPlugin->Channels(VST_OUTPUT_CHANNELS);
		}
	}
	if (f.format == w.format) {
		f.format = media_raw_audio_format::B_AUDIO_FLOAT;
	}
	if (f.byte_order == w.format) {
		f.byte_order = (B_HOST_IS_BENDIAN) ?
			B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	}
	if (f.buffer_size == w.buffer_size) {
		f.buffer_size = BUFF_SIZE;
	}
}

void 
VSTNode::InitParameterValues()
{	
	fMute = 0;
	fByPass = 0;
	fMuteLastChanged = 0LL;
	fByPassLastChanged = 0LL;
}

void 
VSTNode::InitParameterWeb()
{
	fWeb = new BParameterWeb();

	bool switch_group_needed = false;
	for(int i = 0; i < fPlugin->ParametersCount(); i++) {
		VSTParameter* param = fPlugin->Parameter(i);
		if (param->Type() == VST_PARAM_CHECKBOX ||
		   param->Type() == VST_PARAM_DROPLIST) {
		   switch_group_needed = true;
		   break;
		}
	}

	BParameterGroup* fParamGroup = fWeb->MakeGroup("Parameters");	
	BParameterGroup* fSwitchesGroup = switch_group_needed ?
		fWeb->MakeGroup("Switches") : NULL;
	BParameterGroup* fAboutGroup = fWeb->MakeGroup("About");

	BParameter* value;
	BNullParameter* label;
	BParameterGroup* group;

	BParameterGroup* fFControlGroup = fParamGroup->MakeGroup("FilterControl");
	BParameterGroup* fCheckBoxGroup = switch_group_needed ?
		fSwitchesGroup->MakeGroup("CheckBoxes") : NULL;
	BParameterGroup* fSelectorsGroup = switch_group_needed ?
		fSwitchesGroup->MakeGroup("Selectors") : NULL;
	
	fFControlGroup->MakeDiscreteParameter(P_MUTE, 
		B_MEDIA_NO_TYPE,"Mute", B_ENABLE);
	fFControlGroup->MakeDiscreteParameter(P_BYPASS,
		B_MEDIA_NO_TYPE,"ByPass", B_ENABLE);
	
	for(int i = 0; i < fPlugin->ParametersCount(); i++) {
		VSTParameter *param = fPlugin->Parameter(i);
		switch(param->Type()) {
			case VST_PARAM_CHECKBOX:
			{
				BString str;
				str << param->Name() << " (" << param->MinimumValue()
					<< "/" << param->MaximumValue() << ")";
				value = fCheckBoxGroup->MakeDiscreteParameter(
					P_PARAM + param->Index(), B_MEDIA_NO_TYPE,
					str.String(), B_ENABLE);
				break;
 			}
			case VST_PARAM_DROPLIST:
			{
				BDiscreteParameter *dvalue =
					fSelectorsGroup->MakeDiscreteParameter(P_PARAM + param->Index(),
						B_MEDIA_NO_TYPE, param->Name(), B_OUTPUT_MUX);
				for(int j = 0; j < param->ListCount(); j++) {
					dvalue->AddItem( param->ListItemAt(j)->Index,
						param->ListItemAt(j)->Name.String());
				}
				break;
			}
			//sliders
			default:
			{
				BString str;
				group = fParamGroup->MakeGroup(param->Name());
				label = group->MakeNullParameter(P_LABEL + param->Index(), 
								B_MEDIA_NO_TYPE, param->Name(), B_GENERIC);
			
				str.SetTo(param->MaximumValue());
				str << " " << param->Unit();
				
				group->MakeNullParameter(P_LABEL2 + param->Index(), 
								B_MEDIA_NO_TYPE, str.String(), B_GENERIC);			
				value = group->MakeContinuousParameter(P_PARAM + param->Index(), 
								B_MEDIA_NO_TYPE, "", B_GAIN, "", 0.0, 1.0, 0.01);
				label->AddOutput(value);
				value->AddInput(label);
			
				str.SetTo(param->MinimumValue());
				str << " " << param->Unit();
				
				group->MakeNullParameter(P_LABEL3 + param->Index(), 
								B_MEDIA_NO_TYPE, str.String(), B_GENERIC);
				break;
			}
		}		
	}
	
	BString str("About plugin");
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 0, 
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);	

	str.SetTo("Effect name: ");
	if (strlen(fPlugin->EffectName()) != 0) {
		str.Append(fPlugin->EffectName());
	} else {
		str.Append("not specified");
	}	
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 1, 
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);
	
	str.SetTo("Vendor: ");
	if (strlen(fPlugin->Vendor()) != 0) {
		str.Append(fPlugin->Vendor());
	} else {
		str.Append("not specified");
	}		
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 2, 
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);
	
	str.SetTo("Product: ");
	if (strlen(fPlugin->Product()) != 0) {
		str.Append(fPlugin->Product());
	} else {
		str.Append("not specified");
	}
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 3,
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);

	str.SetTo("Input Channels: ");
	str<<fPlugin->Channels(VST_INPUT_CHANNELS);
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 4,
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);

	str.SetTo("Output Channels: ");
	str<<fPlugin->Channels(VST_OUTPUT_CHANNELS);
	label = fAboutGroup->MakeNullParameter(P_ABOUT + 5,
						B_MEDIA_NO_TYPE, str.String(), B_GENERIC);
		
	SetParameterWeb(fWeb);
}

void 
VSTNode::InitFilter()
{	
	fBlockSize = fFormat.u.raw_audio.buffer_size / 
				(fFormat.u.raw_audio.channel_count * sizeof(float));

	fPlugin->SetBlockSize(fBlockSize);
	fPlugin->SetSampleRate(fFormat.u.raw_audio.frame_rate);
}

bigtime_t 
VSTNode::GetFilterLatency()
{
	if (fOutputMedia.destination == media_destination::null) {
		return 0LL;
	}

	BBufferGroup* temp_group =
		new BBufferGroup(fOutputMedia.format.u.raw_audio.buffer_size, 1);

	BBuffer *buffer =
		temp_group->RequestBuffer(fOutputMedia.format.u.raw_audio.buffer_size);
	buffer->Header()->type = B_MEDIA_RAW_AUDIO;
	buffer->Header()->size_used = fOutputMedia.format.u.raw_audio.buffer_size;

	bigtime_t begin = system_time();
	FilterBuffer(buffer);
	bigtime_t latency = system_time()-begin;

	InitFilter();

	buffer->Recycle();
	delete temp_group;
	
	return latency;
}

void 
VSTNode::FilterBuffer(BBuffer* buffer)
{
	uint32 m_frameSize = (fFormat.u.raw_audio.format & 0x0f)*
				 fFormat.u.raw_audio.channel_count;
	uint32 samples = buffer->Header()->size_used / m_frameSize;		
	uint32 channels = fFormat.u.raw_audio.channel_count;
	
	if (fMute != 0) {		
		memset(buffer->Data(), 0, buffer->Header()->size_used);
	} else {
		if (fByPass == 0) {
			fPlugin->Process((float*)buffer->Data(), samples, channels);
		}
	}
}
