/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 *
 * Distributed under the terms of the MIT License.
 */


#include "MultiAudioUtility.h"

#include <errno.h>

#include <MediaDefs.h>


const float kSampleRates[] = {
	8000.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0, 44100.0,
	48000.0, 64000.0, 88200.0, 96000.0, 176400.0, 192000.0, 384000.0, 1536000.0
};
const uint32 kSampleRateCount = sizeof(kSampleRates) / sizeof(kSampleRates[0]);


template<typename T> static status_t
call_driver(int device, int32 op, T* data)
{
	if (ioctl(device, op, data, sizeof(T)) != 0)
		return errno;

	return B_OK;
}


namespace MultiAudio {


float
convert_to_sample_rate(uint32 rate)
{
	for (uint32 i = 0; i < kSampleRateCount; i++) {
		if (rate & 1)
			return kSampleRates[i];

		rate >>= 1;
	}

	return 0.0;
}


uint32
convert_from_sample_rate(float rate)
{
	for (uint32 i = 0; i < kSampleRateCount; i++) {
		if (rate <= kSampleRates[i])
			return 1 << i;
	}

	return 0;
}


uint32
convert_to_media_format(uint32 format)
{
	switch (format) {
		case B_FMT_FLOAT:
			return media_raw_audio_format::B_AUDIO_FLOAT;
		case B_FMT_18BIT:
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
			return media_raw_audio_format::B_AUDIO_INT;
		case B_FMT_16BIT:
			return media_raw_audio_format::B_AUDIO_SHORT;
		case B_FMT_8BIT_S:
			return media_raw_audio_format::B_AUDIO_CHAR;
		case B_FMT_8BIT_U:
			return media_raw_audio_format::B_AUDIO_UCHAR;

		default:
			return 0;
	}
}


int16
convert_to_valid_bits(uint32 format)
{
	switch (format) {
		case B_FMT_18BIT:
			return 18;
		case B_FMT_20BIT:
			return 20;
		case B_FMT_24BIT:
			return 24;
		case B_FMT_32BIT:
			return 32;

		default:
			return 0;
	}
}


uint32
convert_from_media_format(uint32 format)
{
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			return B_FMT_FLOAT;
		case media_raw_audio_format::B_AUDIO_INT:
			return B_FMT_32BIT;
		case media_raw_audio_format::B_AUDIO_SHORT:
			return B_FMT_16BIT;
		case media_raw_audio_format::B_AUDIO_CHAR:
			return B_FMT_8BIT_S;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			return B_FMT_8BIT_U;

		default:
			return 0;
	}
}


uint32
select_sample_rate(uint32 rate)
{
	// highest rate
	uint32 crate = B_SR_1536000;
	while (crate != 0) {
		if (rate & crate)
			return crate;
		crate >>= 1;
	}
	return 0;
}


uint32
select_format(uint32 format)
{
	// best format
	// TODO ensure we support this format
	/*if (format & B_FMT_FLOAT)
		return B_FMT_FLOAT;*/
	if (format & B_FMT_32BIT)
		return B_FMT_32BIT;
	if (format & B_FMT_24BIT)
		return B_FMT_24BIT;
	if (format & B_FMT_20BIT)
		return B_FMT_20BIT;
	if (format & B_FMT_18BIT)
		return B_FMT_18BIT;
	if (format & B_FMT_16BIT)
		return B_FMT_16BIT;
	if (format & B_FMT_8BIT_S)
		return B_FMT_8BIT_S;
	if (format & B_FMT_8BIT_U)
		return B_FMT_8BIT_U;

	return 0;
}


status_t
get_description(int device, multi_description* description)
{
	return call_driver(device, B_MULTI_GET_DESCRIPTION, description);
}


status_t
get_enabled_channels(int device, multi_channel_enable* enable)
{
	return call_driver(device, B_MULTI_GET_ENABLED_CHANNELS, enable);
}


status_t
set_enabled_channels(int device, multi_channel_enable* enable)
{
	return call_driver(device, B_MULTI_SET_ENABLED_CHANNELS, enable);
}


status_t
get_global_format(int device, multi_format_info* info)
{
	return call_driver(device, B_MULTI_GET_GLOBAL_FORMAT, info);
}


status_t
set_global_format(int device, multi_format_info* info)
{
	return call_driver(device, B_MULTI_SET_GLOBAL_FORMAT, info);
}


status_t
get_buffers(int device, multi_buffer_list* list)
{
	return call_driver(device, B_MULTI_GET_BUFFERS, list);
}


status_t
buffer_exchange(int device, multi_buffer_info* info)
{
	return call_driver(device, B_MULTI_BUFFER_EXCHANGE, info);
}


status_t
list_mix_controls(int device, multi_mix_control_info* info)
{
	return call_driver(device, B_MULTI_LIST_MIX_CONTROLS, info);
}


status_t
get_mix(int device, multi_mix_value_info* info)
{
	return call_driver(device, B_MULTI_GET_MIX, info);
}


status_t
set_mix(int device, multi_mix_value_info* info)
{
	return call_driver(device, B_MULTI_SET_MIX, info);
}


}	// namespace MultiAudio
