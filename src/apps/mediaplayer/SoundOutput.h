/*
 * SoundOutput.h - Media Player for the Haiku Operating System
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
#ifndef __SOUND_OUTPUT_H
#define __SOUND_OUTPUT_H

#include <MediaDefs.h>

class BSoundPlayer;

class SoundOutput
{
public:
					SoundOutput(const char *name, const media_multi_audio_format &format);
					~SoundOutput();
					
	status_t		InitCheck();
					
	bigtime_t		Latency();

	float			Volume();
	void			SetVolume(float new_volume);
	
	void			Play(const void *data, size_t size);
	
private:
	static void		play_buffer(void *cookie, void *buffer, size_t size, const media_raw_audio_format & format);
	void			PlayBuffer(void *buffer);
	
private:
	BSoundPlayer *	fSoundPlayer;
	uint8 *			fBuffer;
	size_t			fBufferSize;
	sem_id			fBufferWriteable;
	sem_id			fBufferReadable;
	bigtime_t		fBufferDuration;
	bool			fIsPlaying;
};

#endif
