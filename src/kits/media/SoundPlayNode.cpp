/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SoundPlayNode.cpp
 *  DESCR: This is the BBufferProducer, used internally by BSoundPlayer
 *         This belongs into a private namespace, but isn't for 
 *         compatibility reasons.
 ***********************************************************************/

//#include <ByteOrder.h>
#include <BufferProducer.h>
#include <SoundPlayer.h>
#include <string.h>
#include "SoundPlayNode.h"
#include "debug.h"
#include "ChannelMixer.h"
#include "SampleConverter.h"
#include "SamplingrateConverter.h"

#include <stdlib.h>
#include <unistd.h>
#include <Drivers.h> 

int driver;
sem_id sem;

enum { 
	SOUND_GET_PARAMS = B_DEVICE_OP_CODES_END, 
	SOUND_SET_PARAMS, 
	SOUND_SET_PLAYBACK_COMPLETION_SEM, 
	SOUND_SET_CAPTURE_COMPLETION_SEM, 
	SOUND_RESERVED_1,	 /* unused */ 
	SOUND_RESERVED_2,	 /* unused */ 
	SOUND_DEBUG_ON,		 /* unused */ 
	SOUND_DEBUG_OFF,		/* unused */ 
	SOUND_WRITE_BUFFER, 
	SOUND_READ_BUFFER, 
	SOUND_LOCK_FOR_DMA
};

typedef struct audio_buffer_header { 
	int32 buffer_number; 
	int32 subscriber_count; 
	bigtime_t time; 
	int32 reserved_1; 
	int32 reserved_2; 
	int32 reserved_3; 
	int32 reserved_4; 
} audio_buffer_header;

enum adc_source { 
  line = 0, cd, mic, loopback 
}; 

enum sample_rate { 
  kHz_8_0 = 0, kHz_5_51, kHz_16_0, kHz_11_025, kHz_27_42, 
  kHz_18_9, kHz_32_0, kHz_22_05, kHz_37_8 = 9, 
  kHz_44_1 = 11, kHz_48_0, kHz_33_075, kHz_9_6, kHz_6_62 
}; 

enum sample_format {};  /* obsolete */ 

struct channel { 
  enum adc_source adc_source; 
  char adc_gain; 
  char mic_gain_enable; 
  char cd_mix_gain; 
  char cd_mix_mute; 
  char aux2_mix_gain; 
  char aux2_mix_mute; 
  char line_mix_gain; 
  char line_mix_mute; 
  char dac_attn; 
  char dac_mute; 
}; 

typedef struct sound_setup { 
  struct channel left; 
  struct channel right; 
  enum sample_rate sample_rate; 
  enum sample_format playback_format; 
  enum sample_format capture_format; 
  char dither_enable; 
  char mic_attn; 
  char mic_enable; 
  char output_boost; 
  char highpass_enable; 
  char mono_gain; 
  char mono_mute; 
} sound_setup; 

sound_setup setup = 
{
		{ line, 15, 0, 31,0,0,0,31,0,6 /* dac attenuation */,0 }, /* left channel*/
		{ line, 15, 0, 31,0,0,0,31,0,6 /* dac attenuation */,0 }, /* right channel*/
		kHz_44_1,   /* sample rate */ 
		(sample_format)0,  /* ignore (always 16bit-linear) */ 
  		(sample_format)0,  /* ignore (always 16bit-linear) */ 
		0,	/* non-zero enables dither on 16 => 8 bit */ 
		0,  /* 0..64 mic input level */ 
		0,  /* non-zero enables mic input */ 
		0,  /* ignore (always on) */ 
		0,  /* ignore (always on) */ 
		9, /* 0..64 mono speaker gain */ 
		0   /* non-zero mutes speaker */ 
};


_SoundPlayNode::_SoundPlayNode(const char *name, const media_multi_audio_format *format, BSoundPlayer *player) : 
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaNode(name)
{
	TRACE("_SoundPlayNode\n");
	fFormat = *format;
	fPlayer = player;
	fThreadId = -1;

	fFramesPerBuffer = fFormat.buffer_size / (fFormat.channel_count * (fFormat.format & media_raw_audio_format::B_AUDIO_SIZE_MASK));

	printf("Format Info:\n");
	printf("  frame_rate:     %f\n",fFormat.frame_rate);
	printf("  channel_count:  %ld\n",fFormat.channel_count);
	printf("  byte_order:     %ld (",fFormat.byte_order);
	switch (fFormat.byte_order) {
		case B_MEDIA_BIG_ENDIAN: printf("B_MEDIA_BIG_ENDIAN)\n"); break;
		case B_MEDIA_LITTLE_ENDIAN: printf("B_MEDIA_LITTLE_ENDIAN)\n"); break;
		default: printf("unknown)\n"); break;
	}
	printf("  buffer_size:    %ld\n",fFormat.buffer_size);
	printf("  format:         %ld (",fFormat.format);
	switch (fFormat.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT: printf("B_AUDIO_FLOAT)\n"); break;
		case media_raw_audio_format::B_AUDIO_SHORT: printf("B_AUDIO_SHORT)\n"); break;
		case media_raw_audio_format::B_AUDIO_INT: printf("B_AUDIO_INT)\n"); break;
		case media_raw_audio_format::B_AUDIO_CHAR: printf("B_AUDIO_CHAR)\n"); break;
		case media_raw_audio_format::B_AUDIO_UCHAR: printf("B_AUDIO_UCHAR)\n"); break;
		default: printf("unknown)\n"); break;
	}

	driver = open("/dev/audio/old/awe64/1", O_RDWR);
	if (driver <= 0)
		debugger("couldn't open soundcard device driver\n");
		
	sem = create_sem(0,"sem");
	ioctl(driver,SOUND_SET_PARAMS,&setup);
	ioctl(driver,SOUND_SET_PLAYBACK_COMPLETION_SEM,&sem);
}


_SoundPlayNode::~_SoundPlayNode()
{
	TRACE("~_SoundPlayNode\n");
	Stop();
	close(driver);
}


media_multi_audio_format 
_SoundPlayNode::Format() const
{
	return fFormat;
}


/* virtual */ status_t 
_SoundPlayNode::FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format)
{
	return B_ERROR;
}


/* virtual */ status_t 
_SoundPlayNode::FormatProposal(
				const media_source & output,
				media_format * format)
{
	return B_ERROR;
}

				
/* If the format isn't good, put a good format into *io_format and return error */
/* If format has wildcard, specialize to what you can do (and change). */
/* If you can change the format, return OK. */
/* The request comes from your destination sychronously, so you cannot ask it */
/* whether it likes it -- you should assume it will since it asked. */
/* virtual */ status_t 
_SoundPlayNode::FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_)
{
	return B_ERROR;
}

		
/* virtual */ status_t 
_SoundPlayNode::GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output)
{
	return B_ERROR;
}

				
/* virtual */ status_t 
_SoundPlayNode::DisposeOutputCookie(
				int32 cookie)
{
	return B_ERROR;
}

				
/* In this function, you should either pass on the group to your upstream guy, */
/* or delete your current group and hang on to this group. Deleting the previous */
/* group (unless you passed it on with the reclaim flag set to false) is very */
/* important, else you will 1) leak memory and 2) block someone who may want */
/* to reclaim the buffers living in that group. */
/* virtual */ status_t 
_SoundPlayNode::SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group)
{
	return B_ERROR;
}

				
/* Format of clipping is (as int16-s): <from line> <npairs> <startclip> <endclip>. */
/* Repeat for each line where the clipping is different from the previous line. */
/* If <npairs> is negative, use the data from line -<npairs> (there are 0 pairs after */
/* a negative <npairs>. Yes, we only support 32k*32k frame buffers for clipping. */
/* Any non-0 field of 'display' means that that field changed, and if you don't support */
/* that change, you should return an error and ignore the request. Note that the buffer */
/* offset values do not have wildcards; 0 (or -1, or whatever) are real values and must */
/* be adhered to. */
/* virtual */ status_t 
_SoundPlayNode::VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_)
{
	return B_ERROR;
}

				
/* Iterates over all outputs and maxes the latency found */
/* virtual */ status_t 
_SoundPlayNode::GetLatency(
				bigtime_t * out_lantency)
{
	return B_ERROR;
}

				
/* virtual */ status_t 
_SoundPlayNode::PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	return B_ERROR;
}

				
/* virtual */ void 
_SoundPlayNode::Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name)
{
}

			
/* virtual */ void 
_SoundPlayNode::Disconnect(
				const media_source & what,
				const media_destination & where)
{
}

			
/* virtual */ void 
_SoundPlayNode::LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time)
{
}

			
/* virtual */ void 
_SoundPlayNode::EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_)
{
}


/* Who instantiated you -- or NULL for app class */
/* virtual */ BMediaAddOn* 
_SoundPlayNode::AddOn(int32 * internal_id) const
{
	return NULL;
}

void 
_SoundPlayNode::Start()
{
	TRACE("_SoundPlayNode::Start\n");
	if (fThreadId != -1)
		return;
	
	fStopThread = false;
	fThreadId = spawn_thread(threadstart,"OpenBeOS play thread",105,this);
	resume_thread(fThreadId);
}

void 
_SoundPlayNode::Stop()
{
	TRACE("_SoundPlayNode::Stop\n");
	if (fThreadId == -1)
		return;
		
	fStopThread = true;
	
	status_t dummy;
	wait_for_thread(fThreadId,&dummy);
	fThreadId = -1;
}


int32 
_SoundPlayNode::threadstart(void *arg)
{
	((_SoundPlayNode *)arg)->PlayThread();
	return 0;
}


void
_SoundPlayNode::PlayThread()
{
	/*
	 * without real media nodes, timesources, and other stuff,
	 * it's not easy to provide accurate audio playback 
	 */
	double buffer_time = (1000000.0 * fFramesPerBuffer) / fFormat.frame_rate;
	bool ChangeSampleformat = fFormat.format != media_raw_audio_format::B_AUDIO_SHORT;
	bool ChangeSamplingrate = fFormat.frame_rate != 44100.0f;
	bool ChangeChannelcount = fFormat.channel_count != 2;
	int ResampledFramesPerBuffer = int(0.5f + (fFramesPerBuffer * (44100.0f / fFormat.frame_rate)));
	void *temp1 = 0;
	void *temp2 = 0;
	void *temp3 = 0;
	
	if (ChangeSampleformat)
		temp1 = new char [(fFormat.format & 0xf) * fFramesPerBuffer * fFormat.channel_count];
	if (ChangeSamplingrate)
		temp2 = new char [sizeof(uint16) * ResampledFramesPerBuffer * fFormat.channel_count];
	if (ChangeChannelcount)
		temp3 = new char [sizeof(uint16) * ResampledFramesPerBuffer * 2];

	TRACE("_SoundPlayNode::PlayThread: ChangeSampleformat = %d, ChangeSamplingrate = %d, ChangeChannelcount = %d\n",
		ChangeSampleformat,ChangeSamplingrate,ChangeChannelcount);
	
	double buf_nb;
	bigtime_t start;
	void *source;

	char *buf = new char [2 * 2 * ResampledFramesPerBuffer + sizeof(audio_buffer_header)];
	char *buffer = buf + sizeof(audio_buffer_header);
	audio_buffer_header *header = (audio_buffer_header *)buf;
	header->reserved_1 = 2 * 2 * ResampledFramesPerBuffer + sizeof(audio_buffer_header); 
	
	start = system_time() + 50000;
	buf_nb = 0;
	while (!fStopThread) {
		if (fPlayer->_m_has_data != 0) {
			memset(fPlayer->_m_buf,0,fPlayer->_m_bufsize);
			fPlayer->PlayBuffer(fPlayer->_m_buf,fPlayer->_m_bufsize,(const media_raw_audio_format)fFormat);
			source = fPlayer->_m_buf;
			if (ChangeSampleformat) {
				MediaKitPrivate::SampleConverter::convert(
					temp1, media_raw_audio_format::B_AUDIO_SHORT, 
            		source, fFormat.format, 
            		fFramesPerBuffer * fFormat.channel_count);
            	source = temp1;
			}
			if (ChangeSamplingrate) {
				MediaKitPrivate::SamplingrateConverter::convert(
					temp2, ResampledFramesPerBuffer,
            		source, fFramesPerBuffer,
            		media_raw_audio_format::B_AUDIO_SHORT,		
            		fFormat.channel_count);
            	source = temp2;
			}
			if (ChangeChannelcount) {
				MediaKitPrivate::ChannelMixer::mix(
					temp3, 2,
            		source, fFormat.channel_count,
            		ResampledFramesPerBuffer,
            		media_raw_audio_format::B_AUDIO_SHORT);
            	source = temp3;
            }
//            if (fFormat.byte_order == B_MEDIA_LITTLE_ENDIAN)
//            	swap_data(B_INT16_TYPE,source,ResampledFramesPerBuffer * 2 * 2,B_SWAP_ALWAYS);
            //at this place, we have a buffer with sound data
            //of 2 channels, 16bit integer samples, at 44100 frames/sec
            //pointer: source
            //framecount: ResampledFramesPerBuffer

//			snooze_until(start + bigtime_t(buf_nb++ * buffer_time),B_SYSTEM_TIMEBASE);
			buf_nb++;
			memcpy(buffer,source,ResampledFramesPerBuffer * 2 * 2);
			ioctl(driver,SOUND_WRITE_BUFFER,buf);
			acquire_sem(sem);

		} else {
			snooze_until(start + bigtime_t(buf_nb++ * buffer_time),B_SYSTEM_TIMEBASE);
		}
	}
	TRACE("_SoundPlayNode::PlayThread: %.0f buffers processed\n",buf_nb);
	delete [] temp1;
	delete [] temp2;
	delete [] temp3;
	delete [] buf;
}

