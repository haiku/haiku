/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DVB_MEDIA_NODE_H
#define __DVB_MEDIA_NODE_H


#include <OS.h>
#include <media/BufferProducer.h>
#include <media/Controllable.h>
#include <media/MediaDefs.h>
#include <media/MediaEventLooper.h>
#include <media/MediaNode.h>
#include <support/Locker.h>

#include "TransportStreamDemux.h"
#include "MediaStreamDecoder.h"
#include "DVBCard.h"
#include "StringList.h"

class BDiscreteParameter;
class BParameterGroup;
class PacketQueue;
class DVBCard;


class DVBMediaNode : public virtual BBufferProducer,
	public virtual BControllable, public virtual BMediaEventLooper {
public:
						DVBMediaNode(BMediaAddOn *addon,
								const char *name, int32 internal_id, DVBCard *card);
virtual					~DVBMediaNode();

virtual	status_t		InitCheck() const { return fInitStatus; }

/* BMediaNode */
public:
virtual	BMediaAddOn	*AddOn(int32 * internal_id) const;
virtual	status_t 	HandleMessage(int32 message, const void *data,
							size_t size);
protected:	
virtual	void 		Preroll();
virtual void		SetTimeSource(BTimeSource * time_source);
virtual void		SetRunMode(run_mode mode);

/* BMediaEventLooper */
protected:
virtual	void 		NodeRegistered();
virtual void		Stop(bigtime_t performance_time, bool immediate);

virtual void		HandleEvent(const media_timed_event *event,
							bigtime_t lateness, bool realTimeEvent = false);

/* BBufferProducer */									
protected:
virtual	status_t	FormatSuggestionRequested(media_type type, int32 quality,
							media_format * format);
virtual	status_t 	FormatProposal(const media_source &source,
							media_format *format);
virtual	status_t	FormatChangeRequested(const media_source &source,
							const media_destination &destination,
							media_format *io_format, int32 *_deprecated_);
virtual	status_t 	GetNextOutput(int32 * cookie, media_output * out_output);
virtual	status_t	DisposeOutputCookie(int32 cookie);
virtual	status_t	SetBufferGroup(const media_source &for_source,
							BBufferGroup * group);
virtual	status_t 	VideoClippingChanged(const media_source &for_source,
							int16 num_shorts, int16 *clip_data,
							const media_video_display_info &display,
							int32 * _deprecated_);
virtual	status_t	GetLatency(bigtime_t * out_latency);
virtual	status_t	PrepareToConnect(const media_source &what,
							const media_destination &where,
							media_format *format,
							media_source *out_source, char *out_name);
virtual	void		Connect(status_t error, const media_source &source,
							const media_destination &destination,
							const media_format & format, char *io_name);
virtual	void 		Disconnect(const media_source & what,
							const media_destination & where);
virtual	void 		LateNoticeReceived(const media_source & what,
							bigtime_t how_much, bigtime_t performance_time);
virtual	void 		EnableOutput(const media_source & what, bool enabled,
							int32 * _deprecated_);
virtual	status_t	SetPlayRate(int32 numer,int32 denom);
virtual	void 		AdditionalBufferRequested(const media_source & source,
							media_buffer_id prev_buffer, bigtime_t prev_time,
							const media_seek_tag * prev_tag);
virtual	void		LatencyChanged(const media_source & source,
							const media_destination & destination,
							bigtime_t new_latency, uint32 flags);

/* BControllable */									
protected:
virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change,
							void *value, size_t *size);
virtual void		SetParameterValue(int32 id, bigtime_t when,
							const void *value, size_t size);

/* state */
private:
		void				HandleStart(bigtime_t performance_time);
		void				HandleStop();
		void				HandleTimeWarp(bigtime_t performance_time);
		void				HandleSeek(bigtime_t performance_time);

		void				InitDefaultFormats();
		void				SpecializeFormatRawVideo(media_raw_video_format *format);
		void				SpecializeFormatRawAudio(media_multi_audio_format *format);
		void				SpecializeFormatEncVideo(media_encoded_video_format *format);
		void				SpecializeFormatEncAudio(media_encoded_audio_format *format);
		void				SpecializeFormatTS(media_multistream_format *format);

		bool				VerifyFormatRawVideo(const media_raw_video_format &format);
		bool				VerifyFormatRawAudio(const media_multi_audio_format &format);
		
		status_t			GetStreamFormat(PacketQueue *queue, media_format *format);
		
		status_t			SetOutput(const media_source &source, const media_destination &destination, const media_format &format);
		status_t			ResetOutput(const media_source &source);
		const char *		SourceDefaultName(const media_source &source);
		
		BParameterWeb *		CreateParameterWeb();
		void				SetAboutInfo(BParameterGroup *about);

		
		void				RefreshParameterWeb();
		
		void				LoadSettings();
		
		void				AddStateItems(BDiscreteParameter *param);
		void				AddRegionItems(BDiscreteParameter *param);
		void				AddChannelItems(BDiscreteParameter *param);
		void				AddAudioItems(BDiscreteParameter *param);
		
		void				RefreshStateList();
		void				RefreshRegionList();
		void				RefreshChannelList();
		void				RefreshAudioList();

		status_t			StartCapture();
		status_t			StopCapture();
		status_t			StartCaptureThreads();
		status_t			StopCaptureThreads();		
		status_t			Tune();
		
		status_t			ExtractTuningParams(const char *description, int audio_pid_index,
												dvb_tuning_parameters_t *param, 
												int *video_pid, int *audio_pid, int *pcr_pid);

		static int32		_card_reader_thread_(void *arg);
		static int32		_mpeg_demux_thread_(void *arg);
		static int32		_enc_audio_thread_(void *arg);
		static int32		_enc_video_thread_(void *arg);
		static int32		_mpeg_ts_thread_(void *arg);
		static int32		_raw_audio_thread_(void *arg);
		static int32		_raw_video_thread_(void *arg);

		void				card_reader_thread();
		void				mpeg_demux_thread();
		void				enc_audio_thread();
		void				enc_video_thread();
		void				mpeg_ts_thread();
		void				raw_audio_thread();
		void				raw_video_thread();

		status_t			GetNextVideoChunk(const void **chunkData, size_t *chunkLen, media_header *mh);
		status_t			GetNextAudioChunk(const void **chunkData, size_t *chunkLen, media_header *mh);

		static status_t		_GetNextVideoChunk(const void **chunkData, size_t *chunkLen, media_header *mh, void *cookie);
		static status_t		_GetNextAudioChunk(const void **chunkData, size_t *chunkLen, media_header *mh, void *cookie);

		status_t			fInitStatus;
		
		int32				fInternalID;
		BMediaAddOn			*fAddOn;
		
		bool				fStopDisabled;

		bool				fOutputEnabledRawVideo;
		bool				fOutputEnabledRawAudio;
		bool				fOutputEnabledEncVideo;
		bool				fOutputEnabledEncAudio;
		bool				fOutputEnabledTS;

		media_output		fOutputRawVideo;
		media_output		fOutputRawAudio;
		media_output		fOutputEncVideo;
		media_output		fOutputEncAudio;
		media_output		fOutputTS;
		
		media_format		fDefaultFormatRawVideo;
		media_format		fDefaultFormatRawAudio;
		media_format		fDefaultFormatEncVideo;
		media_format		fDefaultFormatEncAudio;
		media_format		fDefaultFormatTS;

		media_format		fRequiredFormatRawVideo;
		media_format		fRequiredFormatRawAudio;
		media_format		fRequiredFormatEncVideo;
		media_format		fRequiredFormatEncAudio;
		media_format		fRequiredFormatTS;
		
		PacketQueue *		fCardDataQueue; // holds MPEG TS data read from the card
		PacketQueue *		fRawVideoQueue; // holds encoded PES frames for video decoding
		PacketQueue *		fRawAudioQueue; // holds encoded PES frames for audio decoding
		PacketQueue *		fEncVideoQueue; // holds encoded PES frames for enc video out
		PacketQueue *		fEncAudioQueue; // holds encoded PES frames for enc audio out
		PacketQueue *		fMpegTsQueue; 	// holds encoded PES frames for MPEG TS out
		
		DVBCard *			fCard;
		
		bool				fCaptureThreadsActive;
		
		thread_id			fThreadIdCardReader;
		thread_id			fThreadIdMpegDemux;
		thread_id			fThreadIdRawAudio;
		thread_id			fThreadIdRawVideo;
		thread_id			fThreadIdEncAudio;
		thread_id			fThreadIdEncVideo;
		thread_id			fThreadIdMpegTS;
		
		volatile bool		fTerminateThreads;
		
		TransportStreamDemux * fDemux;
		
		BBufferGroup *		fBufferGroupRawVideo;
		BBufferGroup *		fBufferGroupRawAudio;
		
		BParameterWeb *		fWeb;
		
	dvb_tuning_parameters_t fTuningParam;
	dvb_type_t				fInterfaceType;
	
		int					fAudioPid;
		int					fVideoPid;
		int					fPcrPid;
		
		bool				fTuningSuccess;
		bool				fCaptureActive;
		
		sem_id				fVideoDelaySem;
		sem_id				fAudioDelaySem;
		
		int					fSelectedState;
		int					fSelectedRegion;
		int					fSelectedChannel;
		int					fSelectedAudio;

		StringList *		fStateList;
		StringList *		fRegionList;
		StringList *		fChannelList;
		StringList *		fAudioList;
		
		MediaStreamDecoder *fVideoDecoder;
		MediaStreamDecoder *fAudioDecoder;

		Packet *			fCurrentVideoPacket;
		Packet *			fCurrentAudioPacket;

		BLocker lock;
		
		// used only for debugging:
		int					fVideoFile;
		int					fAudioFile;
		int					fRawAudioFile;
};

#endif	// __DVB_MEDIA_NODE_H
