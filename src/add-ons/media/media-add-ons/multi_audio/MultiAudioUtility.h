/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef MULTI_AUDIO_UTILITY_H
#define MULTI_AUDIO_UTILITY_H


#include "hmulti_audio.h"


namespace MultiAudio {

// sample rate & format conversion
float convert_to_sample_rate(uint32 rate);
uint32 convert_from_sample_rate(float rate);
uint32 convert_to_media_format(uint32 format);
int16 convert_to_valid_bits(uint32 format);
uint32 convert_from_media_format(uint32 format);
uint32 select_sample_rate(uint32 rate);
uint32 select_format(uint32 format);

// device driver interface
status_t get_description(int device, multi_description* description);

status_t get_enabled_channels(int device, multi_channel_enable* enable);
status_t set_enabled_channels(int device, multi_channel_enable* enable);

status_t get_global_format(int device, multi_format_info* info);
status_t set_global_format(int device, multi_format_info* info);
status_t get_buffers(int device, multi_buffer_list* list);
status_t buffer_exchange(int device, multi_buffer_info* info);

status_t list_mix_controls(int device, multi_mix_control_info* info);
status_t get_mix(int device, multi_mix_value_info* info);
status_t set_mix(int device, multi_mix_value_info* info);

}	// namespace MultiAudio

#endif	// MULTI_AUDIO_UTILITY_H
