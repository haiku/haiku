#ifndef _MIXER_UTILS_H
#define _MIXER_UTILS_H

#if DEBUG > 0
	#define PRINT_CHANNEL_MASK(fmt) do { char s[200]; string_for_channel_mask(s, (fmt).u.raw_audio.channel_mask); printf(" channel_mask 0x%08X %s\n", (fmt).u.raw_audio.channel_mask, s); } while (0)
#else
	#define PRINT_CHANNEL_MASK(fmt) ((void)0)
#endif

void string_for_channel_mask(char *str, uint32 mask);
void fix_multiaudio_format(media_multi_audio_format *format);

int count_nonzero_bits(uint32 value);

uint32 GetChannelMask(int channel, uint32 all_channel_masks);

bool HasKawamba();

void ZeroFill(float *_dst, int32 _dst_sample_offset, int32 _sample_count);

bigtime_t buffer_duration(const media_multi_audio_format & format);

int bytes_per_sample(const media_multi_audio_format & format);

int bytes_per_frame(const media_multi_audio_format & format);
int frames_per_buffer(const media_multi_audio_format & format);

int64 frames_for_duration(double framerate, bigtime_t duration);
bigtime_t duration_for_frames(double framerate, int64 frames);

int ChannelMaskToChannelType(uint32 mask);
uint32 ChannelTypeToChannelMask(int type);

double us_to_s(bigtime_t usecs);
bigtime_t s_to_us(double secs);

class MixerInput;
class MixerOutput;

const char *StringForFormat(MixerOutput *output);	// not thread save
const char *StringForFormat(MixerInput *input);		// not thread save

#endif //_MIXER_UTILS_H
