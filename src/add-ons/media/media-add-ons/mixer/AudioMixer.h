// AudioMixer.h
/*

	By David Shipman, 2002

*/

#ifndef _AUDIOMIXER_H
#define _AUDIOMIXER_H

// forward declarations

class mixer_input;

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
					
	// AudioMixer support
					
		bool				IsValidDest( media_destination dest);
		bool				IsValidSource( media_source source);
	
		
		BParameterWeb	*	BuildParameterWeb(); // used to create the initial 'master' web
		void				MakeWebForInput(char *name, media_format format);
		
		void 				AllocateBuffers();
		status_t			FillMixBuffer(void *outbuffer, size_t size);
		
		void				SendNewBuffer(bigtime_t event_time);
		void				HandleInputBuffer(BBuffer *buffer);
					
	// BMediaNode methods

		BMediaAddOn		*	AddOn(int32*) const;
	//	void 			SetRunMode(run_mode);
	//	void Preroll();
	//	void SetTimeSource(BTimeSource* time_source);
	//	status_t RequestCompleted(const media_request_info & info);
	
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
	
		virtual	void 		NodeRegistered();
	
		void				HandleEvent( const media_timed_event *event, 
								bigtime_t lateness,	
								bool realTimeEvent = false);
		
		// handle mixing in separate thread
		// not implemented (yet)
		
		static	int32		_mix_thread_(void *data);
		int32				MixThread();
															
	private:
	
		thread_id			fThread; // for launching mixthread

		media_output 		fOutput; // single output (temp)
		float			*	fMasterGainScale; // array used for scaling output - _not_ for display!
		float			*	fMasterGainDisplay; // array of volume values for display purposes
		bigtime_t			fMasterGainDisplayLastChange;
		
		int					fMasterMuteValue;
		bigtime_t			fMasterMuteValueLastChange;
		

		BMediaAddOn		*	fAddOn;
		BParameterWeb	*	fWeb; // local pointer to parameterweb

		bigtime_t 			fLatency, fInternalLatency; // latency (downstream and internal)
		bigtime_t			fStartTime; // time node started
		uint64 				fFramesSent; // audio frames sent
		bool				fOutputEnabled;
		
		media_format 		fPrefInputFormat, fPrefOutputFormat;
		BBufferGroup	*	fBufferGroup;
		
		BList				fMixerInputs;
		
		int					fNextFreeID;
		
};		
			
#endif
