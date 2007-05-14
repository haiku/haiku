/*
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */

#ifndef _MULTIAUDIODEVICE_H
#define _MULTIAUDIODEVICE_H

#include "hmulti_audio.h"

#define MAX_CONTROLS	128
#define MAX_CHANNELS	32
#define NB_BUFFERS		32

class MultiAudioDevice
{
	public:
		explicit MultiAudioDevice(const char *name, const char* path);
		virtual ~MultiAudioDevice(void);

		virtual status_t InitCheck(void) const;

		static float convert_multiaudio_rate_to_media_rate(uint32 rate);
		static uint32 convert_media_rate_to_multiaudio_rate(float rate);
		static uint32 convert_multiaudio_format_to_media_format(uint32 fmt);
		static int16 convert_multiaudio_format_to_valid_bits(uint32 fmt);
		static uint32 convert_media_format_to_multiaudio_format(uint32 fmt);
		static uint32 select_multiaudio_rate(uint32 rate);
		static uint32 select_multiaudio_format(uint32 fmt);

		int DoBufferExchange(multi_buffer_info *MBI);
		int DoSetMix(multi_mix_value_info *MMVI);
		int DoGetMix(multi_mix_value_info *MMVI);

	private:
		status_t			InitDriver();

		int						fd; 			//file descriptor for hw driver
		char					fDevice_name[32];
		char					fDevice_path[32];

	public:
		multi_description		MD;
		multi_channel_info		MCI[MAX_CHANNELS];
		multi_format_info 		MFI;
		multi_buffer_list 		MBL;

		multi_mix_control_info 	MMCI;
		multi_mix_control		MMC[MAX_CONTROLS];

		buffer_desc		play_buffer_list[NB_BUFFERS * MAX_CHANNELS];
		buffer_desc		record_buffer_list[NB_BUFFERS * MAX_CHANNELS];
		buffer_desc 	*play_buffer_desc[NB_BUFFERS];
		buffer_desc 	*record_buffer_desc[NB_BUFFERS];

	private:
		status_t 				fInitCheckStatus;
};

#endif
