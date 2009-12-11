/*
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */
#ifndef MULTI_AUDIO_DEVICE_H
#define MULTI_AUDIO_DEVICE_H


#include "hmulti_audio.h"


#define MAX_CONTROLS	128
#define MAX_CHANNELS	32
#define MAX_BUFFERS		32


class MultiAudioDevice {
public:
						MultiAudioDevice(const char* name, const char* path);
						~MultiAudioDevice();

	status_t			InitCheck() const;

	const multi_description& Description() const { return fDescription; }
	const multi_format_info& FormatInfo() const { return fFormatInfo; }
	const multi_buffer_list& BufferList() const { return fBufferList; }
	const multi_mix_control_info& MixControlInfo() const
							{ return fMixControlInfo; }

	status_t			BufferExchange(multi_buffer_info* bufferInfo);
	status_t			SetMix(multi_mix_value_info* mixValueInfo);
	status_t			GetMix(multi_mix_value_info* mixValueInfo);

	status_t			SetInputFrameRate(uint32 multiAudioRate);
	status_t			SetOutputFrameRate(uint32 multiAudioRate);

private:
	status_t			_InitDriver();
	status_t			_GetBuffers();

private:
	status_t 			fInitStatus;
	int					fDevice;
	char				fPath[B_PATH_NAME_LENGTH];

	multi_description	fDescription;
	multi_channel_info	fChannelInfo[MAX_CHANNELS];
	multi_format_info 	fFormatInfo;
	multi_buffer_list 	fBufferList;

	multi_mix_control_info fMixControlInfo;
	multi_mix_control	fMixControl[MAX_CONTROLS];

	buffer_desc			fPlayBufferList[MAX_BUFFERS * MAX_CHANNELS];
	buffer_desc			fRecordBufferList[MAX_BUFFERS * MAX_CHANNELS];
	buffer_desc*		fPlayBuffers[MAX_BUFFERS];
	buffer_desc*		fRecordBuffers[MAX_BUFFERS];
};

#endif	// MULTI_AUDIO_DEVICE_H
