/*
 * Copyright 2002 David Shipman,
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H


#include <BufferConsumer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>


class MixerCore;
class MixerOutput;


class AudioMixer : public BBufferConsumer, public BBufferProducer,
	public BControllable, public BMediaEventLooper {
public:
								AudioMixer(BMediaAddOn* addOn,
									bool isSystemMixer);
	virtual						~AudioMixer();

			void				DisableNodeStop();

	// AudioMixer support
			void				ApplySettings();

			void				PublishEventLatencyChange();
			void				UpdateParameterWeb();

			void				HandleInputBuffer(BBuffer* buffer,
									bigtime_t lateness);

			BBufferGroup*		CreateBufferGroup();

			status_t			SendBuffer(BBuffer* buffer,
									MixerOutput* output);

			float				dB_to_Gain(float db);
			float				Gain_to_dB(float gain);

	// BMediaNode methods
	virtual	BMediaAddOn*		AddOn(int32* _internalID) const;
	virtual	void				NodeRegistered();
	virtual	void 				Stop(bigtime_t performanceTime, bool immediate);
	virtual	void 				SetTimeSource(BTimeSource* timeSource);

protected:
	// BControllable methods
	virtual	status_t 			GetParameterValue(int32 id,
									bigtime_t* _lastChange, void* _value,
									size_t* _size);
	virtual	void				SetParameterValue(int32 id, bigtime_t when,
									const void* value, size_t size);

	// BBufferConsumer methods
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);
	virtual	status_t			AcceptFormat(const media_destination& dest,
									media_format* format);
	virtual	status_t			GetNextInput(int32* cookie,
									media_input* _input);
	virtual	void				DisposeInputCookie(int32 cookie);
	virtual	void				BufferReceived(BBuffer *buffer);
	virtual	void				ProducerDataStatus(
									const media_destination& forWhom,
									int32 status, bigtime_t atPerformanceTime);
	virtual	status_t			GetLatencyFor(const media_destination& forWhom,
									bigtime_t* _latency,
									media_node_id* _timesource);
	virtual	status_t			Connected(const media_source& producer,
									const media_destination& where,
									const media_format& withFormat,
									media_input* _input);
	virtual	void				Disconnected(const media_source& producer,
									const media_destination& where);
	virtual	status_t			FormatChanged(const media_source& producer,
									const media_destination& consumer,
									int32 changeTag,
									const media_format& format);

	// BBufferProducer methods
	virtual	status_t 			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format);
	virtual	status_t 			FormatProposal(const media_source& output,
									media_format* format);
	virtual	status_t			FormatChangeRequested(
									const media_source& source,
									const media_destination &destination,
									media_format* format,
									int32* /*deprecated*/);
	virtual	status_t			GetNextOutput(int32* cookie,
									media_output* _output);
	virtual	status_t			DisposeOutputCookie(int32 cookie);
	virtual	status_t			SetBufferGroup(const media_source& source,
									BBufferGroup* group);
	virtual	status_t			GetLatency(bigtime_t* _latency);
	virtual	status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format, media_source* _source,
									char* _name);
	virtual	void				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format, char *_name);
	virtual	void				Disconnect(const media_source& what,
									const media_destination& where);
	virtual	void				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceTime);
	virtual	void				EnableOutput(const media_source& what,
									bool enabled, int32* /*_deprecated_*/);
	virtual	void				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t newLatency, uint32 flags);

		// BMediaEventLooper methods
	virtual	void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

private:
			BMediaAddOn*		fAddOn;
			MixerCore*			fCore;
			BParameterWeb*		fWeb;
			BBufferGroup*		fBufferGroup;
			bigtime_t			fDownstreamLatency;
			bigtime_t			fInternalLatency;
			bool				fDisableStop;
			media_format		fDefaultFormat;
			bigtime_t			fLastLateNotification;
			bigtime_t			fLastLateness;
};


#endif	// AUDIO_MIXER_H
