/*
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */
#ifndef MULTI_AUDIO_NODE_H
#define MULTI_AUDIO_NODE_H


#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <Locker.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <Message.h>
#include <TimeSource.h>

#include "hmulti_audio.h"
#include "MultiAudioDevice.h"
#include "TimeComputer.h"


class BDiscreteParameter;
class BParameterGroup;

class node_input;
class node_output;


class MultiAudioNode : public BBufferConsumer, public BBufferProducer,
		public BTimeSource,	public BMediaEventLooper, public BControllable {
protected:
	virtual					~MultiAudioNode();

public:
							MultiAudioNode(BMediaAddOn* addon, const char* name,
								MultiAudioDevice* device, int32 internalID,
								BMessage* config);

	virtual status_t		InitCheck() const;

	static	void			GetFlavor(flavor_info* info, int32 id);
	static	void			GetFormat(media_format* outFormat);

			status_t		GetConfigurationFor(BMessage* message);

	// BMediaNode methods
	virtual	BMediaAddOn*	AddOn(int32* internalID) const;
	virtual	status_t		HandleMessage(int32 message, const void* data,
								size_t size);

protected:
	virtual	void			Preroll();
	virtual	void			NodeRegistered();
	virtual	status_t		RequestCompleted(const media_request_info& info);
	virtual	void			SetTimeSource(BTimeSource* timeSource);

	// BBufferConsumer methods

	virtual	status_t		AcceptFormat(const media_destination& dest,
								media_format* format);
	virtual	status_t		GetNextInput(int32* cookie, media_input* input);
	virtual	void			DisposeInputCookie(int32 cookie);
	virtual	void			BufferReceived(BBuffer* buffer);
	virtual	void			ProducerDataStatus(const media_destination& forWhom,
								int32 status, bigtime_t atPerformanceTime);
	virtual	status_t		GetLatencyFor(const media_destination& forWhom,
								bigtime_t* latency, media_node_id* timeSource);
	virtual	status_t 		Connected(const media_source& producer,
								const media_destination& where,
								const media_format& withFormat,
								media_input* input);
	virtual	void			Disconnected(const media_source& producer,
								const media_destination& where);
	virtual	status_t		FormatChanged(const media_source& producer,
								const media_destination& consumer,
								int32 changeTag, const media_format& format);

	virtual	status_t		SeekTagRequested(
								const media_destination& destination,
								bigtime_t targetTime, uint32 flags,
								media_seek_tag* _seekTag, bigtime_t* _taggedTime,
								uint32* _flags);

	// BBufferProducer methods

	virtual status_t		FormatSuggestionRequested(media_type type,
								int32 quality, media_format* format);

	virtual status_t		FormatProposal(const media_source& output,
								media_format* format);

	virtual status_t		FormatChangeRequested(const media_source& source,
								const media_destination& destination,
								media_format* io_format, int32* _deprecated);
	virtual status_t		GetNextOutput(int32* cookie,
								media_output* out_output);
	virtual status_t		DisposeOutputCookie(int32 cookie);

	virtual	status_t		SetBufferGroup(const media_source& for_source,
								BBufferGroup* group);

	virtual status_t		PrepareToConnect(const media_source& what,
								const media_destination& where,
								media_format* format, media_source* source,
								char* name);

	virtual void			Connect(status_t error, const media_source& source,
								const media_destination& destination,
								const media_format& format, char* name);
	virtual void			Disconnect(const media_source& what,
								const media_destination& where);

	virtual void			LateNoticeReceived(const media_source& what,
								bigtime_t howMuch, bigtime_t performanceTime);

	virtual void			EnableOutput(const media_source& what, bool enabled,
								int32* _deprecated);
	virtual void			AdditionalBufferRequested(const media_source& source,
								media_buffer_id previousBuffer,
								bigtime_t previousTime,
								const media_seek_tag* previousTag);

	// BMediaEventLooper methods
	virtual void			HandleEvent(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);

	// BTimeSource methods
	virtual void			SetRunMode(run_mode mode);
	virtual status_t		TimeSourceOp(const time_source_op_info& op,
								void *_reserved);

	// BControllable methods
	virtual status_t		GetParameterValue(int32 id, bigtime_t* lastChange,
								void* value, size_t* size);
	virtual void			SetParameterValue(int32 id, bigtime_t when,
								const void* value, size_t size);
	virtual BParameterWeb*	MakeParameterWeb();

private:
	// private unimplemented
							MultiAudioNode(const MultiAudioNode& clone);
	MultiAudioNode& 		operator=(const MultiAudioNode& clone);

			status_t		_HandleStart(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleSeek(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleWarp(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleStop(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleBuffer(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleDataStatus(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);
			status_t		_HandleParameter(const media_timed_event* event,
								bigtime_t lateness, bool realTimeEvent = false);

			char*			_PlaybackBuffer(int32 cycle, int32 channel)
								{ return fDevice->BufferList().playback_buffers
									[cycle][channel].base; }
			uint32			_PlaybackStride(int32 cycle, int32 channel)
								{ return fDevice->BufferList().playback_buffers
									[cycle][channel].stride; }

			void			_WriteZeros(node_input& input, uint32 bufferCycle);
			void			_FillWithZeros(node_input& input);
			void			_FillNextBuffer(node_input& channel,
								BBuffer* buffer);

	static	int32			_OutputThreadEntry(void* data);
			int32			_OutputThread();
			status_t		_StartOutputThreadIfNeeded();
			status_t		_StopOutputThread();

			void 			_AllocateBuffers(node_output& channel);
			BBuffer* 		_FillNextBuffer(multi_buffer_info& info,
								node_output& channel);
			void			_UpdateTimeSource(multi_buffer_info& info,
								multi_buffer_info& oldInfo, node_input& input);

			node_output* 	_FindOutput(media_source source);
			node_input* 	_FindInput(media_destination destination);
			node_input* 	_FindInput(int32 destinationId);

			const char*		_GetControlName(multi_mix_control& control);
			void 			_ProcessGroup(BParameterGroup* group, int32 index,
								int32& numParameters);
			void 			_ProcessMux(BDiscreteParameter* parameter,
								int32 index);
			void			_CreateFrequencyParameterGroup(
								BParameterGroup* parentGroup, const char* name,
								int32 parameterID, uint32 rateMask);

			status_t		_SetNodeInputFrameRate(float frameRate);
			status_t		_SetNodeOutputFrameRate(float frameRate);
			void			_UpdateInternalLatency(const media_format& format);

private:
	status_t			fInitStatus;

	BMediaAddOn*		fAddOn;
	int32				fId;

	BLocker				fBufferLock;

	BList				fInputs;
	TimeComputer		fTimeComputer;

	bigtime_t 			fLatency;
	BList				fOutputs;
	media_format 		fOutputPreferredFormat;
	media_format 		fInputPreferredFormat;

	bigtime_t			fInternalLatency;
		// this is computed from the real (negotiated) chunk size and bit rate,
		// not the defaults that are in the parameters
	bigtime_t			fBufferPeriod;

	sem_id				fBufferFreeSem;
	thread_id			fThread;
	MultiAudioDevice*	fDevice;
	bool 				fTimeSourceStarted;
	BParameterWeb*		fWeb;
	BMessage			fConfig;
};

#endif	// MULTI_AUDIO_NODE_H
