/***********************************************************************
 * AUTHOR: Marcus Overhagen, Jérôme Duval
 *   FILE: SoundPlayer.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeSource.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <math.h>
#include <string.h>

#include "debug.h"
#include "SoundPlayNode.h"
#include "SoundPlayer.h"

// Flags used internally in BSoundPlayer
enum {
	F_HAS_DATA			(1 << 0),
	F_NODES_CONNECTED	(1 << 1),
};

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
	media_multi_audio_format fmt = media_multi_audio_format::wildcard;
	fmt.frame_rate = 44100.0f;
//	fmt.channel_count = 2;
	fmt.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fmt.byte_order = B_MEDIA_HOST_ENDIAN;
	//fmt.buffer_size = 4096;
	Init(NULL, &fmt, name, NULL, PlayBuffer, Notifier, cookie);
}

BSoundPlayer::BSoundPlayer(const media_raw_audio_format * format, 
						   const char * name,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();
	media_multi_audio_format fmt = media_multi_audio_format::wildcard;
	fmt = *format;
	Init(NULL, &fmt, name, NULL, PlayBuffer, Notifier, cookie);
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
	Init(&toNode, format, name, input, PlayBuffer, Notifier, cookie);
}

/* virtual */
BSoundPlayer::~BSoundPlayer()
{
	CALLED();
	
	F_NODES_CONNECTED
	
	
	
	if (fPlayerNode) {
		BMediaRoster *roster = BMediaRoster::Roster();
		if (!roster) {
			TRACE("BSoundPlayer::~BSoundPlayer: Couldn't get BMediaRoster\n");
		} else {
			status_t err;

			// Ordinarily we'd stop *all* of the nodes in the chain at this point.  However,
			// one of the nodes is the System Mixer, and stopping the Mixer is a Bad Idea (tm).
			// So, we just disconnect from it, and release our references to the nodes that
			// we're using.  We *are* supposed to do that even for global nodes like the Mixer.
			Stop(true, false);
			
			err = roster->Disconnect(fMediaInput.node.node, fMediaInput.source, 
				fMediaOutput.node.node, fMediaOutput.destination);
			if (err) {
				fprintf(stderr, "* Error disconnecting nodes:  %ld (%s)\n", err, strerror(err));
			}
			fVolumeSlider = NULL;
			
			err = roster->ReleaseNode(fMediaInput.node);
			if (err) {
				fprintf(stderr, "* Error releasing input node:  %ld (%s)\n", err, strerror(err));
			}
			
			err = roster->ReleaseNode(fMediaOutput.node);
			if (err) {
				fprintf(stderr, "* Error releasing output node:  %ld (%s)\n", err, strerror(err));
			}
			fPlayerNode = NULL;
		}
	}
	delete [] _m_buf;
	
	delete fParameterWeb;
}


status_t
BSoundPlayer::InitCheck()
{
	CALLED();
	return fInitStatus;
}


media_raw_audio_format
BSoundPlayer::Format() const
{
	CALLED();

	media_raw_audio_format temp = media_raw_audio_format::wildcard;
	
	if (fPlayerNode) {
		media_multi_audio_format fmt;
		fmt = fPlayerNode->Format();
		memcpy(&temp,&fmt,sizeof(temp));
	}

	return temp;
}


status_t
BSoundPlayer::Start()
{
	CALLED();
	
	if (!fPlayerNode)
		return B_ERROR;

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Start: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}
	
	// make sure we give the producer enough time to run buffers through
	// the node chain, otherwise it'll start up already late
	bigtime_t latency = 0;
	status_t err = roster->GetLatencyFor(fPlayerNode->Node(), &latency);

	err = roster->StartNode(fPlayerNode->Node(), fPlayerNode->TimeSource()->Now() + latency + 5000);
	
	return err;
}


void
BSoundPlayer::Stop(bool block,
				   bool flush)
{
	CALLED();

	if (!fPlayerNode)
		return;
	
	// XXX flush is ignored
		
	TRACE("BSoundPlayer::Stop: block %d, flush %d\n", (int)block, (int)flush);
		
	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Stop: Couldn't get BMediaRoster\n");
		return;
	}
	
	roster->StopNode(fPlayerNode->Node(), 0, true);
	
	if (block) {
		// wait until the node is stopped
		int maxtrys;
		for (maxtrys = 250; fPlayerNode->IsPlaying() && maxtrys != 0; maxtrys--)
			snooze(2000);
		
		DEBUG_ONLY(if (maxtrys == 0) printf("BSoundPlayer::Stop: waiting for node stop failed\n"));
		
		// wait until all buffers on the way to the physical output have been played		
		snooze(fPlayerNode->Latency() + 2000);
	}
}

BSoundPlayer::BufferPlayerFunc
BSoundPlayer::BufferPlayer() const
{
	CALLED();
	return fPlayBufferFunc;
}

void BSoundPlayer::SetBufferPlayer(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format))
{
	CALLED();
	fLocker.Lock();
	fPlayBufferFunc = PlayBuffer;
	fLocker.Unlock();
}

BSoundPlayer::EventNotifierFunc
BSoundPlayer::EventNotifier() const
{
	CALLED();
	return fNotifierFunc;
}

void BSoundPlayer::SetNotifier(void (*Notifier)(void *, sound_player_notification what, ...))
{
	CALLED();
	fLocker.Lock();
	fNotifierFunc = Notifier;
	fLocker.Unlock();
}

void *
BSoundPlayer::Cookie() const
{
	CALLED();
	return fCookie;
}

void
BSoundPlayer::SetCookie(void *cookie)
{
	CALLED();
	fLocker.Lock();
	fCookie = cookie;
	fLocker.Unlock();
}

void BSoundPlayer::SetCallbacks(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
								void (*Notifier)(void *, sound_player_notification what, ...),
								void * cookie)
{
	CALLED();
	fLocker.Lock();
	SetBufferPlayer(PlayBuffer);
	SetNotifier(Notifier);
	SetCookie(cookie);
	fLocker.Unlock();
}


bigtime_t
BSoundPlayer::CurrentTime()
{
	CALLED();
	if (!fPlayerNode)
		return system_time();  // XXX wrong

	return fPlayerNode->TimeSource()->Now(); // XXX wrong
}


bigtime_t
BSoundPlayer::PerformanceTime()
{
	CALLED();
	if (!fPlayerNode)
		return (bigtime_t) B_ERROR;

	return fPlayerNode->TimeSource()->Now();
}


status_t
BSoundPlayer::Preroll()
{
	CALLED();

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Preroll: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}
	
	status_t err = roster->PrerollNode(fMediaOutput.node);
	
	if (err != B_OK) {
		fprintf(stderr, "Error while PrerollNode:  %ld (%s)\n", err, strerror(err));
	}
	
	return err;
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
	
	return pow(10.0, VolumeDB(true) / 20.0);
}


void
BSoundPlayer::SetVolume(float new_volume)
{
	CALLED();
	SetVolumeDB(20.0 * log10(new_volume));
}


float
BSoundPlayer::VolumeDB(bool forcePoll)
{
	CALLED();
	if (!fVolumeSlider)
		return 0.0f;
		
	if (!forcePoll && (system_time() - fLastVolumeUpdate < 500000))
		return fVolume;
	
	int32 count = fVolumeSlider->CountChannels(); 
	float values[count];
	size_t size = count * sizeof(float);
	fVolumeSlider->GetValue(&values, &size, NULL);
	fLastVolumeUpdate = system_time();
	fVolume = values[0];
		
	return values[0];
}


void
BSoundPlayer::SetVolumeDB(float volume_dB)
{
	CALLED();
	if (!fVolumeSlider)
		return;
	
	float min_dB = fVolumeSlider->MinValue();
	float max_dB = fVolumeSlider->MinValue();
	if (volume_dB < min_dB)
		volume_dB = min_dB;
	if (volume_dB > max_dB)
		volume_dB = max_dB;
		
	int count = fVolumeSlider->CountChannels(); 
	float values[count];
	for (int i = 0; i < count; i++)
		values[i] = volume_dB;
	fVolumeSlider->SetValue(values, sizeof(float) * count, 0);

	fVolume = volume_dB;
	fLastVolumeUpdate = system_time();
}


status_t
BSoundPlayer::GetVolumeInfo(media_node *out_node,
							int32 *out_parameter_id,
							float *out_min_dB,
							float *out_max_dB)
{
	CALLED();
	if (!fVolumeSlider)
		return B_NO_INIT;
		
	if (out_node)
		*out_node = fMediaInput.node;
	if (out_parameter_id)
		*out_parameter_id = fVolumeSlider->ID();
	if (out_min_dB)
		*out_min_dB = fVolumeSlider->MinValue();
	if (out_max_dB)
		*out_max_dB = fVolumeSlider->MaxValue();
	return B_OK;
}


bigtime_t
BSoundPlayer::Latency()
{
	CALLED();
	if (fInitStatus != B_OK)
		return B_NO_INIT; 

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Latency: Couldn't get BMediaRoster\n");
		return 0;
	}
	
	bigtime_t latency;
	status_t err = roster->GetLatencyFor(fMediaOutput.node, &latency);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Latency: GetLatencyFor failed %ld (%s)\n", err, strerror(err));
		return 0;
	}

	return latency;
}


/* virtual */ bool
BSoundPlayer::HasData()
{
	CALLED();
	return (atomic_read(&fFlags) & F_HAS_DATA) != 0;
}


void
BSoundPlayer::SetHasData(bool has_data)
{
	CALLED();
	if (has_data)
		atomic_or(&fFlags, F_HAS_DATA);
	else
		atomic_and(&fFlags, ~F_HAS_DATA);
}


/*************************************************************
 * protected BSoundPlayer
 *************************************************************/

//final
void
BSoundPlayer::SetInitError(status_t in_error)
{
	CALLED();
	fInitStatus = in_error;
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
	CALLED();

	ASSERT(fVolumeSlider == NULL);
	
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (!roster)
		return;
		
	if (!fParameterWeb && roster->GetParameterWebFor(fMediaInput.node, &fParameterWeb) < B_OK)
		return;

	int count = fParameterWeb->CountParameters();
	for (int i = 0; i < count; i++) {
		BParameter *parameter = web->ParameterAt(i);
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
			continue;
		if (parameter->ID() >> 16) != fMediaInput.destination.id)
			continue;
		if  (strcmp(parameter->Kind(), B_GAIN) != 0)
			continue;
		fVolumeSlider = (BContinuousParameter *)parameter;
		break;	
	}
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
	CALLED();
	fPlayerNode = NULL;
	_m_sounds = NULL;
	_m_waiting = NULL;
	fPlayBufferFunc = PlayBuffer;
	fNotifierFunc = Notifier;
	fVolume = 0.0f;
	_m_mix_buffer = 0;
	_m_mix_buffer_size = 0;
	fCookie = cookie;
	_m_buf = NULL;
	_m_bufsize = 0;
	fFlags = 0;
	fInitStatus = B_ERROR;
	_m_perfTime = 0;
	fVolumeSlider = NULL;
	fParameterWeb = NULL;
	fLastVolumeUpdate = 0;

	fPlayerNode = 0;

	status_t err; 
	media_node mixerNode;
	media_output _output;
	media_input _input;
	int32 inputCount, outputCount;
	media_format tryFormat;
	media_multi_audio_format fmt;
	media_node timeSource;

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Init: Couldn't get BMediaRoster\n");
		return;
	}

	//connect our producer node either to the 
	//system mixer or to the supplied out node
	if (!node) {
		err = roster->GetAudioMixer(&mixerNode);
		if (err != B_OK) {
			TRACE("BSoundPlayer::Init: Couldn't GetAudioMixer\n");
			goto the_end;
		}
		node = &mixerNode;
	}
	
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
		ERROR("BSoundPlayer: not a 1 or 2 channel audio format\n");
	if (fmt.frame_rate <= 0.0f)
		ERROR("BSoundPlayer: framerate must be > 0\n");

	_m_bufsize = fmt.buffer_size;
	_m_buf = new char[_m_bufsize];
	fPlayerNode = new _SoundPlayNode(name,&fmt,this);
	
	err = roster->RegisterNode(fPlayerNode);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't RegisterNode\n");
		goto the_end;
	}
	
	// set the producer's time source to be the "default" time source, which
	// the Mixer uses too.
	
	err = roster->GetTimeSource(&timeSource);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't GetTimeSource\n");
		goto the_end;
	}
	err = roster->SetTimeSourceFor(fPlayerNode->Node().node, timeSource.node);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't SetTimeSourceFor\n");
		goto the_end;
	}
	
	if (!input) {
		err = roster->GetFreeInputsFor(*node, &_input, 1, 
			&inputCount, B_MEDIA_RAW_AUDIO);
		if (err != B_OK) {
			TRACE("BSoundPlayer::Init: Couldn't GetFreeInputsFor\n");
			goto the_end;
		}
	} else {
		_input = *input;
	}
	err = roster->GetFreeOutputsFor(fPlayerNode->Node(), &_output, 1, &outputCount, B_MEDIA_RAW_AUDIO);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't GetFreeOutputsFor\n");
		goto the_end;
	}

	// Set an appropriate run mode for the producer
	err = roster->SetRunModeNode(fPlayerNode->Node(), BMediaNode::B_INCREASE_LATENCY);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't SetRunModeNode\n");
		goto the_end;
	}
		
	//tryFormat.type = B_MEDIA_RAW_AUDIO;
	//tryformat.fileAudioOutput.format;
	tryFormat = _output.format;
	err = roster->Connect(_output.source, _input.destination, &tryFormat, &fMediaOutput, &fMediaInput);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't Connect\n");
		goto the_end;
	}
	
	get_volume_slider();
	
	
	printf("BSoundPlayer node %ld has timesource %ld\n", fPlayerNode->Node().node, fPlayerNode->TimeSource()->Node().node);
	
the_end:
	TRACE("BSoundPlayer::Init: %s\n", strerror(err));
	SetInitError(err);
}


/* virtual */ void
BSoundPlayer::Notify(sound_player_notification what,
					 ...)
{
	CALLED();
	if (fLocker.Lock()) {
		if (fNotifierFunc)
			(*fNotifierFunc)(fCookie, what);
		fLocker.Unlock();
	}
}


/* virtual */ void
BSoundPlayer::PlayBuffer(void *buffer,
						 size_t size,
						 const media_raw_audio_format &format)
{
	if (fLocker.Lock()) {
		if (fPlayBufferFunc)
			(*fPlayBufferFunc)(fCookie, buffer, size, format);
		fLocker.Unlock();
	}
}
