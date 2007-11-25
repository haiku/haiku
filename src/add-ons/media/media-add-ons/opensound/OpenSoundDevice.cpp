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

const char *gSupportedFormatsNames[] = {
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
	//highest rate
	return info->max_rate;
	//XXX:FIXME: use min/max + array
	return rate;
}


int OpenSoundDevice::select_oss_format(int fmt)
{
	//highest format, native endian first
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
	PRINT(("OpenSoundDevice::InitDriver: %d engines, %d mixers\n", CountEngines(), CountMixers()));
	
	if (CountMixers()) {
		;//...
	}
	
	fInitCheckStatus = B_OK;
	
	return B_OK;

#if MA
	multi_channel_enable 	MCE;
	uint32					mce_enable_bits;
	int rval, i, num_outputs, num_inputs, num_channels;

	CALLED();

	//open the device driver for output
	fd = open(fDevice_path, O_WRONLY);

	if (fd == -1) {
		fprintf(stderr, "Failed to open %s: %s\n", fDevice_path, strerror(errno));
		return B_ERROR;
	}

	//
	// Get description
	//
	MD.info_size = sizeof(MD);
	MD.request_channel_count = MAX_CHANNELS;
	MD.channels = MCI;
	rval = DRIVER_GET_DESCRIPTION(&MD, 0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_DESCRIPTION\n");
		return B_ERROR;
	}

	PRINT(("Friendly name:\t%s\nVendor:\t\t%s\n",
		MD.friendly_name, MD.vendor_info));
	PRINT(("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
		MD.output_channel_count, MD.input_channel_count,
		MD.output_bus_channel_count, MD.input_bus_channel_count));
	PRINT(("\nChannels\n"
		"ID\tKind\tDesig\tConnectors\n"));

	for (i = 0 ; i < (MD.output_channel_count + MD.input_channel_count); i++)
	{
		PRINT(("%ld\t%d\t0x%lx\t0x%lx\n", MD.channels[i].channel_id,
			MD.channels[i].kind,
			MD.channels[i].designations,
			MD.channels[i].connectors));
	}
	PRINT(("\n"));

	PRINT(("Output rates\t\t0x%lx\n", MD.output_rates));
	PRINT(("Input rates\t\t0x%lx\n", MD.input_rates));
	PRINT(("Max CVSR\t\t%.0f\n", MD.max_cvsr_rate));
	PRINT(("Min CVSR\t\t%.0f\n", MD.min_cvsr_rate));
	PRINT(("Output formats\t\t0x%lx\n", MD.output_formats));
	PRINT(("Input formats\t\t0x%lx\n", MD.input_formats));
	PRINT(("Lock sources\t\t0x%lx\n", MD.lock_sources));
	PRINT(("Timecode sources\t0x%lx\n", MD.timecode_sources));
	PRINT(("Interface flags\t\t0x%lx\n", MD.interface_flags));
	PRINT(("Control panel string:\t\t%s\n", MD.control_panel));
	PRINT(("\n"));

	num_outputs = MD.output_channel_count;
	num_inputs = MD.input_channel_count;
	num_channels = num_outputs + num_inputs;

	// Get and set enabled channels
	MCE.info_size = sizeof(MCE);
	MCE.enable_bits = (uchar *) & mce_enable_bits;
	rval = DRIVER_GET_ENABLED_CHANNELS(&MCE, sizeof(MCE));
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_ENABLED_CHANNELS len is 0x%lx\n", sizeof(MCE));
		return B_ERROR;
	}

	mce_enable_bits = (1 << num_channels) - 1;
	MCE.lock_source = B_MULTI_LOCK_INTERNAL;
	rval = DRIVER_SET_ENABLED_CHANNELS(&MCE, 0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_SET_ENABLED_CHANNELS 0x%p 0x%x\n", MCE.enable_bits, *(MCE.enable_bits));
		return B_ERROR;
	}

	//
	// Set the sample rate
	//
	MFI.info_size = sizeof(MFI);
	MFI.output.rate = select_oss_rate(MD.output_rates);
	MFI.output.cvsr = 0;
	MFI.output.format = select_oss_format(MD.output_formats);
	MFI.input.rate = MFI.output.rate;
	MFI.input.cvsr = MFI.output.cvsr;
	MFI.input.format = MFI.output.format;
	rval = DRIVER_SET_GLOBAL_FORMAT(&MFI, 0);
	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_SET_GLOBAL_FORMAT\n");
		return B_ERROR;
	}

	//
	// Get the buffers
	//
	for (i = 0; i < NB_BUFFERS; i++) {
		play_buffer_desc[i] = &play_buffer_list[i * MAX_CHANNELS];
		record_buffer_desc[i] = &record_buffer_list[i * MAX_CHANNELS];
	}
	MBL.info_size = sizeof(MBL);
	MBL.request_playback_buffer_size = 0;           //use the default......
	MBL.request_playback_buffers = NB_BUFFERS;
	MBL.request_playback_channels = num_outputs;
	MBL.playback_buffers = (buffer_desc **) play_buffer_desc;
	MBL.request_record_buffer_size = 0;           //use the default......
	MBL.request_record_buffers = NB_BUFFERS;
	MBL.request_record_channels = num_inputs;
	MBL.record_buffers = (buffer_desc **) record_buffer_desc;

	rval = DRIVER_GET_BUFFERS(&MBL, 0);

	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on B_MULTI_GET_BUFFERS\n");
		return B_ERROR;
	}

	for (i = 0; i < MBL.return_playback_buffers; i++)
		for (int j = 0; j < MBL.return_playback_channels; j++) {
			PRINT(("MBL.playback_buffers[%d][%d].base: %p\n",
				i, j, MBL.playback_buffers[i][j].base));
			PRINT(("MBL.playback_buffers[%d][%d].stride: %i\n",
				i, j, MBL.playback_buffers[i][j].stride));
		}

	for (i = 0; i < MBL.return_record_buffers; i++)
		for (int j = 0; j < MBL.return_record_channels; j++) {
			PRINT(("MBL.record_buffers[%d][%d].base: %p\n",
				i, j, MBL.record_buffers[i][j].base));
			PRINT(("MBL.record_buffers[%d][%d].stride: %i\n",
				i, j, MBL.record_buffers[i][j].stride));
		}

	MMCI.info_size = sizeof(MMCI);
	MMCI.control_count = MAX_CONTROLS;
	MMCI.controls = MMC;

	rval = DRIVER_LIST_MIX_CONTROLS(&MMCI, 0);

	if (B_OK != rval)
	{
		fprintf(stderr, "Failed on DRIVER_LIST_MIX_CONTROLS\n");
		return B_ERROR;
	}

	return B_OK;
#endif
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
	OpenSoundDeviceEngine *engine = new OpenSoundDeviceEngine(info);
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
	OpenSoundDeviceMixer *mixer = new OpenSoundDeviceMixer(info);
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


#if MA
int
OpenSoundDevice::DoBufferExchange(multi_buffer_info *MBI)
{
	return DRIVER_BUFFER_EXCHANGE(MBI, 0);
}

int
OpenSoundDevice::DoSetMix(multi_mix_value_info *MMVI)
{
	return DRIVER_SET_MIX(MMVI, 0);
}

int
OpenSoundDevice::DoGetMix(multi_mix_value_info *MMVI)
{
	return DRIVER_GET_MIX(MMVI, 0);
}
#endif
