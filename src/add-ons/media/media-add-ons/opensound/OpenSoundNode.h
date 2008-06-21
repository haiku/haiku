/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef _OPENSOUNDNODE_H
#define _OPENSOUNDNODE_H

#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <MediaEventLooper.h>
#include <MediaNode.h>
#include <Message.h>
#include <ParameterWeb.h>
#include <TimeSource.h>

class OpenSoundDevice;
class OpenSoundDeviceEngine;
class OpenSoundDeviceMixer;

struct audio_buf_info;
struct flavor_info;

class OpenSoundNode : public BBufferConsumer, public BBufferProducer,
	public BTimeSource, public BMediaEventLooper, public BControllable {

private:
	class NodeInput;
	class NodeOutput;

protected:
	virtual						~OpenSoundNode();

public:
	explicit					OpenSoundNode(BMediaAddOn* addon,
									const char* name,
									OpenSoundDevice* device,
									int32 internalID, BMessage* config);

	virtual	status_t			InitCheck() const;

	// BMediaNode interface
public:
	virtual	BMediaAddOn*		AddOn(int32* internalID) const;
		// Who instantiated you -- or NULL for app class

protected:
	virtual	void				Preroll();
		// These don't return errors; instead, they use the global error
		// condition reporter. A node is required to have a queue of at
		// least one pending command (plus TimeWarp) and is recommended to
		// allow for at least one pending command of each type. Allowing an
		// arbitrary number of outstanding commands might be nice, but apps
		// cannot depend on that happening.

public:
	virtual	status_t			HandleMessage(int32 message,
									const void* data, size_t size);
				
protected:
	virtual	void				NodeRegistered();
	virtual	status_t			RequestCompleted(
									const media_request_info& info);
	virtual	void				SetTimeSource(BTimeSource* timeSource);

	// BBufferConsumer interface
protected:
	virtual	status_t			AcceptFormat(
									const media_destination& dest,
									media_format* format);
	virtual	status_t			GetNextInput(int32* cookie,
									media_input* out_input);
	virtual	void				DisposeInputCookie(int32 cookie);
	virtual	void				BufferReceived(BBuffer* buffer);
	virtual	void				ProducerDataStatus(
									const media_destination& for_whom,
									int32 status,
									bigtime_t at_performance_time);
	virtual	status_t			GetLatencyFor(
									const media_destination& for_whom,
									bigtime_t* out_latency,
									media_node_id* out_timesource);
	virtual	status_t			Connected(const media_source& producer,
									const media_destination& where,
									const media_format& with_format,
									media_input* out_input);
			// here's a good place to request buffer group usage

	virtual	void				Disconnected(const media_source& producer,
									const media_destination& where);

	virtual	status_t			FormatChanged(const media_source& producer,
									const media_destination& consumer, 
								int32 change_tag,
								const media_format& format);

	virtual	status_t			SeekTagRequested(
									const media_destination& destination,
									bigtime_t in_target_time,
									uint32 in_flags, 
									media_seek_tag* out_seek_tag,
									bigtime_t* out_tagged_time,
									uint32* out_flags);

	// BBufferProducer interface
protected:
	virtual	status_t			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* format);
	
	virtual	status_t			FormatProposal(const media_source& output,
									media_format* format);
	
	virtual	status_t			FormatChangeRequested(
									const media_source& source,
									const media_destination& destination,
									media_format* io_format,
									int32* _deprecated_);
	virtual	status_t			GetNextOutput(int32* cookie,
									media_output* out_output);
	virtual	status_t			DisposeOutputCookie(int32 cookie);
	
	virtual	status_t			SetBufferGroup(
									const media_source& for_source,
									BBufferGroup* group);
	
	virtual	status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format,
									media_source* out_source,
									char* out_name);
	
	virtual	void				Connect(status_t error, 
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* io_name);
	
	virtual	void				Disconnect(const media_source& what,
									const media_destination& where);
	
	virtual	void				LateNoticeReceived(
									const media_source& what,
									bigtime_t how_much,
									bigtime_t performance_time);
	
	virtual	void				EnableOutput(const media_source& what,
									bool enabled, int32* _deprecated_);
	virtual	void				AdditionalBufferRequested(
									const media_source& source,
									media_buffer_id prev_buffer,
									bigtime_t prev_time,
									const media_seek_tag* prev_tag);

	// BMediaEventLooper interface
protected:
		/* you must override to handle your events! */
		/* you should not call HandleEvent directly */
	virtual	void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

	// BTimeSource interface
protected:
	virtual	void				SetRunMode(run_mode mode);
	virtual	status_t			TimeSourceOp(const time_source_op_info& op, 
									void* _reserved);

	//  BControllable interface
protected:
	virtual	status_t			GetParameterValue(int32 id,
									bigtime_t* last_change,
									void* value, size_t* ioSize);
	virtual	void				SetParameterValue(int32 id,
									bigtime_t when, const void* value,
									size_t size);
	virtual	BParameterWeb*		MakeParameterWeb();

private:
			void 				_ProcessGroup(BParameterGroup* group,
									int32 index, int32& _parameters);
			void 				_ProcessMux(BDiscreteParameter* parameter,
									int32 index);
			status_t			_PropagateParameterChanges(int from,
									int type, const char* id);
		
protected:
	virtual	status_t			HandleStart(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleSeek(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleWarp(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleStop(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleBuffer(
									const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleDataStatus(
									const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);
	virtual	status_t			HandleParameter(
									const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

public:
	static	void				GetFlavor(flavor_info* outInfo, int32 id);
	static	void				GetFormat(media_format* outFormat);

			status_t			GetConfigurationFor(BMessage* intoMessage);


private:
								OpenSoundNode(const OpenSoundNode& clone);
									// private unimplemented
			OpenSoundNode&		operator=(const OpenSoundNode& clone);	
				
private:
	static	void				_SignalHandler(int sig);

	static	int32				_PlayThreadEntry(void* data);
	static	int32				_RecThreadEntry(void* data);
			int32				_PlayThread(NodeInput* input);
			int32				_RecThread(NodeOutput* output);

			status_t			_StartPlayThread(NodeInput* input);
			status_t			_StopPlayThread(NodeInput* input);
			status_t			_StartRecThread(NodeOutput* output);
			status_t			_StopRecThread(NodeOutput* output);
		
			BBuffer* 			_FillNextBuffer(audio_buf_info* abinfo,
									NodeOutput& channel);
			void				_UpdateTimeSource(bigtime_t performanceTime,
									bigtime_t realTime, float drift);
		
		
			NodeOutput* 		_FindOutput(
									const media_source& source) const;
			NodeInput* 			_FindInput(
									const media_destination& dest) const;
			NodeInput* 			_FindInput(int32 destinationId);
		
private:
			status_t			fInitCheckStatus;

			BMediaAddOn*		fAddOn;
			int32				fId;

			BList				fInputs;
			BList				fOutputs;
		
#if 1
			// TODO: remove and use use a preferred format
			// per NodeInput/NodeOutput channel
			media_format 		fPreferredFormat;
#endif
			bigtime_t 			fLatency;
			bigtime_t			fInternalLatency;
				// this is computed from the real (negotiated) chunk size
				// and bit rate, not the defaults that are in the
				// parameters
			OpenSoundDevice*	fDevice;

			bool 				fTimeSourceStarted;
			bigtime_t			fTimeSourceStartTime;

			BParameterWeb*		fWeb;
			BMessage			fConfig;
};

#endif // _OPENSOUNDNODE_H
