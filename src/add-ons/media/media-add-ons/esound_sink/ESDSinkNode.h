/*
 * ESounD media addon for BeOS
 *
 * Copyright (c) 2006 François Revol (revol@free.fr)
 * 
 * Based on Multi Audio addon for Haiku,
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _ESDSINK_NODE_H
#define _ESDSINK_NODE_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <FileInterface.h>
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <Controllable.h>
#include <File.h>
#include <Entry.h>
#include "ESDEndpoint.h"

//#define ENABLE_INPUT 1
//#define ENABLE_TS 1

/*bool format_is_acceptible(
						const media_format & producer_format,
						const media_format & consumer_format);*/


enum {
	PARAM_ENABLED,
	PARAM_HOST,
	PARAM_PORT
};

class ESDSinkNode :
    public BBufferConsumer,
#if ENABLE_INPUT
    public BBufferProducer,
#endif
#ifdef ENABLE_TS
    public BTimeSource,
#endif
    public BMediaEventLooper,
    public BControllable
{
protected:
virtual ~ESDSinkNode(void);

public:

explicit ESDSinkNode(BMediaAddOn *addon, char* name, BMessage * config);

virtual status_t InitCheck(void) const;

/*************************/
/* begin from BMediaNode */
public:
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;	/* Who instantiated you -- or NULL for app class */

protected:
		/* These don't return errors; instead, they use the global error condition reporter. */
		/* A node is required to have a queue of at least one pending command (plus TimeWarp) */
		/* and is recommended to allow for at least one pending command of each type. */
		/* Allowing an arbitrary number of outstanding commands might be nice, but apps */
		/* cannot depend on that happening. */
virtual	void Preroll(void);

public:
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
				
protected:
virtual		void NodeRegistered(void);	/* reserved 2 */
virtual		status_t RequestCompleted(const media_request_info &info);
virtual		void SetTimeSource(BTimeSource *timeSource);

/* end from BMediaNode */
/***********************/

/******************************/
/* begin from BBufferConsumer */

//included from BMediaAddOn
//virtual	status_t HandleMessage(
//				int32 message,
//				const void * data,
//				size_t size);

	/* Someone, probably the producer, is asking you about this format. Give */
	/* your honest opinion, possibly modifying *format. Do not ask upstream */
	/* producer about the format, since he's synchronously waiting for your */
	/* reply. */
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput(
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	void ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,	/* here's a good place to request buffer group usage */
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
	/* The notification comes from the upstream producer, so he's already cool with */
	/* the format; you should not ask him about it in here. */
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format);

	/* Given a performance time of some previous buffer, retrieve the remembered tag */
	/* of the closest (previous or exact) performance time. Set *out_flags to 0; the */
	/* idea being that flags can be added later, and the understood flags returned in */
	/* *out_flags. */
virtual	status_t SeekTagRequested(
				const media_destination & destination,
				bigtime_t in_target_time,
				uint32 in_flags, 
				media_seek_tag * out_seek_tag,
				bigtime_t * out_tagged_time,
				uint32 * out_flags);

/* end from BBufferConsumer */
/****************************/

/******************************/
/* begin from BBufferProducer */
#if 0
		virtual status_t 	FormatSuggestionRequested(	media_type type,
														int32 quality,
														media_format* format);
		
		virtual status_t 	FormatProposal(				const media_source& output,
														media_format* format);
		
		virtual status_t 	FormatChangeRequested(		const media_source& source,
														const media_destination& destination,
														media_format* io_format,
														int32* _deprecated_);
		virtual status_t 	GetNextOutput(				int32* cookie,
														media_output* out_output);
		virtual status_t 	DisposeOutputCookie(		int32 cookie);
		
		virtual	status_t 	SetBufferGroup(				const media_source& for_source,
														BBufferGroup* group);
		
		virtual status_t 	PrepareToConnect(			const media_source& what,
														const media_destination& where,
														media_format* format,
														media_source* out_source,
														char* out_name);
		
		virtual void 		Connect(					status_t error, 
														const media_source& source,
														const media_destination& destination,
														const media_format& format,
														char* io_name);
		
		virtual void 		Disconnect(					const media_source& what,
														const media_destination& where);
		
		virtual void 		LateNoticeReceived(			const media_source& what,
														bigtime_t how_much,
														bigtime_t performance_time);
		
		virtual void 		EnableOutput(				const media_source & what,
														bool enabled,
														int32* _deprecated_);
		virtual void 		AdditionalBufferRequested(	const media_source& source,
														media_buffer_id prev_buffer,
														bigtime_t prev_time,
														const media_seek_tag* prev_tag);
#endif
/* end from BBufferProducer */
/****************************/

/*****************/
/* BControllable */
/*****************/

/********************************/
/* start from BMediaEventLooper */

	protected:
		/* you must override to handle your events! */
		/* you should not call HandleEvent directly */
		virtual void		HandleEvent(	const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent = false);

/* end from BMediaEventLooper */
/******************************/

/********************************/
/* start from BTimeSource */
#ifdef ENABLE_TS
	protected:
		virtual void		SetRunMode(		run_mode mode);
		virtual status_t 	TimeSourceOp(	const time_source_op_info &op, 
											void *_reserved);
#endif
/* end from BTimeSource */
/******************************/

/********************************/
/* start from BControllable */
	protected:
		virtual status_t 	GetParameterValue(	int32 id,
												bigtime_t* last_change,
												void* value,
												size_t* ioSize);
		virtual void 		SetParameterValue(	int32 id,
												bigtime_t when,
												const void* value,
												size_t size);
		virtual BParameterWeb* MakeParameterWeb();
		
/* end from BControllable */
/******************************/

protected:

virtual status_t HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleBuffer(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);
virtual status_t HandleParameter(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false);

public:

static void GetFlavor(flavor_info * outInfo, int32 id);
static void GetFormat(media_format * outFormat);

status_t GetConfigurationFor(BMessage * into_message);


private:

		ESDSinkNode(	/* private unimplemented */
				const ESDSinkNode & clone);
		ESDSinkNode & operator=(
				const ESDSinkNode & clone);	
		
#if 0
		
		void 				AllocateBuffers(node_output &channel);
		BBuffer* 			FillNextBuffer(	multi_buffer_info &MBI,
										node_output &channel);
		void				UpdateTimeSource(multi_buffer_info &MBI,
										multi_buffer_info &oldMBI,
										node_input &input);
#endif
//		node_output* 		FindOutput(media_source source);
//		node_input* 		FindInput(media_destination dest);
//		node_input* 		FindInput(int32 destinationId);
		
//		void 				ProcessGroup(BParameterGroup *group, int32 index, int32 &nbParameters);
//		void 				ProcessMux(BDiscreteParameter *parameter, int32 index);
		
		status_t fInitCheckStatus;

		BMediaAddOn 		*fAddOn;
		int32				fId;

		BList				fInputs;
		media_input			fInput;
		
		bigtime_t 			fLatency;
		BList				fOutputs;
		media_output		fOutput;
		media_format 		fPreferredFormat;
		
		bigtime_t fInternalLatency;
		// this is computed from the real (negotiated) chunk size and bit rate,
		// not the defaults that are in the parameters
		bigtime_t fBufferPeriod;
		
		
		//volatile uint32 	fBufferCycle;
		sem_id				fBuffer_free;
				
		
		thread_id			fThread;
		
		BString				fHostname;
		uint16				fPort;
		bool				fEnabled;
		ESDEndpoint		 	*fDevice;
		
		//multi_description	MD;
		//multi_format_info 	MFI;
		//multi_buffer_list 	MBL;
		
		//multi_mix_control_info MMCI;
		//multi_mix_control	MMC[MAX_CONTROLS];
		
		bool 				fTimeSourceStarted;
		
		BParameterWeb		*fWeb;
		
		BMessage			fConfig;
};

#endif /* _ESDSINK_NODE_H */
