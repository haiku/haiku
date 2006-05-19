/*
 * SoundOutput.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "SoundOutput.h"

#include <OS.h>
#include <SoundPlayer.h>
#include <Debug.h>

#include <new>
#include <string.h>

using std::nothrow;


SoundOutput::SoundOutput(const char *name, const media_multi_audio_format &format)
 :	fSoundPlayer(new(nothrow) BSoundPlayer(static_cast<const media_raw_audio_format *>(&format), name, play_buffer, NULL, this))
 ,	fBuffer(new(nothrow) uint8[format.buffer_size])
 ,	fBufferSize(format.buffer_size)
 ,	fBufferWriteable(create_sem(1, "SoundOutput writeable"))
 ,	fBufferReadable(create_sem(0, "SoundOutput readable"))
 ,	fBufferDuration(0)
 ,	fIsPlaying(false)
{
	if (format.channel_count > 0 && format.frame_rate > 0 && (format.format & 0xf) != 0)
		fBufferDuration = bigtime_t(1000000.0 * (format.buffer_size / (format.channel_count * (format.format & 0xf))) / format.frame_rate);
	printf("SoundOutput: buffer duration is %Ld\n", fBufferDuration);
}


SoundOutput::~SoundOutput()
{
	delete fSoundPlayer;
	delete_sem(fBufferWriteable);
	delete_sem(fBufferReadable);
	delete [] fBuffer;
}
					

status_t
SoundOutput::InitCheck()
{
	if (!fSoundPlayer || !fBuffer || fBufferSize <= 0 || fBufferWriteable < B_OK || fBufferReadable < B_OK)
		return B_ERROR;
	return fSoundPlayer->InitCheck();
}
			

bigtime_t
SoundOutput::Latency()
{
	// Because of buffering, latency of SoundOutput is
	// slightly higher then the BSoundPlayer latency.
	return fSoundPlayer->Latency() + min_c(1000, fBufferDuration / 4);
}


float
SoundOutput::Volume()
{
	return fSoundPlayer->Volume();
}


void
SoundOutput::SetVolume(float new_volume)
{
	fSoundPlayer->SetVolume(new_volume);
}


void
SoundOutput::Play(const void *data, size_t size)
{
	ASSERT(size > 0 && size <= fBufferSize);
	acquire_sem(fBufferWriteable);
	memcpy(fBuffer, data, size);
	size_t fillsize = fBufferSize - size;
	if (fillsize)
		memset(fBuffer + size, 0, fillsize);
	release_sem(fBufferReadable);
	if (!fIsPlaying) {
		fSoundPlayer->SetHasData(true);
		fSoundPlayer->Start();
		fIsPlaying = true;
	}
}


void
SoundOutput::PlayBuffer(void *buffer)
{
	if (acquire_sem_etc(fBufferReadable, 1, B_RELATIVE_TIMEOUT, fBufferDuration / 2) != B_OK) {
		printf("SoundOutput: buffer not ready, playing silence\n");
		memset(buffer, 0, fBufferSize);
		return;
	}
	memcpy(buffer, fBuffer, fBufferSize);
	release_sem(fBufferWriteable);
}


void
SoundOutput::play_buffer(void *cookie, void *buffer, size_t size, const media_raw_audio_format & format)
{
	ASSERT(size == static_cast<SoundOutput *>(cookie)->fBufferSize);
	ASSERT(size == format.buffer_size);
	static_cast<SoundOutput *>(cookie)->PlayBuffer(buffer);
}

