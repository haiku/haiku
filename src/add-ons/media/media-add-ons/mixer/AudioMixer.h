/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUDIOMIXER_H
#define _AUDIOMIXER_H

#include <BufferConsumer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>

class MixerCore;

class AudioMixer :
		public BBufferConsumer,
		public BBufferProducer,
		public BControllable,
		public BMediaEventLooper
{
	public:
							AudioMixer(BMediaAddOn *addOn, bool isSystemMixer);
							~AudioMixer();

		void				DisableNodeStop();
		
		// AudioMixer support
		void				ApplySettings();

		void				PublishEventLatencyChange();
		void				UpdateParameterWeb();

		void				HandleInputBuffer(BBuffer *buffer, bigtime_t lateness);

		BBufferGroup *		CreateBufferGroup();

		float				dB_to_Gain(float db);
		float				Gain_to_dB(float gain);

		// BMediaNode methods
		BMediaAddOn *		AddOn(int32 *internal_id) const;
		void				NodeRegistered();
		void 				Stop(bigtime_t performance_time, bool immediate);
		void 				SetTimeSource(BTimeSource * time_source);
		using BBufferProducer::SendBuffer;

	protected:
		// BControllable methods
		status_t 			GetParameterValue(int32 id, 
											bigtime_t *last_change,
											void *value,
											size_t *ioSize);

		void				SetParameterValue(int32 id, bigtime_t when,
											const void *value,
											size_t size);

		// BBufferConsumer methods
		status_t			HandleMessage(int32 message, const void* data,
											size_t size);
		status_t			AcceptFormat(const media_destination &dest,
											media_format *format);
		status_t			GetNextInput(int32 *cookie,
											media_input *out_input);
		void				DisposeInputCookie(int32 cookie);
		void				BufferReceived(BBuffer *buffer);
		void				ProducerDataStatus(const media_destination &for_whom,
											int32 status, 
											bigtime_t at_performance_time);
		status_t			GetLatencyFor(const media_destination &for_whom,
											bigtime_t *out_latency, 
											media_node_id *out_timesource);
		status_t			Connected(const media_source &producer,
											const media_destination &where,
											const media_format &with_format,
											media_input *out_input);
		void				Disconnected(const media_source &producer,
											const media_destination &where);
		status_t			FormatChanged(const media_source &producer,
											const media_destination &consumer,
											int32 change_tag,
											const media_format &format);

		// BBufferProducer methods
		status_t 			FormatSuggestionRequested(media_type type,
											int32 quality,
											media_format *format);
		status_t 			FormatProposal(const media_source &output,
											media_format *format);
		status_t			FormatChangeRequested(
											const media_source& source,
											const media_destination &destination,
											media_format *io_format,
											int32 *_deprecated_);
		status_t			GetNextOutput(int32 *cookie,media_output *out_output);
		status_t			DisposeOutputCookie(int32 cookie);
		status_t			SetBufferGroup(const media_source &for_source,
											BBufferGroup *group);
		status_t			GetLatency(bigtime_t *out_latency);
		status_t			PrepareToConnect(const media_source &what,
											const media_destination &where,
											media_format *format,
											media_source *out_source,
											char *out_name);
		void				Connect(status_t error,
											const media_source &source,
											const media_destination &destination,
											const media_format &format,
											char *io_name);
		void				Disconnect(const media_source &what,
											const media_destination &where);
		void				LateNoticeReceived(const media_source &what,
											bigtime_t how_much,
											bigtime_t performance_time);
		void				EnableOutput(const media_source &what,
											bool enabled,
											int32 *_deprecated_);
		void				LatencyChanged(const media_source &source,
											const media_destination &destination,
											bigtime_t new_latency, uint32 flags);

		// BMediaEventLooper methods
		void				HandleEvent(const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent = false);

	private:
		BMediaAddOn			*fAddOn;
		MixerCore			*fCore;
		BParameterWeb		*fWeb; // local pointer to parameterweb
		BBufferGroup		*fBufferGroup;
		bigtime_t			fDownstreamLatency;
		bigtime_t			fInternalLatency;
		bool				fDisableStop;
		media_format		fDefaultFormat;
};
#endif
