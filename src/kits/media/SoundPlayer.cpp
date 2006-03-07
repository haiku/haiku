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

#define atomic_read(a)	atomic_or(a, 0)

// Flags used internally in BSoundPlayer
enum {
	F_NODES_CONNECTED	= (1 << 0),
	F_HAS_DATA			= (1 << 1),
	F_IS_STARTED		= (1 << 2),
	F_MUST_RELEASE_MIXER = (1 << 3),
};


/*************************************************************
 * public BSoundPlayer
 *************************************************************/


BSoundPlayer::BSoundPlayer(const char * name,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();

	TRACE("BSoundPlayer::BSoundPlayer: default constructor used\n");

	media_multi_audio_format fmt = media_multi_audio_format::wildcard;

	Init(NULL, &fmt, name, NULL, PlayBuffer, Notifier, cookie);
}


BSoundPlayer::BSoundPlayer(const media_raw_audio_format * format, 
						   const char * name,
						   void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
						   void (*Notifier)(void *, sound_player_notification what, ...),
						   void * cookie)
{
	CALLED();

	TRACE("BSoundPlayer::BSoundPlayer: raw audio format constructor used\n");

	media_multi_audio_format fmt = media_multi_audio_format::wildcard;
	*(media_raw_audio_format *)&fmt = *format;

#if DEBUG > 0
	char buf[100];
	media_format tmp; tmp.type = B_MEDIA_RAW_AUDIO; tmp.u.raw_audio = fmt;
	string_for_format(tmp, buf, sizeof(buf));
	TRACE("BSoundPlayer::BSoundPlayer: format %s\n", buf);
#endif

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
	
	TRACE("BSoundPlayer::BSoundPlayer: multi audio format constructor used\n");
	
	if (toNode.kind & B_BUFFER_CONSUMER == 0)
		debugger("BSoundPlayer: toNode must have B_BUFFER_CONSUMER kind!\n");

#if DEBUG > 0
	char buf[100];
	media_format tmp; tmp.type = B_MEDIA_RAW_AUDIO; tmp.u.raw_audio = *format;
	string_for_format(tmp, buf, sizeof(buf));
	TRACE("BSoundPlayer::BSoundPlayer: format %s\n", buf);
#endif

	Init(&toNode, format, name, input, PlayBuffer, Notifier, cookie);
}


/*************************************************************
 * private BSoundPlayer
 *************************************************************/


void 
BSoundPlayer::Init(	const media_node * node,
					const media_multi_audio_format * format, 
					const char * name,
					const media_input * input,
					void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
					void (*Notifier)(void *, sound_player_notification what, ...),
					void * cookie)
{
	CALLED();
	_m_sounds = NULL;	// unused
	_m_waiting = NULL;	// unused

	fPlayerNode = NULL;
	fPlayBufferFunc = PlayBuffer;
	fNotifierFunc = Notifier;
	fVolumeDB = 0.0f;
	fCookie = cookie;
	fFlags = 0;
	fInitStatus = B_ERROR;
	fParameterWeb = NULL;
	fVolumeSlider = NULL;
	fLastVolumeUpdate = 0;

	status_t 		err; 
	media_node		timeSource;
	media_node		inputNode;
	media_output	_output;
	media_input		_input;
	int32 			inputCount;
	int32			outputCount;
	media_format 	tryFormat;

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Init: Couldn't get BMediaRoster\n");
		return;
	}

	// The inputNode that our player node will be
	// connected with is either supplied by the user
	// or the system audio mixer
	if (node) {
		inputNode = *node;
	} else {
		err = roster->GetAudioMixer(&inputNode);
		if (err != B_OK) {
			TRACE("BSoundPlayer::Init: Couldn't GetAudioMixer\n");
			goto the_end;
		}
		fFlags |= F_MUST_RELEASE_MIXER;
	}

	// Create the player node and register it
	fPlayerNode = new _SoundPlayNode(name, this);
	err = roster->RegisterNode(fPlayerNode);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't RegisterNode\n");
		goto the_end;
	}
	
	// set the producer's time source to be the "default" time source,
	// which the system audio mixer uses too.
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
	
	// find a free media_input
	if (!input) {
		err = roster->GetFreeInputsFor(inputNode, &_input, 1, &inputCount, B_MEDIA_RAW_AUDIO);
		if (err != B_OK) {
			TRACE("BSoundPlayer::Init: Couldn't GetFreeInputsFor\n");
			goto the_end;
		}
		if (inputCount < 1) {
			TRACE("BSoundPlayer::Init: Couldn't find a free input\n");
			err = B_ERROR;
			goto the_end;
		}
	} else {
		_input = *input;
	}

	// find a free media_output
	err = roster->GetFreeOutputsFor(fPlayerNode->Node(), &_output, 1, &outputCount, B_MEDIA_RAW_AUDIO);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't GetFreeOutputsFor\n");
		goto the_end;
	}
	if (outputCount < 1) {
		TRACE("BSoundPlayer::Init: Couldn't find a free output\n");
		err = B_ERROR;
		goto the_end;
	}

	// Set an appropriate run mode for the producer
	err = roster->SetRunModeNode(fPlayerNode->Node(), BMediaNode::B_INCREASE_LATENCY);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't SetRunModeNode\n");
		goto the_end;
	}

	// setup our requested format (can still have many wildcards)
	tryFormat.type = B_MEDIA_RAW_AUDIO;
	tryFormat.u.raw_audio = *format;
	
#if DEBUG > 0
	char buf[100];
	string_for_format(tryFormat, buf, sizeof(buf));
	TRACE("BSoundPlayer::Init: trying to connect with format %s\n", buf);
#endif

	// and connect the nodes
	err = roster->Connect(_output.source, _input.destination, &tryFormat, &fMediaOutput, &fMediaInput);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Init: Couldn't Connect\n");
		goto the_end;
	}

	fFlags |= F_NODES_CONNECTED;
	
	get_volume_slider();
	
	TRACE("BSoundPlayer node %ld has timesource %ld\n", fPlayerNode->Node().node, fPlayerNode->TimeSource()->Node().node);

the_end:
	TRACE("BSoundPlayer::Init: %s\n", strerror(err));
	SetInitError(err);
}


/*************************************************************
 * public BSoundPlayer
 *************************************************************/


/* virtual */
BSoundPlayer::~BSoundPlayer()
{
	CALLED();

	if (fFlags & F_IS_STARTED) {
		Stop(true, false); // block, but don't flush
	}

	status_t err;
	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::~BSoundPlayer: Couldn't get BMediaRoster\n");
		goto cleanup;
	}

	if (fFlags & F_NODES_CONNECTED) {
		// Ordinarily we'd stop *all* of the nodes in the chain before disconnecting. However,
		// our node is already stopped, and we can't stop the System Mixer.
		// So, we just disconnect from it, and release our references to the nodes that
		// we're using.  We *are* supposed to do that even for global nodes like the Mixer.
		err = roster->Disconnect(fMediaOutput, fMediaInput);
#if DEBUG >0	
		if (err) {
			TRACE("BSoundPlayer::~BSoundPlayer: Error disconnecting nodes:  %ld (%s)\n", err, strerror(err));
		}
#endif
	}
	
	if (fFlags & F_MUST_RELEASE_MIXER) {
		// Release the mixer as it was acquired
		// through BMediaRoster::GetAudioMixer()
		err = roster->ReleaseNode(fMediaInput.node);
#if DEBUG >0	
		if (err) {
			TRACE("BSoundPlayer::~BSoundPlayer: Error releasing input node:  %ld (%s)\n", err, strerror(err));
		}
#endif
	}

cleanup:
	
	// Dispose of the player node
	if (fPlayerNode) {
		// We do not call BMediaRoster::ReleaseNode(), since
		// the player was created by using "new". We could
		// call BMediaRoster::UnregisterNode(), but this is
		// supposed to be done by BMediaNode destructor automatically
		delete fPlayerNode;
	}

	delete fParameterWeb;
	// do not delete fVolumeSlider, it belonged to the parameter web
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

	if ((fFlags & F_NODES_CONNECTED) == 0)
		return media_raw_audio_format::wildcard;

	return fPlayerNode->Format();
}


status_t
BSoundPlayer::Start()
{
	CALLED();

	if ((fFlags & F_NODES_CONNECTED) == 0)
		return B_NO_INIT;
	
	if (fFlags & F_IS_STARTED)
		return B_OK;

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Start: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}

	// Add latency and a few ms to the nodes current time to	
	// make sure that we give the producer enough time to run
	// buffers through the node chain, otherwise it'll start
	// up already late
	
	status_t err = roster->StartNode(fPlayerNode->Node(), fPlayerNode->TimeSource()->Now() + Latency() + 5000);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Start: StartNode failed, %ld", err);
		return err;
	}
		
	atomic_or(&fFlags, F_IS_STARTED);
	
	return B_OK;
}


void
BSoundPlayer::Stop(bool block,
				   bool flush)
{
	CALLED();

	TRACE("BSoundPlayer::Stop: block %d, flush %d\n", (int)block, (int)flush);

	if ((fFlags & F_NODES_CONNECTED) == 0)
		return;

	// XXX flush is ignored

	if (fFlags & F_IS_STARTED) {

		BMediaRoster *roster = BMediaRoster::Roster();
		if (!roster) {
			TRACE("BSoundPlayer::Stop: Couldn't get BMediaRoster\n");
			return;
		}
		
		roster->StopNode(fPlayerNode->Node(), 0, true);
		
		atomic_and(&fFlags, ~F_IS_STARTED);
	}
	
	if (block) {
		// wait until the node is stopped
		int maxtrys;
		for (maxtrys = 250; fPlayerNode->IsPlaying() && maxtrys != 0; maxtrys--)
			snooze(2000);
		
		DEBUG_ONLY(if (maxtrys == 0) TRACE("BSoundPlayer::Stop: waiting for node stop failed\n"));
		
		// wait until all buffers on the way to the physical output have been played		
		snooze(Latency() + 2000);
	}
}


bigtime_t
BSoundPlayer::Latency()
{
	CALLED();
	
	if ((fFlags & F_NODES_CONNECTED) == 0)
		return 0;

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

	TRACE("BSoundPlayer::Latency: latency is %Ld\n", latency);

	return latency;
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


/* virtual */ bool
BSoundPlayer::HasData()
{
	CALLED();
	return (atomic_read(&fFlags) & F_HAS_DATA) != 0;
}


BSoundPlayer::BufferPlayerFunc
BSoundPlayer::BufferPlayer() const
{
	CALLED();
	return fPlayBufferFunc;
}


void
BSoundPlayer::SetBufferPlayer(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format))
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


void
BSoundPlayer::SetCallbacks(void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
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


/* The BeBook is inaccurate about the meaning of this function.
 * The probably best interpretation is to return the time that
 * has elapsed since playing was started, whichs seems to match
 * " CurrentTime() returns the current media time "
 */
bigtime_t
BSoundPlayer::CurrentTime()
{
	if ((fFlags & F_NODES_CONNECTED) == 0)
		return 0;

	return fPlayerNode->CurrentTime();
}


/* Returns the current performance time of the sound player node
 * being used by the BSoundPlayer. Will return B_ERROR if the
 * BSoundPlayer object hasn't been properly initialized.
 */
bigtime_t
BSoundPlayer::PerformanceTime()
{
	if ((fFlags & F_NODES_CONNECTED) == 0)
		return (bigtime_t) B_ERROR;

	return fPlayerNode->TimeSource()->Now();
}


status_t
BSoundPlayer::Preroll()
{
	CALLED();

	if ((fFlags & F_NODES_CONNECTED) == 0)
		return B_NO_INIT;

	BMediaRoster *roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Preroll: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}
	
	status_t err = roster->PrerollNode(fMediaOutput.node);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Preroll: Error while PrerollNode:  %ld (%s)\n", err, strerror(err));
		return err;
	}
	
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
		return -94.0f; // silence
		
	if (!forcePoll && (system_time() - fLastVolumeUpdate < 500000))
		return fVolumeDB;
	
	int32 count = fVolumeSlider->CountChannels(); 
	float values[count];
	size_t size = count * sizeof(float);
	fVolumeSlider->GetValue(&values, &size, NULL);
	fLastVolumeUpdate = system_time();
	fVolumeDB = values[0];
		
	return values[0];
}


void
BSoundPlayer::SetVolumeDB(float volume_dB)
{
	CALLED();
	if (!fVolumeSlider)
		return;
	
	float min_dB = fVolumeSlider->MinValue();
	float max_dB = fVolumeSlider->MaxValue();
	if (volume_dB < min_dB)
		volume_dB = min_dB;
	if (volume_dB > max_dB)
		volume_dB = max_dB;

	int count = fVolumeSlider->CountChannels(); 
	float values[count];
	for (int i = 0; i < count; i++)
		values[i] = volume_dB;
	fVolumeSlider->SetValue(values, sizeof(float) * count, 0);

	fVolumeDB = volume_dB;
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



/*************************************************************
 * protected BSoundPlayer
 *************************************************************/


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
	if (!roster) {
		TRACE("BSoundPlayer::get_volume_slider failed to get BMediaRoster");
		return;
	}
		
	if (!fParameterWeb && roster->GetParameterWebFor(fMediaInput.node, &fParameterWeb) < B_OK) {
		TRACE("BSoundPlayer::get_volume_slider couldn't get parameter web");
		return;
	}

	int count = fParameterWeb->CountParameters();
	for (int i = 0; i < count; i++) {
		BParameter *parameter = fParameterWeb->ParameterAt(i);
		if (parameter->Type() != BParameter::B_CONTINUOUS_PARAMETER)
			continue;
		if ((parameter->ID() >> 16) != fMediaInput.destination.id)
			continue;
		if  (strcmp(parameter->Kind(), B_GAIN) != 0)
			continue;
		fVolumeSlider = (BContinuousParameter *)parameter;
		break;	
	}

#if DEBUG >0	
	if (!fVolumeSlider) {
		TRACE("BSoundPlayer::get_volume_slider couldn't find volume control");
	}
#endif
}


/* virtual */ void
BSoundPlayer::Notify(sound_player_notification what, ...)
{
	CALLED();
	if (fLocker.Lock()) {
		if (fNotifierFunc)
			(*fNotifierFunc)(fCookie, what);
		fLocker.Unlock();
	}
}


/* virtual */ void
BSoundPlayer::PlayBuffer(void *buffer, size_t size, const media_raw_audio_format &format)
{
	if (fLocker.Lock()) {
		if (fPlayBufferFunc)
			(*fPlayBufferFunc)(fCookie, buffer, size, format);
		fLocker.Unlock();
	}
}


/*************************************************************
 * public sound_error
 *************************************************************/


sound_error::sound_error(const char *str)
{
	m_str_const = str;
}


const char *
sound_error::what() const throw ()
{
	return m_str_const;
}

