/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SoundPlayer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeSource.h>
#include <SoundPlayer.h>
#include <MediaRoster.h>
#include <math.h>

#include "debug.h"
#include "SoundPlayNode.h"

/* this is the normal volume DB range of 16 bit integer */
const float minDB = -96;
const float maxDB = 0;

/*************************************************************
 * public sound_error
 *************************************************************/

//final
sound_error::sound_error(const char *str)
{
	CALLED();
	m_str_const = str;
}

//final
const char *
sound_error::what() const
{
	CALLED();
	return m_str_const;
}

/*************************************************************
 * public BSoundPlayer
 *************************************************************/

BSoundPlayer::BSoundPlayer(const char * name,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();

	Init(NULL,&media_multi_audio_format::wildcard,name,NULL,PlayBuffer,Notifier,cookie);
}

BSoundPlayer::BSoundPlayer(const media_raw_audio_format * format, 
						   const char * name,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();
	media_multi_audio_format fmt = media_multi_audio_format::wildcard;
	memcpy(&fmt,format,sizeof(*format));
	Init(NULL,&fmt,name,NULL,PlayBuffer,Notifier,cookie);
}

BSoundPlayer::BSoundPlayer(const media_node & toNode,
						   const media_multi_audio_format * format,
						   const char * name,
						   const media_input * input,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();
	if (toNode.kind & B_BUFFER_CONSUMER == 0)
		debugger("BSoundPlayer: toNode must have B_BUFFER_CONSUMER kind!\n");
	Init(&toNode,format,name,input,PlayBuffer,Notifier,cookie);
}

/* virtual */
BSoundPlayer::~BSoundPlayer()
{
	CALLED();
	if (_m_node)
		_m_node->Release();
	delete _m_node;
	delete [] _m_buf;
}


status_t
BSoundPlayer::InitCheck()
{
	CALLED();
	return _m_init_err;
}


media_raw_audio_format
BSoundPlayer::Format() const
{
	CALLED();

	media_raw_audio_format temp = media_raw_audio_format::wildcard;
	
	if (_m_node) {
		media_multi_audio_format fmt;
		fmt = _m_node->Format();
		memcpy(&temp,&fmt,sizeof(temp));
	}

	return temp;
}


status_t
BSoundPlayer::Start()
{
	CALLED();
	
	if (!_m_node)
		return B_ERROR;
	
	_m_node->Start();
	return B_OK;
}


void
BSoundPlayer::Stop(bool block,
				   bool flush)
{
	CALLED();

	if (!_m_node)
		return;

	_m_node->Stop();
}

BSoundPlayer::BufferPlayerFunc
BSoundPlayer::BufferPlayer() const
{
	CALLED();
	return _PlayBuffer;
}

void BSoundPlayer::SetBufferPlayer(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format))
{
	CALLED();
	_m_lock.Lock();
	_PlayBuffer = PlayBuffer;
	_m_lock.Unlock();
}

BSoundPlayer::EventNotifierFunc
BSoundPlayer::EventNotifier() const
{
	CALLED();
	return _Notifier;
}

void BSoundPlayer::SetNotifier(void (*Notifier)(void *, sound_player_notification what, ...))
{
	CALLED();
	_m_lock.Lock();
	_Notifier = Notifier;
	_m_lock.Unlock();
}

void *
BSoundPlayer::Cookie() const
{
	CALLED();
	return _m_cookie;
}

void
BSoundPlayer::SetCookie(void *cookie)
{
	CALLED();
	_m_lock.Lock();
	_m_cookie = cookie;
	_m_lock.Unlock();
}

void BSoundPlayer::SetCallbacks(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
								void (*Notifier)(void *, sound_player_notification what, ...),
								void * cookie)
{
	CALLED();
	_m_lock.Lock();
	SetBufferPlayer(PlayBuffer);
	SetNotifier(Notifier);
	SetCookie(cookie);
	_m_lock.Unlock();
}


bigtime_t
BSoundPlayer::CurrentTime()
{
	CALLED();
	if (!_m_node)
		return system_time();
#if 0 /* we don't have a media roster, or real media nodes yet */
	return _m_node->TimeSource()->Now(); /* either this one is wrong */
#endif
	return system_time();
}


bigtime_t
BSoundPlayer::PerformanceTime()
{
	CALLED();
	if (!_m_node)
		return (bigtime_t) B_ERROR;
#if 0 /* we don't have a media roster, or real media nodes yet */
	return _m_node->TimeSource()->Now(); /* or this one is wrong */
#endif
	return system_time();
}


status_t
BSoundPlayer::Preroll()
{
	UNIMPLEMENTED();

	return B_OK;
}


BSoundPlayer::play_id
BSoundPlayer::StartPlaying(BSound *sound,
						   bigtime_t at_time)
{
	UNIMPLEMENTED();
	return 1;
}
 

BSoundPlayer::play_id
BSoundPlayer::StartPlaying(BSound *sound,
						   bigtime_t at_time,
						   float with_volume)
{
	UNIMPLEMENTED();
	return 1;
}


status_t
BSoundPlayer::SetSoundVolume(play_id sound,
							 float new_volume)
{
	UNIMPLEMENTED();

	return B_OK;
}


bool
BSoundPlayer::IsPlaying(play_id id)
{
	UNIMPLEMENTED();

	return true;
}


status_t
BSoundPlayer::StopPlaying(play_id id)
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BSoundPlayer::WaitForSound(play_id id)
{
	UNIMPLEMENTED();

	return B_OK;
}


float
BSoundPlayer::Volume()
{
	CALLED();
	return _m_volume;
}


void
BSoundPlayer::SetVolume(float new_volume)
{
	CALLED();
	_m_lock.Lock();
	if (new_volume >= 0.0f)
		_m_volume = new_volume;
	_m_lock.Unlock();
}


float
BSoundPlayer::VolumeDB(bool forcePoll)
{
	CALLED();
	return 20.0f * log10(_m_volume);
}


void
BSoundPlayer::SetVolumeDB(float volume_dB)
{
	CALLED();
	_m_lock.Lock();
	_m_volume = pow(10.0f,volume_dB / 20.0f);
	_m_lock.Unlock();
}


status_t
BSoundPlayer::GetVolumeInfo(media_node *out_node,
							int32 *out_parameter,
							float *out_min_dB,
							float *out_max_dB)
{
	BROKEN();
	
	*out_node = m_output.node;
	*out_parameter = -1; /* is the parameter ID for the volume control */
	*out_min_dB = minDB;
	*out_max_dB = maxDB;

	return B_OK;
}


bigtime_t
BSoundPlayer::Latency()
{
	BROKEN();
	return 50000;
}


/* virtual */ bool
BSoundPlayer::HasData()
{
	CALLED();

	return _m_has_data != 0;
}


void
BSoundPlayer::SetHasData(bool has_data)
{
	CALLED();
	_m_lock.Lock();
	_m_has_data = has_data ? 1 : 0;
	_m_lock.Unlock();
}


/*************************************************************
 * protected BSoundPlayer
 *************************************************************/

//final
void
BSoundPlayer::SetInitError(status_t in_error)
{
	CALLED();
	_m_init_err = in_error;
}


/*************************************************************
 * private BSoundPlayer
 *************************************************************/

status_t BSoundPlayer::_Reserved_SoundPlayer_0(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_1(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_2(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_3(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_4(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_5(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_6(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_7(void *, ...) { return B_ERROR; }


void
BSoundPlayer::NotifySoundDone(play_id sound,
							  bool got_to_play)
{
	UNIMPLEMENTED();
}


void
BSoundPlayer::get_volume_slider()
{
	UNIMPLEMENTED();
}

void 
BSoundPlayer::Init(
					const media_node * node,
					const media_multi_audio_format * format, 
					const char * name,
					const media_input * input,
					void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
					void (*Notifier)(void *, sound_player_notification what, ...),
					void * cookie)
{
	BROKEN();
	_m_node = NULL;
	_m_sounds = NULL;
	_m_waiting = NULL;
	_PlayBuffer = PlayBuffer;
	_Notifier = Notifier;
	//_m_lock;
	_m_volume = 0.0f;
	//m_input;
	//m_output;
	_m_mix_buffer = 0;
	_m_mix_buffer_size = 0;
	_m_cookie = cookie;
	_m_buf = NULL;
	_m_bufsize = 0;
	_m_has_data = 0;
	_m_init_err = B_ERROR;
	_m_perfTime = 0;
	_m_volumeSlider = NULL;
	_m_gotVolume = 0;

	_m_node = 0;

#if 0 /* we don't have a media roster, or real media nodes yet */
	status_t status; 
	BMediaRoster *roster;
	media_node outnode;
	roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Init: Couldn't get BMediaRoster\n");
		return;
	}
	
	//connect our producer node either to the 
	//system mixer or to the supplied out node
	if (!node) {
		status = roster->GetAudioMixer(&outnode);
		if (status != B_OK) {
			TRACE("BSoundPlayer::Init: Couldn't GetAudioMixer\n");
			SetInitError(status);
			return;
		}
		node = &outnode;
	}
#endif

	media_multi_audio_format fmt;
	memcpy(&fmt,format,sizeof(fmt));
	
	if (fmt.frame_rate == media_multi_audio_format::wildcard.frame_rate)
		fmt.frame_rate = 44100.0f;
	if (fmt.channel_count == media_multi_audio_format::wildcard.channel_count)
		fmt.channel_count = 2;
	if (fmt.format == media_multi_audio_format::wildcard.format)
		fmt.format = media_raw_audio_format::B_AUDIO_FLOAT;
	if (fmt.byte_order == media_multi_audio_format::wildcard.byte_order)
		fmt.byte_order = B_MEDIA_HOST_ENDIAN;
	if (fmt.buffer_size == media_multi_audio_format::wildcard.buffer_size)
		fmt.buffer_size = 4096;
		
	if (fmt.channel_count != 1 && fmt.channel_count != 2)
		debugger("BSoundPlayer: not a 1 or 2 channel audio format\n");
	if (fmt.frame_rate <= 0.0f)
		debugger("BSoundPlayer: framerate must be > 0\n");

	_m_bufsize = fmt.buffer_size;
	_m_buf = new char[_m_bufsize];
	_m_node = new _SoundPlayNode(name,&fmt,this);


/*
	m_input = ;
	m_output = ;

	
tryFormat = fileAudioOutput.format; 
   err = roster->Connect(fileAudioOutput.source, audioInput.destination, 
               &tryFormat, &m_output, &m_input); 


 err = roster->GetStartLatencyFor(timeSourceNode, &startTime); 
   startTime += b_timesource->PerformanceTimeFor(BTimeSource::RealTime() 
               + 1000000 / 50); 
    
   err = roster->StartNode(mediaFileNode, startTime); 
   err = roster->StartNode(codecNode, startTime); 
   err = roster->StartNode(videoNode, startTime); 

*/	
	
	SetInitError(B_OK);
}

/* virtual */ void
BSoundPlayer::Notify(sound_player_notification what,
					 ...)
{
	CALLED();
	_m_lock.Lock();
	if (_Notifier)
		(*_Notifier)(_m_cookie,what);
	else {
	}
	_m_lock.Unlock();
}


/* virtual */ void
BSoundPlayer::PlayBuffer(void *buffer,
						 size_t size,
						 const media_raw_audio_format &format)
{
//	CALLED();
	_m_lock.Lock();
	if (_PlayBuffer)
		(*_PlayBuffer)(_m_cookie,buffer,size,format);
	else {
	}
	_m_lock.Unlock();
}


