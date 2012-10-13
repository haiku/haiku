/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __VST_NODE_H__
#define __VST_NODE_H__

#include <Controllable.h>
#include <BufferProducer.h>
#include <BufferConsumer.h>
#include <MediaEventLooper.h>

#include "VSTHost.h"

class BMediaAddOn;
class BBufferGroup;

#define BUFF_SIZE 0x2000

#define ID_AUDIO_INPUT	1
#define	ID_AUDIO_OUTPUT 2

//parameter IDs
enum param_id_t {
	P_MUTE		= 50,
	P_BYPASS,
	
	P_PARAM		= 5000,
	P_ABOUT		= 6000,
	P_LABEL		= 7000,
	P_LABEL2	= 8000,		
	P_LABEL3	= 9000,
};


class VSTNode : public	BBufferConsumer,
				public	BBufferProducer,
				public	BControllable,
				public	BMediaEventLooper {	
public:
	virtual 				~VSTNode();
							VSTNode(BMediaAddOn *addon, const char *name, const char* path);
	//BMediaNode
public:
	virtual		BMediaAddOn *AddOn(int32 *id) const;
	virtual		status_t 	HandleMessage(int32 code, const void *data,	size_t size);
protected:
	virtual 	void 		NodeRegistered();
	//BControllable
public:	
	virtual 	status_t 	GetParameterValue(int32 id,	bigtime_t *lastChangeTime,
									void *value, size_t *size);
	virtual 	void 		SetParameterValue(int32 id,	bigtime_t changeTime,
									const void *value, size_t size);
	//BBufferConsumer
public:
	virtual 	void 		BufferReceived(BBuffer *buffer);									
	virtual 	status_t 	AcceptFormat(const media_destination &dst,
							media_format *format);
	virtual 	status_t 	GetNextInput(int32 *cookie,	media_input *input);
	virtual 	void 		DisposeInputCookie(int32 cookie);
	virtual 	status_t 	FormatChanged(const media_source &src,
									const media_destination &dst,
							int32 change_tag, const media_format &format);
	virtual 	void 		ProducerDataStatus(const media_destination &dst,
									int32 status,
									bigtime_t when);	
	virtual 	status_t 	GetLatencyFor(const media_destination &dst,
							bigtime_t *latency,
							media_node_id *time_src);		
	virtual 	status_t 	Connected(const media_source &src, 
									const media_destination &dst,
									const media_format &format,
									media_input *input);
	virtual 	void 		Disconnected(const media_source &src,
									const media_destination &dst);
	//BBufferProducer
public:
	virtual 	status_t 	FormatSuggestionRequested(media_type type,
									int32 quality, media_format *format);
	virtual 	status_t 	FormatProposal(const media_source &src,
									media_format *format);
	virtual 	status_t 	FormatChangeRequested(const media_source &src,
									const media_destination &dst,
									media_format *format,
									int32* _deprecated_);
	virtual 	void 		LateNoticeReceived(const media_source &src,
									bigtime_t late,	bigtime_t when);
	virtual 	status_t	GetNextOutput(int32 *cookie, media_output *output);
	virtual 	status_t 	DisposeOutputCookie(int32 cookie);
	virtual 	status_t	SetBufferGroup(const media_source &src,
									BBufferGroup *group);
	virtual 	status_t 	PrepareToConnect(const media_source &src,
									const media_destination &dst,
									media_format *format, media_source *out_source,
									char *name);
	virtual 	void 		Connect(status_t status,
									const media_source &src,
									const media_destination &dst,
									const media_format &format,
									char* name);
	virtual		void 		Disconnect(const media_source &src,
									const media_destination &dst);		
	virtual 	void 		EnableOutput(const media_source &src,
									bool enabled, int32* _deprecated_);
	virtual 	status_t 	GetLatency(bigtime_t *outLatency);
	virtual 	void 		LatencyChanged(	const media_source &src,
									const media_destination &dst,
									bigtime_t latency, uint32 flags);
	//BMediaEventLooper
protected:									
	virtual 	bigtime_t 	OfflineTime();
	//VSTNode
protected:
	virtual 	void 		HandleEvent(const media_timed_event *event,
									bigtime_t late,
									bool realTimeEvent=false);
				void 		ParameterEventProcessing(const media_timed_event *event);		
				status_t 	ValidateFormat(const media_format& preferredFormat,
									media_format &proposedFormat);
				void 		SetOutputFormat(media_format& ioFormat);
	virtual 	void 		InitParameterValues();
	virtual 	void 		InitParameterWeb();
	virtual 	void 		InitFilter();
	virtual 	bigtime_t 	GetFilterLatency();
	virtual 	void 		FilterBuffer(BBuffer* pBuffer);

private:
	BMediaAddOn				*fAddOn;
	VSTPlugin				*fPlugin;
	BParameterWeb			*fWeb;

	media_format			fPreferredFormat;
	media_format			fFormat;
	media_input				fInputMedia;
	media_output			fOutputMedia;
	bool					fOutputMediaEnabled;
	bigtime_t				fDownstreamLatency;
	bigtime_t				fProcessLatency;
	
	int32					fBlockSize;	
	int32					fMute;
	int32					fByPass;
	bigtime_t				fMuteLastChanged;
	bigtime_t				fByPassLastChanged;
};

#endif //__VST_NODE_H__
