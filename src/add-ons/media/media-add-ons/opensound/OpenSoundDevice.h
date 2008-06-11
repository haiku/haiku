/*
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */

#ifndef _OPENSOUNDDEVICE_H
#define _OPENSOUNDDEVICE_H

#include <OS.h>
#include <List.h>
#include <MediaFormats.h>
#include <Locker.h>
#include "soundcard.h"

//
//#define OSS_PREFIX "/dev/audio/oss/"
#define OSS_PREFIX "/dev/oss/"
// should be fixed later
#define OSS_MIXER_DEV "/dev/mixer"

#define MAX_CONTROLS	128
#define MAX_CHANNELS	32
#define NB_BUFFERS		32

#define DEFAULT_BUFFER_SIZE 2048

/* define to support encoded audio (AC3, MPEG, ...) when the card supports it */
//#define ENABLE_NON_RAW_SUPPORT 1
//XXX: make it a BParameter ?

#define ENABLE_REC 1

// timeout in OpenSoundNode::RunThread()
#define MIN_SNOOZING 2000

// pretend we don't drift
#define DISABLE_DRIFT 1


/* bit mask of supported formats for raw_audio */
/* also used to mark the raw_audio node input&outputs */
//XXX: _OE ?
#define AFMT_SUPPORTED_PCM (AFMT_U8|AFMT_S8|\
							AFMT_S16_NE|\
							AFMT_S24_NE|AFMT_S32_NE|\
							AFMT_S16_OE|\
							AFMT_S24_OE|AFMT_S32_OE|\
							AFMT_FLOAT)


extern const int gSupportedFormats[];
extern const char *gSupportedFormatsNames[];

class OpenSoundDeviceEngine;
class OpenSoundDeviceMixer;


class OpenSoundDevice
{
	public:
		explicit OpenSoundDevice(oss_card_info *cardinfo);
		virtual ~OpenSoundDevice(void);

		status_t			InitDriver();
		virtual status_t InitCheck(void) const;
		
		status_t			AddEngine(oss_audioinfo *info);
		status_t			AddMixer(oss_mixerinfo *info);
//		status_t			AddMidi();

		int32					CountEngines();
		int32					CountMixers();
		OpenSoundDeviceEngine	*EngineAt(int32 i);
		OpenSoundDeviceMixer	*MixerAt(int32 i);

		OpenSoundDeviceEngine	*NextFreeEngineAt(int32 i, bool rec=false);
		
		BLocker					*Locker() { return &fLocker; };

		static float convert_oss_rate_to_media_rate(int rate);
		static int convert_media_rate_to_oss_rate(float rate);
		static uint32 convert_oss_format_to_media_format(int fmt);
		static int convert_oss_format_to_endian(int fmt);
		static int16 convert_oss_format_to_valid_bits(int fmt);
		static int convert_media_format_to_oss_format(uint32 fmt);
		static int select_oss_rate(const oss_audioinfo *info, int rate);
		static int select_oss_format(int fmt);
		
		static status_t get_media_format_description_for(int fmt, media_format_description *desc, int count=1);
		static status_t register_media_formats();
		static status_t get_media_format_for(int fmt, media_format &format);

	public:
		oss_card_info			fCardInfo;

	private:
		status_t 				fInitCheckStatus;
		BList					fEngines;
		BList					fMixers;
friend class OpenSoundNode; // ugly
friend class OpenSoundDeviceEngine; // ugly
		audio_buf_info			fFragments;
		BLocker					fLocker;
};

#endif
