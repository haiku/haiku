/*
 * Copyright (c) 2002-2007, Jerome Duval (jerome.duval@free.fr)
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 */

#include "OpenSoundDevice.h"
#include "OpenSoundDeviceEngine.h"
#include "OpenSoundDeviceMixer.h"
#include "debug.h"
#include "driver_io.h"
#include <MediaDefs.h>
#include <Debug.h>
#include <errno.h>
#include <string.h>

const int gSupportedFormats[] = {
	AFMT_SUPPORTED_PCM,
#ifdef ENABLE_NON_RAW_SUPPORT
	AFMT_MU_LAW,
	AFMT_A_LAW,
	AFMT_IMA_ADPCM,
	AFMT_MPEG,
	AFMT_AC3,
	AFMT_VORBIS,
	AFMT_SPDIF_RAW,
	AFMT_S24_PACKED,
#endif /*ENABLE_NON_RAW_SUPPORT*/
	0
};

const char* gSupportedFormatsNames[] = {
	"raw", //AFMT_SUPPORTED_PCM,
	"µ-law", //AFMT_MU_LAW,
	"a-law", //AFMT_A_LAW,
	"IMA4 ADPCM", //AFMT_IMA_ADPCM,
	"MPEG layer 2", //AFMT_MPEG,
	"AC3", //AFMT_AC3,
	"Vorbis", //AFMT_VORBIS,
	"Raw S/PDIF", //AFMT_SPDIF_RAW,
	"Packed 24bit", //AFMT_S24_PACKED,*/
	0
};




float OpenSoundDevice::convert_oss_rate_to_media_rate(int rate)
{
	return (float)rate;
}


int OpenSoundDevice::convert_media_rate_to_oss_rate(float rate)
{
	return (int)rate;
}


uint32 OpenSoundDevice::convert_oss_format_to_media_format(int fmt)
{
	if (fmt & AFMT_FLOAT)
		return media_raw_audio_format::B_AUDIO_FLOAT;
	if (fmt & AFMT_S32_LE || 
		fmt & AFMT_S24_LE || 
		fmt & AFMT_S32_BE || 
		fmt & AFMT_S24_BE)
		return media_raw_audio_format::B_AUDIO_INT;
	if (fmt & AFMT_S16_LE || 
		fmt & AFMT_S16_BE) /* U16 unsupported */
		return media_raw_audio_format::B_AUDIO_SHORT;
	if (fmt & AFMT_S8)
		return media_raw_audio_format::B_AUDIO_CHAR;
	if (fmt & AFMT_U8)
		return media_raw_audio_format::B_AUDIO_UCHAR;
	return 0;
}


int OpenSoundDevice::convert_oss_format_to_endian(int fmt)
{
	if (fmt & AFMT_FLOAT)
		return B_MEDIA_HOST_ENDIAN;
	if (fmt & AFMT_S32_LE || 
		fmt & AFMT_S24_LE || 
		fmt & AFMT_S16_LE || 
		fmt & AFMT_U16_LE || 
		fmt & AFMT_S24_PACKED)
		return B_MEDIA_LITTLE_ENDIAN;
	if (fmt & AFMT_S32_BE || 
		fmt & AFMT_S24_BE || 
		fmt & AFMT_S16_BE || 
		fmt & AFMT_U16_BE)
		return B_MEDIA_BIG_ENDIAN;
	if (fmt & AFMT_S8 ||
		fmt & AFMT_U8)
		return B_MEDIA_HOST_ENDIAN;
	return B_MEDIA_HOST_ENDIAN;
}

int16 OpenSoundDevice::convert_oss_format_to_valid_bits(int fmt)
{
	if (fmt & AFMT_S32_NE)
		return 32;
	if (fmt & AFMT_S24_NE)
		return 24;
	if (fmt & AFMT_S32_OE)
		return 32;
	if (fmt & AFMT_S24_OE)
		return 24;
	if (fmt & AFMT_S24_PACKED)
		return 24;
	if (fmt & AFMT_SPDIF_RAW)
		return 32;
	return 0;
}


int OpenSoundDevice::convert_media_format_to_oss_format(uint32 fmt)
{
	switch (fmt) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			return AFMT_FLOAT;
		case media_raw_audio_format::B_AUDIO_INT:
			return AFMT_S32_NE;
		case media_raw_audio_format::B_AUDIO_SHORT:
			return AFMT_S16_NE;
		case media_raw_audio_format::B_AUDIO_CHAR:
			return AFMT_S8;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			return AFMT_U8;
		default:
			return 0;
	}
}


int OpenSoundDevice::select_oss_rate(const oss_audioinfo *info, int rate)
{
	if (info->caps & PCM_CAP_FREERATE) {
		// if not wildcard and matches, return the hint
		if (rate && rate >= info->min_rate && rate <= info->max_rate)
			return rate;
		// else use max available
		return info->max_rate;
	}
	uint32 max_rate = 0;
	for (uint32 i = 0; i < info->nrates; i++) {
		if ((int32)info->rates[i] < info->min_rate
			|| (int32)info->rates[i] > info->max_rate)
			continue;
		// if the hint matches
		if (rate && rate == (int32)info->rates[i])
			return rate;
		if (info->rates[i] > max_rate)
			max_rate = info->rates[i];
	}
	// highest rate
	return max_rate;
}


int OpenSoundDevice::select_oss_format(int fmt)
{
	//highest format, Native Endian first
	if (fmt & AFMT_FLOAT) {
		return AFMT_FLOAT;
	} else if (fmt & AFMT_S32_NE) {
		return AFMT_S32_NE;
	} else if (fmt & AFMT_S24_NE) {
		return AFMT_S24_NE;
	} else if (fmt & AFMT_S16_NE) {
		return AFMT_S16_NE;
	} else if (fmt & AFMT_S32_OE) {
		return AFMT_S32_OE;
	} else if (fmt & AFMT_S24_OE) {
		return AFMT_S24_OE;
	} else if (fmt & AFMT_S16_OE) {
		return AFMT_S16_OE;
	} else if (fmt & AFMT_S8) {
		return AFMT_S8;
	} else if (fmt & AFMT_U8) {
		return AFMT_U8;
	} else
		return 0;
}

status_t OpenSoundDevice::get_media_format_description_for(int fmt, media_format_description *desc, int count)
{
	int i;
	for (i = 0; i < count; i++)
		memset(&desc[i], 0, sizeof(media_format_description));
	if (count < 1)
		return 0;
	if (fmt & AFMT_SUPPORTED_PCM) {
		desc[0].family = B_BEOS_FORMAT_FAMILY;
		desc[0].u.beos.format = 1;
		return 1;
	}
	switch (fmt) {
	case AFMT_MU_LAW:
		desc[0].family = B_MISC_FORMAT_FAMILY;
		desc[0].u.misc.file_format = '.snd';
		desc[0].u.misc.codec = 'ulaw';
		if (count < 2)
			return 1;
		desc[1].family = B_WAV_FORMAT_FAMILY;
		desc[1].u.wav.codec = 0x07;
		return 2;
		if (count < 3)
			return 2;
		desc[1].family = B_QUICKTIME_FORMAT_FAMILY;
		desc[1].u.quicktime.codec = 'ulaw';
		desc[1].u.quicktime.vendor = 0;
		return 3;
	case AFMT_A_LAW:
		desc[0].family = B_MISC_FORMAT_FAMILY;
		desc[0].u.misc.file_format = '.snd';
		desc[0].u.misc.codec = 'alaw';
		return 1;
		// wav/qt ?
	case AFMT_IMA_ADPCM:
		desc[0].family = B_BEOS_FORMAT_FAMILY;
		desc[0].u.beos.format = 'ima4';
		return 1;
	case AFMT_MPEG: /* layer 2 */
		desc[0].family = B_MPEG_FORMAT_FAMILY;
		desc[0].u.mpeg.id = 0x102;
		if (count < 2)
			return 1;
		desc[1].family = B_WAV_FORMAT_FAMILY;
		desc[1].u.wav.codec = 0x50;
		if (count < 3)
			return 2;
		desc[1].family = B_AVI_FORMAT_FAMILY;
		desc[1].u.avi.codec = 0x65610050;
		return 3;
	case AFMT_AC3:
		desc[0].family = B_WAV_FORMAT_FAMILY;
		desc[0].u.wav.codec = 0x2000;
		if (count < 2)
			return 1;
		desc[1].family = B_AVI_FORMAT_FAMILY;
		desc[1].u.avi.codec = 0x65612000;
		return 2;
	case AFMT_VORBIS:
		// nothing official
		desc[0].family = B_MISC_FORMAT_FAMILY;
		desc[0].u.misc.file_format = 'OSS4';
		desc[0].u.misc.codec = 'VORB';
		return 1;
	case AFMT_SPDIF_RAW:
		// nothing official
		desc[0].family = B_MISC_FORMAT_FAMILY;
		desc[0].u.misc.file_format = 'OSS4';
		desc[0].u.misc.codec = 'PDIF';
		return 1;
	case AFMT_S24_PACKED:
		// nothing official
		desc[0].family = B_MISC_FORMAT_FAMILY;
		desc[0].u.misc.file_format = 'OSS4';
		desc[0].u.misc.codec = 'S24P';
		return 1;
	default:
		return EINVAL;
	}
	return 0;
};


status_t OpenSoundDevice::register_media_formats()
{
	status_t err;
	int i, count;
	BMediaFormats formats;
	if (formats.InitCheck() < B_OK)
		return formats.InitCheck();
	media_format format;
	for (i = 0; gSupportedFormats[i]; i++) {
		media_format_description desc[10];
		err = count = get_media_format_description_for(gSupportedFormats[i], desc, 10);
		if (err < 1)
			continue;
		if (gSupportedFormats[i] & AFMT_SUPPORTED_PCM) {
			format.type = B_MEDIA_RAW_AUDIO;
			format.u.raw_audio = media_multi_audio_format::wildcard;
		} else {
			format.type = B_MEDIA_ENCODED_AUDIO;
			format.u.encoded_audio = media_encoded_audio_format::wildcard;
		}
		err = formats.MakeFormatFor(desc, count, &format);
		PRINT(("OpenSoundDevice::register_media_formats: MakeFormatFor: %s\n", strerror(err)));
	}
	return B_OK;
};


status_t OpenSoundDevice::get_media_format_for(int fmt, media_format &format)
{
	status_t err;
	BMediaFormats formats;
	if (formats.InitCheck() < B_OK)
		return formats.InitCheck();
	/* shortcut for raw */
	if (fmt & AFMT_SUPPORTED_PCM) {
		format = media_format();
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
		return B_OK;
	}
	media_format_description desc;
	err = get_media_format_description_for(fmt, &desc);
	if (err < B_OK)
		return err;
	err = formats.GetFormatFor(desc, &format);
	return err;
};


OpenSoundDevice::~OpenSoundDevice()
{
	CALLED();
	OpenSoundDeviceEngine *engine;
	OpenSoundDeviceMixer *mixer;
	while ((engine = EngineAt(0))) {
		delete engine;
		fEngines.RemoveItems(0, 1);
	}
	while ((mixer = MixerAt(0))) {
		delete mixer;
		fMixers.RemoveItems(0, 1);
	}
}

OpenSoundDevice::OpenSoundDevice(oss_card_info *cardinfo)
	: fLocker("OpenSoundDevice")
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	memcpy(&fCardInfo, cardinfo, sizeof(oss_card_info));
	memset(&fFragments, 0, sizeof(fFragments));
#if 0
	strcpy(fDevice_name, name);
	strcpy(fDevice_path, path);

	PRINT(("name : %s, path : %s\n", fDevice_name, fDevice_path));

	if (InitDriver() != B_OK)
		return;

	fInitCheckStatus = B_OK;
#endif
}


status_t OpenSoundDevice::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


status_t OpenSoundDevice::InitDriver()
{

	CALLED();
	PRINT(("OpenSoundDevice::InitDriver: %" B_PRId32 " engines, %" B_PRId32
		" mixers\n", CountEngines(), CountMixers()));
	
	if (CountMixers()) {
		;//...
	}
	
	fInitCheckStatus = B_OK;
	
	return B_OK;
}


status_t
OpenSoundDevice::AddEngine(oss_audioinfo *info)
{
	status_t err;
	/* discard shadow/hidden engines (!?) */
	CALLED();
/*
	if (info->caps & PCM_CAP_SHADOW)
		return B_OK;
	if (info->caps & PCM_CAP_HIDDEN)
		return B_OK;
*/
	OpenSoundDeviceEngine *engine =
		new(std::nothrow) OpenSoundDeviceEngine(info);
	if (!engine)
		return ENOMEM;
	err = engine->InitCheck();
	if (err < B_OK) {
		delete engine;
		return err;
	}
	fEngines.AddItem(engine);	
	return B_OK;
}


status_t
OpenSoundDevice::AddMixer(oss_mixerinfo *info)
{
	status_t err;
	CALLED();
	OpenSoundDeviceMixer *mixer =
		new(std::nothrow) OpenSoundDeviceMixer(info);
	if (!mixer)
		return ENOMEM;
	err = mixer->InitCheck();
	if (err < B_OK) {
		delete mixer;
		return err;
	}
	fMixers.AddItem(mixer);	
	return B_OK;
}


int32
OpenSoundDevice::CountEngines()
{
	return fEngines.CountItems();
}


int32
OpenSoundDevice::CountMixers()
{
	return fMixers.CountItems();
}


OpenSoundDeviceEngine *
OpenSoundDevice::EngineAt(int32 i)
{
	return (OpenSoundDeviceEngine *)(fEngines.ItemAt(i));
}


OpenSoundDeviceMixer *
OpenSoundDevice::MixerAt(int32 i)
{
	return (OpenSoundDeviceMixer *)(fMixers.ItemAt(i));
}

OpenSoundDeviceEngine *
OpenSoundDevice::NextFreeEngineAt(int32 i, bool rec)
{
	// find the first free engine in the rec or play chain
	OpenSoundDeviceEngine *engine = EngineAt(i);
	while (engine && engine->InUse()) {
		engine = rec ? (engine->NextRec()) : (engine->NextPlay());
	}
	return engine;
}

