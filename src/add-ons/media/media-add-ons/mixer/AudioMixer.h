// AudioMixer.h
/*

	By David Shipman, 2002

*/

#ifndef _AUDIOMIXER_H
#define _AUDIOMIXER_H

// forward declarations

class MixerCore;


// includes

#include <media/BufferGroup.h>
#include <media/MediaNode.h>
#include <media/BufferConsumer.h>
#include <media/BufferProducer.h>
#include <media/Controllable.h>
#include <media/MediaEventLooper.h>

// ----------------
// AudioMixer class

class AudioMixer :
		public BBufferConsumer,
		public BBufferProducer,
		public BControllable,
		public BMediaEventLooper
{

	public:
					
					AudioMixer(BMediaAddOn *addOn);
					~AudioMixer();
		
		
		void				DisableNodeStop();
		
	// AudioMixer support
					
		void				UpdateParameterWeb();
		
		void				HandleInputBuffer(BBuffer *buffer, bigtime_t lateness);
		
		BBufferGroup *		CreateBufferGroup();
					
	// BMediaNode methods

		BMediaAddOn		*	AddOn(int32*) const;
	//	void 			SetRunMode(run_mode);
	//	void Preroll();
		//	status_t RequestCompleted(const media_request_info & info);
		void				NodeRegistered();
		void 				Stop(bigtime_t performance_time, bool immediate);
		void 				SetTimeSource(BTimeSource * time_source);
		
		using BBufferProducer::SendBuffer;
	
	protected:
	
	// BControllable methods
	
		status_t 			GetParameterValue( int32 id, bigtime_t* last_change,
								void* value, 
								size_t* ioSize);

		void				SetParameterValue( int32 id, bigtime_t when, 
								const void* value, 
								size_t size);
							
	// BBufferConsumer methods
	
		status_t			HandleMessage( int32 message, 
								const void* data, 
								size_t size);
		
		status_t			AcceptFormat( const media_destination &dest, media_format *format);
		
		status_t			GetNextInput( int32 *cookie, 
								media_input *out_input);
		
		void				DisposeInputCookie( int32 cookie);
		
		void				BufferReceived( BBuffer *buffer);
		
		void				ProducerDataStatus( const media_destination &for_whom, 
								int32 status, 
								bigtime_t at_performance_time);
								
		status_t			GetLatencyFor( const media_destination &for_whom, 
								bigtime_t *out_latency, 
								media_node_id *out_timesource);
		
		status_t			Connected( const media_source &producer, 
								const media_destination &where, 
								const media_format &with_format, 
								media_input *out_input);
		
		void				Disconnected( const media_source &producer, 
								const media_destination &where);
		
		status_t			FormatChanged( const media_source &producer, 
								const media_destination &consumer, 
								int32 change_tag, 
								const media_format &format);

			
	//	
	// BBufferProducer methods
	//
				
		status_t 			FormatSuggestionRequested( media_type type,
								int32 quality,
								media_format *format);
								
		status_t 			FormatProposal(
		const media_source& output,
		media_format* format);
//
//	/* If the format isn't good, put a good format into *io_format and return error */
//	/* If format has wildcard, specialize to what you can do (and change). */
//	/* If you can change the format, return OK. */
//	/* The request comes from your destination sychronously, so you cannot ask it */
//	/* whether it likes it -- you should assume it will since it asked. */
	status_t FormatChangeRequested(
		const media_source& source,
		const media_destination& destination,
		media_format* io_format,
		int32* _deprecated_);

	status_t GetNextOutput(	/* cookie starts as 0 */
		int32* cookie,
		media_output* out_output);

	status_t DisposeOutputCookie(
		int32 cookie);

	status_t SetBufferGroup(
		const media_source& for_source,
		BBufferGroup* group);

	status_t GetLatency(
		bigtime_t* out_latency);

	status_t PrepareToConnect(
		const media_source& what,
		const media_destination& where,
		media_format* format,
		media_source* out_source,
		char* out_name);

	void Connect(
		status_t error, 
		const media_source& source,
		const media_destination& destination,
		const media_format& format,
		char* io_name);

	void Disconnect(
		const media_source& what,
		const media_destination& where);

	void LateNoticeReceived(
		const media_source& what,
		bigtime_t how_much,
		bigtime_t performance_time);

	void EnableOutput(
		const media_source & what,
		bool enabled,
		int32* _deprecated_);
								
	// BMediaEventLooper methods
	
		void				HandleEvent( const media_timed_event *event, 
								bigtime_t lateness,	
								bool realTimeEvent = false);
		
												
	private:
		BMediaAddOn		*fAddOn;
		MixerCore		*fCore;
		BParameterWeb	*fWeb; // local pointer to parameterweb
		BBufferGroup	*fBufferGroup;
		bigtime_t 		fDownstreamLatency;
		bigtime_t		fInternalLatency;
		bool			fDisableStop;
		media_format	fDefaultFormat;
};		
			
#endif
