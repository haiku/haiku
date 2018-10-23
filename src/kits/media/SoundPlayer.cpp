/*
 * Copyright 2002-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 *		Jérôme Duval
 */


#include <SoundPlayer.h>

#include <math.h>
#include <string.h>

#include <Autolock.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <Sound.h>
#include <TimeSource.h>

#include "SoundPlayNode.h"

#include "MediaDebug.h"


// Flags used internally in BSoundPlayer
enum {
	F_NODES_CONNECTED	= (1 << 0),
	F_HAS_DATA			= (1 << 1),
	F_IS_STARTED		= (1 << 2),
	F_MUST_RELEASE_MIXER = (1 << 3),
};


static BSoundPlayer::play_id sCurrentPlayID = 1;


BSoundPlayer::BSoundPlayer(const char* name, BufferPlayerFunc playerFunction,
	EventNotifierFunc eventNotifierFunction, void* cookie)
{
	CALLED();

	TRACE("BSoundPlayer::BSoundPlayer: default constructor used\n");

	media_multi_audio_format format = media_multi_audio_format::wildcard;

	_Init(NULL, &format, name, NULL, playerFunction, eventNotifierFunction,
		cookie);
}


BSoundPlayer::BSoundPlayer(const media_raw_audio_format* _format,
	const char* name, BufferPlayerFunc playerFunction,
	EventNotifierFunc eventNotifierFunction, void* cookie)
{
	CALLED();

	TRACE("BSoundPlayer::BSoundPlayer: raw audio format constructor used\n");

	media_multi_audio_format format = media_multi_audio_format::wildcard;
	*(media_raw_audio_format*)&format = *_format;

#if DEBUG > 0
	char buf[100];
	media_format tmp; tmp.type = B_MEDIA_RAW_AUDIO; tmp.u.raw_audio = format;
	string_for_format(tmp, buf, sizeof(buf));
	TRACE("BSoundPlayer::BSoundPlayer: format %s\n", buf);
#endif

	_Init(NULL, &format, name, NULL, playerFunction, eventNotifierFunction,
		cookie);
}


BSoundPlayer::BSoundPlayer(const media_node& toNode,
	const media_multi_audio_format* format, const char* name,
	const media_input* input, BufferPlayerFunc playerFunction,
	EventNotifierFunc eventNotifierFunction, void* cookie)
{
	CALLED();

	TRACE("BSoundPlayer::BSoundPlayer: multi audio format constructor used\n");

	if ((toNode.kind & B_BUFFER_CONSUMER) == 0)
		debugger("BSoundPlayer: toNode must have B_BUFFER_CONSUMER kind!\n");

#if DEBUG > 0
	char buf[100];
	media_format tmp; tmp.type = B_MEDIA_RAW_AUDIO; tmp.u.raw_audio = *format;
	string_for_format(tmp, buf, sizeof(buf));
	TRACE("BSoundPlayer::BSoundPlayer: format %s\n", buf);
#endif

	_Init(&toNode, format, name, input, playerFunction, eventNotifierFunction,
		cookie);
}


BSoundPlayer::~BSoundPlayer()
{
	CALLED();

	if ((fFlags & F_IS_STARTED) != 0) {
		// block, but don't flush
		Stop(true, false);
	}

	status_t err;
	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster == NULL) {
		TRACE("BSoundPlayer::~BSoundPlayer: Couldn't get BMediaRoster\n");
		goto cleanup;
	}

	if ((fFlags & F_NODES_CONNECTED) != 0) {
		// Ordinarily we'd stop *all* of the nodes in the chain before
		// disconnecting. However, our node is already stopped, and we can't
		// stop the System Mixer.
		// So, we just disconnect from it, and release our references to the
		// nodes that we're using. We *are* supposed to do that even for global
		// nodes like the Mixer.
		err = roster->Disconnect(fMediaOutput, fMediaInput);
		if (err != B_OK) {
			TRACE("BSoundPlayer::~BSoundPlayer: Error disconnecting nodes: "
				"%" B_PRId32 " (%s)\n", err, strerror(err));
		}
	}

	if ((fFlags & F_MUST_RELEASE_MIXER) != 0) {
		// Release the mixer as it was acquired
		// through BMediaRoster::GetAudioMixer()
		err = roster->ReleaseNode(fMediaInput.node);
		if (err != B_OK) {
			TRACE("BSoundPlayer::~BSoundPlayer: Error releasing input node: "
				"%" B_PRId32 " (%s)\n", err, strerror(err));
		}
	}

cleanup:
	// Dispose of the player node

	// We do not call BMediaRoster::ReleaseNode(), since
	// the player was created by using "new". We could
	// call BMediaRoster::UnregisterNode(), but this is
	// supposed to be done by BMediaNode destructor automatically.

	// The node is deleted by the Release() when ref count reach 0.
	// Since we are the sole owners, and no one acquired it
	// this should be the case. The Quit() synchronization
	// is handled by the DeleteHook inheritance.
	// NOTE: this might be crucial when using a BMediaEventLooper.
	if (fPlayerNode->Release() != NULL) {
		TRACE("BSoundPlayer::~BSoundPlayer: Error the producer node "
			"appears to be acquired by someone else than us!");
	}

	// do not delete fVolumeSlider, it belongs to the parameter web
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

	if ((fFlags & F_IS_STARTED) != 0)
		return B_OK;

	BMediaRoster* roster = BMediaRoster::Roster();
	if (!roster) {
		TRACE("BSoundPlayer::Start: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}

	if (!fPlayerNode->TimeSource()->IsRunning()) {
		roster->StartTimeSource(fPlayerNode->TimeSource()->Node(),
			fPlayerNode->TimeSource()->RealTime());
	}

	// Add latency and a few ms to the nodes current time to
	// make sure that we give the producer enough time to run
	// buffers through the node chain, otherwise it'll start
	// up already late

	status_t err = roster->StartNode(fPlayerNode->Node(),
		fPlayerNode->TimeSource()->Now() + Latency() + 5000);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Start: StartNode failed, %" B_PRId32, err);
		return err;
	}

	if (fNotifierFunc != NULL)
		fNotifierFunc(fCookie, B_STARTED, this);

	SetHasData(true);
	atomic_or(&fFlags, F_IS_STARTED);

	return B_OK;
}


void
BSoundPlayer::Stop(bool block, bool flush)
{
	CALLED();

	TRACE("BSoundPlayer::Stop: block %d, flush %d\n", (int)block, (int)flush);

	if ((fFlags & F_NODES_CONNECTED) == 0)
		return;

	// TODO: flush is ignored

	if ((fFlags & F_IS_STARTED) != 0) {
		BMediaRoster* roster = BMediaRoster::Roster();
		if (roster == NULL) {
			TRACE("BSoundPlayer::Stop: Couldn't get BMediaRoster\n");
			return;
		}

		roster->StopNode(fPlayerNode->Node(), 0, true);

		atomic_and(&fFlags, ~F_IS_STARTED);
	}

	if (block) {
		// wait until the node is stopped
		int tries;
		for (tries = 250; fPlayerNode->IsPlaying() && tries != 0; tries--)
			snooze(2000);

		DEBUG_ONLY(if (tries == 0)
			TRACE("BSoundPlayer::Stop: waiting for node stop failed\n"));

		// Wait until all buffers on the way to the physical output have been
		// played
		snooze(Latency() + 2000);
	}

	if (fNotifierFunc)
		fNotifierFunc(fCookie, B_STOPPED, this);

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
		TRACE("BSoundPlayer::Latency: GetLatencyFor failed %" B_PRId32
			" (%s)\n", err, strerror(err));
		return 0;
	}

	TRACE("BSoundPlayer::Latency: latency is %" B_PRId64 "\n", latency);

	return latency;
}


void
BSoundPlayer::SetHasData(bool hasData)
{
	CALLED();
	if (hasData)
		atomic_or(&fFlags, F_HAS_DATA);
	else
		atomic_and(&fFlags, ~F_HAS_DATA);
}


bool
BSoundPlayer::HasData()
{
	CALLED();
	return (atomic_get(&fFlags) & F_HAS_DATA) != 0;
}


BSoundPlayer::BufferPlayerFunc
BSoundPlayer::BufferPlayer() const
{
	CALLED();
	return fPlayBufferFunc;
}


void
BSoundPlayer::SetBufferPlayer(BufferPlayerFunc playerFunction)
{
	CALLED();
	BAutolock _(fLocker);

	fPlayBufferFunc = playerFunction;
}


BSoundPlayer::EventNotifierFunc
BSoundPlayer::EventNotifier() const
{
	CALLED();
	return fNotifierFunc;
}


void
BSoundPlayer::SetNotifier(EventNotifierFunc eventNotifierFunction)
{
	CALLED();
	BAutolock _(fLocker);

	fNotifierFunc = eventNotifierFunction;
}


void*
BSoundPlayer::Cookie() const
{
	CALLED();
	return fCookie;
}


void
BSoundPlayer::SetCookie(void *cookie)
{
	CALLED();
	BAutolock _(fLocker);

	fCookie = cookie;
}


void
BSoundPlayer::SetCallbacks(BufferPlayerFunc playerFunction,
	EventNotifierFunc eventNotifierFunction, void* cookie)
{
	CALLED();
	BAutolock _(fLocker);

	SetBufferPlayer(playerFunction);
	SetNotifier(eventNotifierFunction);
	SetCookie(cookie);
}


/*!	The BeBook is inaccurate about the meaning of this function.
	The probably best interpretation is to return the time that
	has elapsed since playing was started, whichs seems to match
	"CurrentTime() returns the current media time"
*/
bigtime_t
BSoundPlayer::CurrentTime()
{
	if ((fFlags & F_NODES_CONNECTED) == 0)
		return 0;

	return fPlayerNode->CurrentTime();
}


/*!	Returns the current performance time of the sound player node
	being used by the BSoundPlayer. Will return B_ERROR if the
	BSoundPlayer object hasn't been properly initialized.
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

	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster == NULL) {
		TRACE("BSoundPlayer::Preroll: Couldn't get BMediaRoster\n");
		return B_ERROR;
	}

	status_t err = roster->PrerollNode(fMediaOutput.node);
	if (err != B_OK) {
		TRACE("BSoundPlayer::Preroll: Error while PrerollNode: %"
			B_PRId32 " (%s)\n", err, strerror(err));
		return err;
	}

	return B_OK;
}


BSoundPlayer::play_id
BSoundPlayer::StartPlaying(BSound* sound, bigtime_t atTime)
{
	return StartPlaying(sound, atTime, 1.0);
}


BSoundPlayer::play_id
BSoundPlayer::StartPlaying(BSound* sound, bigtime_t atTime, float withVolume)
{
	CALLED();

	// TODO: support the at_time and with_volume parameters
	playing_sound* item = (playing_sound*)malloc(sizeof(playing_sound));
	if (item == NULL)
		return B_NO_MEMORY;

	item->current_offset = 0;
	item->sound = sound;
	item->id = atomic_add(&sCurrentPlayID, 1);
	item->delta = 0;
	item->rate = 0;
	item->volume = withVolume;

	if (!fLocker.Lock()) {
		free(item);
		return B_ERROR;
	}

	sound->AcquireRef();
	item->next = fPlayingSounds;
	fPlayingSounds = item;
	fLocker.Unlock();

	SetHasData(true);
	return item->id;
}


status_t
BSoundPlayer::SetSoundVolume(play_id id, float newVolume)
{
	CALLED();
	if (!fLocker.Lock())
		return B_ERROR;

	playing_sound *item = fPlayingSounds;
	while (item) {
		if (item->id == id) {
			item->volume = newVolume;
			fLocker.Unlock();
			return B_OK;
		}

		item = item->next;
	}

	fLocker.Unlock();
	return B_ENTRY_NOT_FOUND;
}


bool
BSoundPlayer::IsPlaying(play_id id)
{
	CALLED();
	if (!fLocker.Lock())
		return B_ERROR;

	playing_sound *item = fPlayingSounds;
	while (item) {
		if (item->id == id) {
			fLocker.Unlock();
			return true;
		}

		item = item->next;
	}

	fLocker.Unlock();
	return false;
}


status_t
BSoundPlayer::StopPlaying(play_id id)
{
	CALLED();
	if (!fLocker.Lock())
		return B_ERROR;

	playing_sound** link = &fPlayingSounds;
	playing_sound* item = fPlayingSounds;

	while (item != NULL) {
		if (item->id == id) {
			*link = item->next;
			sem_id waitSem = item->wait_sem;
			item->sound->ReleaseRef();
			free(item);
			fLocker.Unlock();

			_NotifySoundDone(id, true);
			if (waitSem >= 0)
				release_sem(waitSem);

			return B_OK;
		}

		link = &item->next;
		item = item->next;
	}

	fLocker.Unlock();
	return B_ENTRY_NOT_FOUND;
}


status_t
BSoundPlayer::WaitForSound(play_id id)
{
	CALLED();
	if (!fLocker.Lock())
		return B_ERROR;

	playing_sound* item = fPlayingSounds;
	while (item != NULL) {
		if (item->id == id) {
			sem_id waitSem = item->wait_sem;
			if (waitSem < 0)
				waitSem = item->wait_sem = create_sem(0, "wait for sound");

			fLocker.Unlock();
			return acquire_sem(waitSem);
		}

		item = item->next;
	}

	fLocker.Unlock();
	return B_ENTRY_NOT_FOUND;
}


float
BSoundPlayer::Volume()
{
	CALLED();
	return pow(10.0, VolumeDB(true) / 20.0);
}


void
BSoundPlayer::SetVolume(float newVolume)
{
	CALLED();
	SetVolumeDB(20.0 * log10(newVolume));
}


float
BSoundPlayer::VolumeDB(bool forcePoll)
{
	CALLED();
	if (!fVolumeSlider)
		return -94.0f; // silence

	if (!forcePoll && system_time() - fLastVolumeUpdate < 500000)
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
BSoundPlayer::SetVolumeDB(float volumeDB)
{
	CALLED();
	if (!fVolumeSlider)
		return;

	float minDB = fVolumeSlider->MinValue();
	float maxDB = fVolumeSlider->MaxValue();
	if (volumeDB < minDB)
		volumeDB = minDB;
	if (volumeDB > maxDB)
		volumeDB = maxDB;

	int count = fVolumeSlider->CountChannels();
	float values[count];
	for (int i = 0; i < count; i++)
		values[i] = volumeDB;
	fVolumeSlider->SetValue(values, sizeof(float) * count, 0);

	fVolumeDB = volumeDB;
	fLastVolumeUpdate = system_time();
}


status_t
BSoundPlayer::GetVolumeInfo(media_node* _node, int32* _parameterID,
	float* _minDB, float* _maxDB)
{
	CALLED();
	if (fVolumeSlider == NULL)
		return B_NO_INIT;

	if (_node != NULL)
		*_node = fMediaInput.node;
	if (_parameterID != NULL)
		*_parameterID = fVolumeSlider->ID();
	if (_minDB != NULL)
		*_minDB = fVolumeSlider->MinValue();
	if (_maxDB != NULL)
		*_maxDB = fVolumeSlider->MaxValue();

	return B_OK;
}


// #pragma mark - protected BSoundPlayer


void
BSoundPlayer::SetInitError(status_t error)
{
	CALLED();
	fInitStatus = error;
}


// #pragma mark - private BSoundPlayer


void
BSoundPlayer::_SoundPlayBufferFunc(void *cookie, void *buffer, size_t size,
	const media_raw_audio_format &format)
{
	// TODO: support more than one sound and make use of the format parameter
	BSoundPlayer *player = (BSoundPlayer *)cookie;
	if (!player->fLocker.Lock()) {
		memset(buffer, 0, size);
		return;
	}

	playing_sound *sound = player->fPlayingSounds;
	if (sound == NULL) {
		player->SetHasData(false);
		player->fLocker.Unlock();
		memset(buffer, 0, size);
		return;
	}

	size_t used = 0;
	if (!sound->sound->GetDataAt(sound->current_offset, buffer, size, &used)) {
		// will take care of removing the item and notifying others
		player->StopPlaying(sound->id);
		player->fLocker.Unlock();
		memset(buffer, 0, size);
		return;
	}

	sound->current_offset += used;
	player->fLocker.Unlock();

	if (used < size)
		memset((uint8 *)buffer + used, 0, size - used);
}


status_t BSoundPlayer::_Reserved_SoundPlayer_0(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_1(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_2(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_3(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_4(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_5(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_6(void*, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_7(void*, ...) { return B_ERROR; }


void
BSoundPlayer::_Init(const media_node* node,
	const media_multi_audio_format* format, const char* name,
	const media_input* input, BufferPlayerFunc playerFunction,
	EventNotifierFunc eventNotifierFunction, void* cookie)
{
	CALLED();
	fPlayingSounds = NULL;
	fWaitingSounds = NULL;

	fPlayerNode = NULL;
	if (playerFunction == NULL) {
		fPlayBufferFunc = _SoundPlayBufferFunc;
		fCookie = this;
	} else {
		fPlayBufferFunc = playerFunction;
		fCookie = cookie;
	}

	fNotifierFunc = eventNotifierFunction;
	fVolumeDB = 0.0f;
	fFlags = 0;
	fInitStatus = B_ERROR;
	fParameterWeb = NULL;
	fVolumeSlider = NULL;
	fLastVolumeUpdate = 0;

	BMediaRoster* roster = BMediaRoster::Roster();
	if (roster == NULL) {
		TRACE("BSoundPlayer::_Init: Couldn't get BMediaRoster\n");
		return;
	}

	// The inputNode that our player node will be
	// connected with is either supplied by the user
	// or the system audio mixer
	media_node inputNode;
	if (node) {
		inputNode = *node;
	} else {
		fInitStatus = roster->GetAudioMixer(&inputNode);
		if (fInitStatus != B_OK) {
			TRACE("BSoundPlayer::_Init: Couldn't GetAudioMixer\n");
			return;
		}
		fFlags |= F_MUST_RELEASE_MIXER;
	}

	media_output _output;
	media_input _input;
	int32 inputCount;
	int32 outputCount;
	media_format tryFormat;

	// Create the player node and register it
	fPlayerNode = new BPrivate::SoundPlayNode(name, this);
	fInitStatus = roster->RegisterNode(fPlayerNode);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't RegisterNode: %s\n",
			strerror(fInitStatus));
		return;
	}

	// set the producer's time source to be the "default" time source,
	// which the system audio mixer uses too.
	media_node timeSource;
	fInitStatus = roster->GetTimeSource(&timeSource);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't GetTimeSource: %s\n",
			strerror(fInitStatus));
		return;
	}
	fInitStatus = roster->SetTimeSourceFor(fPlayerNode->Node().node,
		timeSource.node);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't SetTimeSourceFor: %s\n",
			strerror(fInitStatus));
		return;
	}

	// find a free media_input
	if (!input) {
		fInitStatus = roster->GetFreeInputsFor(inputNode, &_input, 1,
			&inputCount, B_MEDIA_RAW_AUDIO);
		if (fInitStatus != B_OK) {
			TRACE("BSoundPlayer::_Init: Couldn't GetFreeInputsFor: %s\n",
				strerror(fInitStatus));
			return;
		}
		if (inputCount < 1) {
			TRACE("BSoundPlayer::_Init: Couldn't find a free input\n");
			fInitStatus = B_ERROR;
			return;
		}
	} else {
		_input = *input;
	}

	// find a free media_output
	fInitStatus = roster->GetFreeOutputsFor(fPlayerNode->Node(), &_output, 1,
		&outputCount, B_MEDIA_RAW_AUDIO);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't GetFreeOutputsFor: %s\n",
			strerror(fInitStatus));
		return;
	}
	if (outputCount < 1) {
		TRACE("BSoundPlayer::_Init: Couldn't find a free output\n");
		fInitStatus = B_ERROR;
		return;
	}

	// Set an appropriate run mode for the producer
	fInitStatus = roster->SetRunModeNode(fPlayerNode->Node(),
		BMediaNode::B_INCREASE_LATENCY);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't SetRunModeNode: %s\n",
			strerror(fInitStatus));
		return;
	}

	// setup our requested format (can still have many wildcards)
	tryFormat.type = B_MEDIA_RAW_AUDIO;
	tryFormat.u.raw_audio = *format;

#if DEBUG > 0
	char buf[100];
	string_for_format(tryFormat, buf, sizeof(buf));
	TRACE("BSoundPlayer::_Init: trying to connect with format %s\n", buf);
#endif

	// and connect the nodes
	fInitStatus = roster->Connect(_output.source, _input.destination,
		&tryFormat, &fMediaOutput, &fMediaInput);
	if (fInitStatus != B_OK) {
		TRACE("BSoundPlayer::_Init: Couldn't Connect: %s\n",
			strerror(fInitStatus));
		return;
	}

	fFlags |= F_NODES_CONNECTED;

	_GetVolumeSlider();

	TRACE("BSoundPlayer node %" B_PRId32 " has timesource %" B_PRId32 "\n",
		fPlayerNode->Node().node, fPlayerNode->TimeSource()->Node().node);
}


void
BSoundPlayer::_NotifySoundDone(play_id id, bool gotToPlay)
{
	CALLED();
	Notify(B_SOUND_DONE, id, gotToPlay);
}


void
BSoundPlayer::_GetVolumeSlider()
{
	CALLED();

	ASSERT(fVolumeSlider == NULL);

	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (!roster) {
		TRACE("BSoundPlayer::_GetVolumeSlider failed to get BMediaRoster");
		return;
	}

	if (!fParameterWeb && roster->GetParameterWebFor(fMediaInput.node, &fParameterWeb) < B_OK) {
		TRACE("BSoundPlayer::_GetVolumeSlider couldn't get parameter web");
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
		TRACE("BSoundPlayer::_GetVolumeSlider couldn't find volume control");
	}
#endif
}


void
BSoundPlayer::Notify(sound_player_notification what, ...)
{
	CALLED();
	if (fLocker.Lock()) {
		if (fNotifierFunc)
			(*fNotifierFunc)(fCookie, what);
		fLocker.Unlock();
	}
}


void
BSoundPlayer::PlayBuffer(void* buffer, size_t size,
	const media_raw_audio_format& format)
{
	if (fLocker.Lock()) {
		if (fPlayBufferFunc)
			(*fPlayBufferFunc)(fCookie, buffer, size, format);
		fLocker.Unlock();
	}
}


// #pragma mark - public sound_error


sound_error::sound_error(const char* string)
{
	m_str_const = string;
}


const char*
sound_error::what() const throw()
{
	return m_str_const;
}

