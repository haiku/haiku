
void string_for_channel_mask(char *str, uint32 mask);
void fix_multiaudio_format(media_multi_audio_format *format);

int count_nonzero_bits(uint32 value);

#define PRINT_CHANNEL_MASK(fmt) do { char s[200]; string_for_channel_mask(s, (fmt).u.raw_audio.channel_mask); printf(" channel_mask 0x%08X %s\n", (fmt).u.raw_audio.channel_mask, s); } while (0)

uint32 GetChannelMask(int channel, uint32 all_channel_masks);

void CopySamples(float *_dst, int32 _dst_sample_offset,
				 const float *_src, int32 _src_sample_offset,
				 int32 _sample_count);

int bytes_per_sample(const media_multi_audio_format & format);

int bytes_per_frame(const media_multi_audio_format & format);
int frames_per_buffer(const media_multi_audio_format & format);

int64 frames_for_duration(double framerate, bigtime_t duration);
bigtime_t duration_for_frames(double framerate, int64 frames);

int ChannelMaskToChannelType(uint32 mask);
uint32 ChannelTypeToChannelMask(int type);
