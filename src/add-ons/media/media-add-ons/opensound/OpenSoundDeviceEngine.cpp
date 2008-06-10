/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 */

#include "OpenSoundDeviceEngine.h"
#include "debug.h"
#include "driver_io.h"
#include <MediaDefs.h>
#include <Debug.h>
#include <errno.h>
#include <string.h>

OpenSoundDeviceEngine::~OpenSoundDeviceEngine()
{
	CALLED();
	if (fFD != 0) {
		close(fFD);
	}
}

OpenSoundDeviceEngine::OpenSoundDeviceEngine(oss_audioinfo *info)
	: fNextPlay(NULL)
	, fNextRec(NULL)
	, fOpenMode(0)
	, fFD(-1)
	, fMediaFormat()
	, fPlayedFramesCount(0LL)
	, fPlayedRealTime(0LL)
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	memcpy(&fAudioInfo, info, sizeof(oss_audioinfo));
	
	// XXX:REMOVEME
	// set default format
/*
	SetFormat(OpenSoundDevice::select_oss_format(info->oformats));
	SetChannels(info->max_channels);
	SetSpeed(info->max_rate);
*/
	fInitCheckStatus = B_OK;
}


status_t OpenSoundDeviceEngine::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}


status_t OpenSoundDeviceEngine::Open(int mode)
{
	int omode, v;
	CALLED();

	switch (mode) {
	case OPEN_READ:
		if (!(Caps() & DSP_CAP_INPUT))
			return EINVAL;
		omode = O_RDONLY;
		break;
	case OPEN_WRITE:
		if (!(Caps() & DSP_CAP_OUTPUT))
			return EINVAL;
		omode = O_WRONLY;
		break;
	case OPEN_READWRITE:
		if (!(Caps() & DSP_CAP_OUTPUT) || !(Caps() & DSP_CAP_INPUT))
			return EINVAL;
		omode = O_RDWR;
		break;
	default:
		return EINVAL;
	}
	// O_EXCL = bypass soft mixer = direct access
	omode |= O_EXCL;

	Close();
	fOpenMode = mode;
	fFD = open(fAudioInfo.devnode, omode);
	if (fFD < 0) {
		fInitCheckStatus = errno;
		return EIO;
	}
	// disable format convertions
	v = 0;
	if (ioctl(fFD, SNDCTL_DSP_COOKEDMODE, &v, sizeof(int)) < 0) {
		fInitCheckStatus = errno;
		Close();
		return EIO;
	}
#if 0
	// set fragments
	v = 0x7fff0000 | 0x000b; // unlimited * 2048
	if (ioctl(fFD, SNDCTL_DSP_SETFRAGMENT, &v, sizeof(int)) < 0) {
		fInitCheckStatus = errno;
		Close();
		return EIO;
	}
#endif

#if 0
	// set latency policy = fragment size
	// XXX: BParameter?
	v = 0;
	if (ioctl(fFD, SNDCTL_DSP_POLICY, &v, sizeof(int)) < 0) {
		fInitCheckStatus = errno;
		Close();
		return EIO;
	}
#endif

	fPlayedFramesCount = 0LL;
	fPlayedRealTime = system_time();
	return B_OK;
}


status_t OpenSoundDeviceEngine::Close(void)
{
	CALLED();
	if (fFD > -1)
		close(fFD);
	fFD = -1;
	fOpenMode = 0;
	fMediaFormat = media_format();
	fPlayedFramesCount = 0LL;
	fPlayedRealTime = 0LL;
	return B_OK;
}


ssize_t OpenSoundDeviceEngine::Read(void *buffer, size_t size)
{
	ssize_t done;
	CALLED();
	done = read(fFD, buffer, size);
	if (done < 0)
		return errno;
	return done;
}


ssize_t OpenSoundDeviceEngine::Write(const void *buffer, size_t size)
{
	ssize_t done;
	int v;
	CALLED();
	ASSERT(size > 0);
	done = write(fFD, buffer, size);
	if (done < 0)
		return errno;
	switch (fMediaFormat.type) {
	case B_MEDIA_RAW_AUDIO:
		fPlayedFramesCount += done / (fMediaFormat.u.raw_audio.channel_count
							 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK));
/*		fPlayedRealTime = system_time();
		v = 0;
		if (ioctl(fFD, SNDCTL_DSP_GETODELAY, &v, sizeof(int)) > -1) {
			bigtime_t delay = (bigtime_t)v * 1000000LL
								/ (fMediaFormat.u.raw_audio.channel_count * fMediaFormat.u.raw_audio.frame_rate
							 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK));
			fPlayedRealTime += delay;
			PRINT(("********************************* v = %d, delay %Ld\n", v, delay));
		}*/
//		PRINT(("OpenSoundDeviceEngine::%s: wrote %d, played %Ld frames"/*", realtime %Ld"*/"\n", __FUNCTION__, done, fPlayedFramesCount/*, fPlayedRealTime*/));
		break;
	case B_MEDIA_ENCODED_AUDIO:
		//XXX: WRITEME! -- bitrate ?
		break;
	default:
		return EINVAL;
	}
	return done;
}


status_t OpenSoundDeviceEngine::UpdateInfo(void)
{
	status_t err;
	CALLED();
	if (fFD < 0)
		return ENODEV;
	
	if (ioctl(fFD, SNDCTL_ENGINEINFO, &fAudioInfo, sizeof(oss_audioinfo)) < 0) {
		return errno;
	}
	return B_OK;
}


bigtime_t OpenSoundDeviceEngine::PlaybackLatency(void)
{
	bigtime_t latency;
	int delay;
	delay = GetODelay();
	delay = 0; //XXX 
	latency = ((double)delay * 1000000LL 
			/ (fMediaFormat.u.raw_audio.channel_count * fMediaFormat.u.raw_audio.frame_rate
			* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK)));
	PRINT(("PlaybackLatency: odelay %d latency %Ld card %Ld\n", delay, latency, CardLatency()));
	latency += CardLatency();
	return latency;
}


bigtime_t OpenSoundDeviceEngine::RecordingLatency(void)
{
	return 0LL; //XXX
}


int OpenSoundDeviceEngine::GetChannels(void)
{
	int chans = -1;
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_CHANNELS, &chans, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_CHANNELS", strerror(errno)));
		return -1;
	}
	return chans;
}

status_t OpenSoundDeviceEngine::SetChannels(int chans)
{
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_CHANNELS, &chans, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_CHANNELS", strerror(errno)));
		return EIO;
	}
	PRINT(("OpenSoundDeviceEngine::%s: %d\n", __FUNCTION__, chans));
	return B_OK;
}

//XXX: either GetFormat*s*() or cache SetFormat()!
int OpenSoundDeviceEngine::GetFormat(void)
{
	int fmt = -1;
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_GETFMTS, &fmt, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GETFMTS", strerror(errno)));
		return -1;
	}
	return fmt;
}

status_t OpenSoundDeviceEngine::SetFormat(int fmt)
{
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_SETFMT, &fmt, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_SETFMT", strerror(errno)));
		return EIO;
	}
	PRINT(("OpenSoundDeviceEngine::%s: 0x%08x\n", __FUNCTION__, fmt));
	return B_OK;
}

int OpenSoundDeviceEngine::GetSpeed(void)
{
	int speed = -1;
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_SPEED, &speed, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_SPEED", strerror(errno)));
		return -1;
	}
	return speed;
}

status_t OpenSoundDeviceEngine::SetSpeed(int speed)
{
	CALLED();
	if (ioctl(fFD, SNDCTL_DSP_SPEED, &speed, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_SPEED", strerror(errno)));
		return EIO;
	}
	PRINT(("OpenSoundDeviceEngine::%s: %d\n", __FUNCTION__, speed));
	return B_OK;
}


size_t OpenSoundDeviceEngine::GetISpace(audio_buf_info *info)
{
	audio_buf_info abinfo;
	CALLED();
	if (!info)
		info = &abinfo;
	memset(info, 0, sizeof(audio_buf_info));
	if (!(fOpenMode & OPEN_READ))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_GETISPACE, info, sizeof(audio_buf_info)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GETISPACE", strerror(errno)));
		return EIO;
	}
	//PRINT(("OpenSoundDeviceEngine::%s: ISPACE: { bytes=%d, fragments=%d, fragsize=%d, fragstotal=%d }\n", __FUNCTION__, info->bytes, info->fragments, info->fragsize, info->fragstotal));
	return info->bytes;
}


size_t OpenSoundDeviceEngine::GetOSpace(audio_buf_info *info)
{
	audio_buf_info abinfo;
	//CALLED();
	if (!info)
		info = &abinfo;
	memset(info, 0, sizeof(audio_buf_info));
	if (!(fOpenMode & OPEN_WRITE))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_GETOSPACE, info, sizeof(audio_buf_info)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GETOSPACE", strerror(errno)));
		return EIO;
	}
	//PRINT(("OpenSoundDeviceEngine::%s: OSPACE: { bytes=%d, fragments=%d, fragsize=%d, fragstotal=%d }\n", __FUNCTION__, info->bytes, info->fragments, info->fragsize, info->fragstotal));
	return info->bytes;
}


int64
OpenSoundDeviceEngine::GetCurrentIPtr(int32 *fifoed, oss_count_t *info)
{
	oss_count_t ocount;
	count_info cinfo;
	CALLED();
	if (!info)
		info = &ocount;
	memset(info, 0, sizeof(oss_count_t));
	if (!(fOpenMode & OPEN_READ))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_CURRENT_IPTR, info, sizeof(oss_count_t)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_CURRENT_IPTR", strerror(errno)));
		//return EIO;
		// fallback: try GET*PTR
		if (ioctl(fFD, SNDCTL_DSP_GETIPTR, &cinfo, sizeof(count_info)) < 0) {
			PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
					__FUNCTION__, "SNDCTL_DSP_GETIPTR", strerror(errno)));
			return 0;
		}
		// it's probably wrong...
		info->samples = cinfo.bytes / (fMediaFormat.u.raw_audio.channel_count
						 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK));
		info->fifo_samples = 0;
	}
	PRINT(("OpenSoundDeviceEngine::%s: IPTR: { samples=%Ld, fifo_samples=%d }\n", __FUNCTION__, info->samples, info->fifo_samples));
	if (fifoed)
		*fifoed = info->fifo_samples;
	return info->samples;
}


int64
OpenSoundDeviceEngine::GetCurrentOPtr(int32 *fifoed, oss_count_t *info)
{
	oss_count_t ocount;
	count_info cinfo;
	CALLED();
	if (!info)
		info = &ocount;
	memset(info, 0, sizeof(oss_count_t));
	if (!(fOpenMode & OPEN_WRITE))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_CURRENT_OPTR, info, sizeof(oss_count_t)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_CURRENT_OPTR", strerror(errno)));
		//return EIO;
		// fallback: try GET*PTR
		if (ioctl(fFD, SNDCTL_DSP_GETOPTR, &cinfo, sizeof(count_info)) < 0) {
			PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
					__FUNCTION__, "SNDCTL_DSP_GETOPTR", strerror(errno)));
			return 0;
		}
		// it's probably wrong...
		info->samples = cinfo.bytes / (fMediaFormat.u.raw_audio.channel_count
						 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK));
		info->fifo_samples = 0;
	}
	//PRINT(("OpenSoundDeviceEngine::%s: OPTR: { samples=%Ld, fifo_samples=%d }\n", __FUNCTION__, info->samples, info->fifo_samples));
	if (fifoed)
		*fifoed = info->fifo_samples;
	return info->samples;
}


int32
OpenSoundDeviceEngine::GetIOverruns()
{
	audio_errinfo info;
	CALLED();
	memset(&info, 0, sizeof(info));
	if (!(fOpenMode & OPEN_WRITE))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_GETERROR, &info, sizeof(info)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GETERROR", strerror(errno)));
		return 0;
	}
	PRINT(("OpenSoundDeviceEngine::%s: IOVERRUNS: { overruns=%d }\n", __FUNCTION__, info.rec_overruns));
	return info.rec_overruns;
}


int32
OpenSoundDeviceEngine::GetOUnderruns()
{
	audio_errinfo info;
	CALLED();
	memset(&info, 0, sizeof(info));
	if (!(fOpenMode & OPEN_WRITE))
		return 0;
	if (ioctl(fFD, SNDCTL_DSP_GETERROR, &info, sizeof(info)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GETERROR", strerror(errno)));
		return 0;
	}
	//PRINT(("OpenSoundDeviceEngine::%s: OUNDERRUNS: { underruns=%d }\n", __FUNCTION__, info.play_underruns));
	return info.play_underruns;
}


int OpenSoundDeviceEngine::GetODelay(void)
{
	//CALLED();
	int v = 1;
	if (ioctl(fFD, SNDCTL_DSP_GETODELAY, &v, sizeof(int)) < 0) {
		return 0;
	}
	return v;
}


status_t OpenSoundDeviceEngine::StartRecording(void)
{
	CALLED();
	oss_syncgroup group;
	group.id = 0;
	group.mode = PCM_ENABLE_INPUT;
	if (ioctl(fFD, SNDCTL_DSP_SYNCGROUP, &group, sizeof(group)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_SYNCGROUP", strerror(errno)));
		return EIO;
	}
	if (ioctl(fFD, SNDCTL_DSP_SYNCSTART, &group.id, sizeof(group.id)) < 0) {
		PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_SYNCSTART", strerror(errno)));
		return EIO;
	}
	return B_OK;
}


int64 OpenSoundDeviceEngine::PlayedFramesCount(void)
{
	int64 played;
	int32 fifoed;
	played = GetCurrentOPtr(&fifoed);
	//played += fifoed;
	//return played;
	fPlayedFramesCount = played - fifoed;
	return fPlayedFramesCount;//XXX
	return fPlayedFramesCount - (GetODelay() / (fMediaFormat.u.raw_audio.channel_count
						 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK)));
	return fPlayedFramesCount - (GetODelay() / (/*fMediaFormat.u.raw_audio.channel_count
						 		* */(fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK)));
	//return fPlayedFramesCount;
}


bigtime_t OpenSoundDeviceEngine::PlayedRealTime(void)
{
	//CALLED();
	bigtime_t playedRealTime = system_time();
	return playedRealTime;//XXX
	int v = 1;
	if (ioctl(fFD, SNDCTL_DSP_GETODELAY, &v, sizeof(int)) < 0) {
		return playedRealTime;
	}
	bigtime_t delay = (bigtime_t)(v * 1000000LL
						/ (fMediaFormat.u.raw_audio.channel_count * fMediaFormat.u.raw_audio.frame_rate
					 		* (fMediaFormat.AudioFormat() & media_raw_audio_format::B_AUDIO_SIZE_MASK)));
	playedRealTime += delay;
	//PRINT(("********************************* v = %d, delay %Ld\n", v, delay));
//	playedRealTime-=41000;
	return playedRealTime;
}


status_t OpenSoundDeviceEngine::WildcardFormatFor(int fmt, media_format &format, bool rec)
{
	status_t err;
	CALLED();
	fmt &= rec ? Info()->iformats : Info()->oformats;
	if (fmt == 0)
		return B_MEDIA_BAD_FORMAT;
	err = OpenSoundDevice::get_media_format_for(fmt, format);
	if (err < B_OK)
		return err;
	char buf[1024];
	string_for_format(format, buf, 1024);
	if (format.type == B_MEDIA_RAW_AUDIO) {
		format.u.raw_audio = media_multi_audio_format::wildcard;
		format.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
		// single rate supported
		if (Info()->min_rate == Info()->max_rate)
			format.u.raw_audio.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
	} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
		format.u.encoded_audio.output = media_multi_audio_format::wildcard;
		//format.u.encoded_audio.output.byte_order = B_MEDIA_HOST_ENDIAN;
		// single rate supported
		//if (Info()->min_rate == Info()->max_rate)
		//	format.u.encoded_audio.output.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
	} else
		return EINVAL;
	PRINT(("%s: %s\n", __FUNCTION__, buf));
	return B_OK;
}


status_t OpenSoundDeviceEngine::PreferredFormatFor(int fmt, media_format &format, bool rec)
{
	status_t err;
	CALLED();
	fmt &= rec ? Info()->iformats : Info()->oformats;
	if (fmt == 0)
		return B_MEDIA_BAD_FORMAT;
	err = WildcardFormatFor(fmt, format);
	if (err < B_OK)
		return err;
	if (format.type == B_MEDIA_RAW_AUDIO) {
		media_multi_audio_format &raw = format.u.raw_audio;
		//format.u.raw_audio.channel_count = Info()->max_channels;
		raw.byte_order = B_MEDIA_HOST_ENDIAN;
		raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		raw.buffer_size = DEFAULT_BUFFER_SIZE;
		/*if (rec)
			raw.buffer_size = 2048;*/
/*
		format.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
		format.u.raw_audio.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		format.u.raw_audio.buffer_size = DEFAULT_BUFFER_SIZE;
*/
	} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
		media_raw_audio_format &raw = format.u.encoded_audio.output;
		//format.u.encoded_audio.output.channel_count = Info()->max_channels;
		raw.byte_order = B_MEDIA_HOST_ENDIAN;
		// single rate supported
		if (Info()->min_rate == Info()->max_rate)
			raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		raw.buffer_size = DEFAULT_BUFFER_SIZE;
	} else
		return EINVAL;
	char buf[1024];
	string_for_format(format, buf, 1024);
	PRINT(("%s: %s\n", __FUNCTION__, buf));
	return B_OK;
}


status_t OpenSoundDeviceEngine::AcceptFormatFor(int fmt, media_format &format, bool rec)
{
	status_t err;
	int afmt = 0;
	char buf[1024];
	CALLED();
	fmt &= rec ? Info()->iformats : Info()->oformats;

	if (fmt == 0)
		return B_MEDIA_BAD_FORMAT;
	media_format wc;
	err = WildcardFormatFor(fmt, wc);
	if (err < B_OK)
		return err;
	
	err = Open(rec ? OPEN_READ : OPEN_WRITE);
	if (err < B_OK)
		return err;
	
	if (format.type == B_MEDIA_RAW_AUDIO) {
		media_multi_audio_format &raw = format.u.raw_audio;
		
		// channel count
		raw.channel_count = MAX((unsigned)(Info()->min_channels), MIN((unsigned)(Info()->max_channels), raw.channel_count));
		err = SetChannels(raw.channel_count);
		if (err < B_OK) {
			Close();
			return err;
		}

		PRINT(("%s:step1  fmt=0x%08x, raw.format=0x%08lx\n", __FUNCTION__, fmt, raw.format));
		// if specified, try it
		if (raw.format)
			afmt = OpenSoundDevice::convert_media_format_to_oss_format(raw.format);
		afmt &= fmt;
		PRINT(("%s:step2 afmt=0x%08x\n", __FUNCTION__, afmt));
		// select the best as default
		if (afmt == 0) {
			afmt = OpenSoundDevice::select_oss_format(fmt);
			raw.format = OpenSoundDevice::convert_oss_format_to_media_format(afmt);
			//Close();
			//return B_MEDIA_BAD_FORMAT;
		}
		PRINT(("%s:step3 afmt=0x%08x\n", __FUNCTION__, afmt));
		// convert back
		raw.format = OpenSoundDevice::convert_oss_format_to_media_format(afmt);
		PRINT(("%s:step4 afmt=0x%08x, raw.format=0x%08lx\n", __FUNCTION__, afmt, raw.format));
		raw.valid_bits = OpenSoundDevice::convert_oss_format_to_valid_bits(afmt);

		err = SetFormat(afmt);
		if (err < B_OK) {
			Close();
			return err;
		}
		
		// endianness
		raw.byte_order = OpenSoundDevice::convert_oss_format_to_endian(afmt);
		
		// sample rate
		raw.frame_rate = OpenSoundDevice::select_oss_rate(Info(), raw.frame_rate);		// measured in Hertz
		//raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		err = SetSpeed(OpenSoundDevice::convert_media_rate_to_oss_rate(raw.frame_rate));
		if (err < B_OK) {
			Close();
			return err;
		}
		
		
		
#if 0
		raw.buffer_size = DEFAULT_BUFFER_SIZE
						* (raw.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* raw.channel_count;
#endif
		audio_buf_info abinfo;
		if (ioctl(fFD, rec?SNDCTL_DSP_GETISPACE:SNDCTL_DSP_GETOSPACE, &abinfo, sizeof(abinfo)) < 0) {
			PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GET?SPACE", strerror(errno)));
			return -1;
		}
		PRINT(("OSS: %cSPACE: { bytes=%d, fragments=%d, fragsize=%d, fragstotal=%d }\n", rec?'I':'O', abinfo.bytes, abinfo.fragments, abinfo.fragsize, abinfo.fragstotal));
		// cache the first one in the Device
		// so StartThread() knows the number of frags
		//if (!fFragments.fragstotal)
		//	memcpy(&fFragments, &abinfo, sizeof(abinfo));

		// make sure buffer size is less than the driver's own buffer ( /2 to keep some margin )
//		if (/*rec && raw.buffer_size &&*/ raw.buffer_size >= abinfo.fragsize * abinfo.fragstotal / 2)
//			return B_MEDIA_BAD_FORMAT;
//		if (!raw.buffer_size)
		raw.buffer_size = abinfo.fragsize;// * abinfo.fragstotal / 4;//XXX
/*						* (raw.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* raw.channel_count;*/
	} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
		media_raw_audio_format &raw = format.u.encoded_audio.output;
		// XXX: do we really have to do this ?
		raw.channel_count = MAX((unsigned)(Info()->min_channels), MIN((unsigned)(Info()->max_channels), raw.channel_count));
		raw.byte_order = B_MEDIA_HOST_ENDIAN;
		raw.frame_rate = OpenSoundDevice::select_oss_rate(Info(), raw.frame_rate);		// measured in Hertz
		//raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		raw.buffer_size = DEFAULT_BUFFER_SIZE;

	} else {
		PRINT(("%s: unknown media type\n", __FUNCTION__));
		Close();
		return EINVAL;
	}
	// cache it
	fMediaFormat = format;
	string_for_format(format, buf, 1024);
	PRINT(("%s: %s\n", __FUNCTION__, buf));
	return B_OK;
}


status_t OpenSoundDeviceEngine::SpecializeFormatFor(int fmt, media_format &format, bool rec)
{
	status_t err;
	int afmt = 0;
	CALLED();
	fmt &= rec ? Info()->iformats : Info()->oformats;
	if (fmt == 0)
		return B_MEDIA_BAD_FORMAT;
	media_format wc;
	err = WildcardFormatFor(fmt, wc);
	if (err < B_OK)
		return err;
	
	err = Open(rec ? OPEN_READ : OPEN_WRITE);
	if (err < B_OK)
		return err;
	
	if (format.type == B_MEDIA_RAW_AUDIO) {
		media_multi_audio_format &raw = format.u.raw_audio;
		
		PRINT(("%s:step1  fmt=0x%08x, raw.format=0x%08lx\n", __FUNCTION__, fmt, raw.format));
		// select the best as default
		if (!raw.format) {
			afmt = OpenSoundDevice::select_oss_format(fmt);
			raw.format = OpenSoundDevice::convert_oss_format_to_media_format(afmt);
		}
		// if specified, try it
		if (raw.format)
			afmt = OpenSoundDevice::convert_media_format_to_oss_format(raw.format);
		afmt &= fmt;
		PRINT(("%s:step2 afmt=0x%08x\n", __FUNCTION__, afmt));
		if (afmt == 0) {
			Close();
			return B_MEDIA_BAD_FORMAT;
		}
		// convert back
		raw.format = OpenSoundDevice::convert_oss_format_to_media_format(afmt);
		PRINT(("%s:step4 afmt=0x%08x, raw.format=0x%08lx\n", __FUNCTION__, afmt, raw.format));
		if (!raw.valid_bits)
			raw.valid_bits = OpenSoundDevice::convert_oss_format_to_valid_bits(afmt);
		if (raw.valid_bits != OpenSoundDevice::convert_oss_format_to_valid_bits(afmt)) {
			Close();
			return B_MEDIA_BAD_FORMAT;
		}

		err = SetFormat(afmt);
		if (err < B_OK) {
			Close();
			return err;
		}
		
		// endianness
		if (!raw.byte_order)
			raw.byte_order = OpenSoundDevice::convert_oss_format_to_endian(afmt);
		if (raw.byte_order != OpenSoundDevice::convert_oss_format_to_endian(afmt)) {
			Close();
			return B_MEDIA_BAD_FORMAT;
		}
		
		// channel count
		if (!raw.channel_count)
			raw.channel_count = (unsigned)Info()->min_channels;
		if (raw.channel_count < Info()->min_channels || raw.channel_count > Info()->max_channels)
			return B_MEDIA_BAD_FORMAT;
		err = SetChannels(raw.channel_count);
		if (err < B_OK) {
			Close();
			return err;
		}

		// sample rate
		if (!raw.frame_rate)
			raw.frame_rate = Info()->max_rate;
		//raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		err = SetSpeed(OpenSoundDevice::convert_media_rate_to_oss_rate(raw.frame_rate));
		if (err < B_OK) {
			Close();
			return err;
		}
		
#if 0
		raw.buffer_size = DEFAULT_BUFFER_SIZE
						* (raw.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* raw.channel_count;
#endif
		audio_buf_info abinfo;
		if (ioctl(fFD, rec?SNDCTL_DSP_GETISPACE:SNDCTL_DSP_GETOSPACE, &abinfo, sizeof(abinfo)) < 0) {
			PRINT(("OpenSoundDeviceEngine::%s: %s: %s\n", 
				__FUNCTION__, "SNDCTL_DSP_GET?SPACE", strerror(errno)));
			return -1;
		}
		PRINT(("OSS: %cSPACE: { bytes=%d, fragments=%d, fragsize=%d, fragstotal=%d }\n", rec?'I':'O', abinfo.bytes, abinfo.fragments, abinfo.fragsize, abinfo.fragstotal));
		// cache the first one in the Device
		// so StartThread() knows the number of frags
		//if (!fFragments.fragstotal)
		//	memcpy(&fFragments, &abinfo, sizeof(abinfo));

		// make sure buffer size is less than the driver's own buffer ( /2 to keep some margin )
		if (/*rec && raw.buffer_size &&*/ raw.buffer_size > abinfo.fragsize * abinfo.fragstotal / 4)
			return B_MEDIA_BAD_FORMAT;
		if (!raw.buffer_size)
			raw.buffer_size = abinfo.fragsize;//XXX
/*						* (raw.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
						* raw.channel_count;*/
	} else if (format.type == B_MEDIA_ENCODED_AUDIO) {
		media_raw_audio_format &raw = format.u.encoded_audio.output;
		// XXX: do we really have to do this ?
		raw.channel_count = MAX((unsigned)(Info()->min_channels), MIN((unsigned)(Info()->max_channels), raw.channel_count));
		raw.byte_order = B_MEDIA_HOST_ENDIAN;
		raw.frame_rate = OpenSoundDevice::select_oss_rate(Info(), raw.frame_rate);		// measured in Hertz
		//raw.frame_rate = OpenSoundDevice::convert_oss_rate_to_media_rate(Info()->max_rate);		// measured in Hertz
		raw.buffer_size = DEFAULT_BUFFER_SIZE;

	} else {
		Close();
		return EINVAL;
	}
	// cache it
	fMediaFormat = format;
	char buf[1024];
	string_for_format(format, buf, 1024);
	PRINT(("%s: %s\n", __FUNCTION__, buf));
	return B_OK;
}

