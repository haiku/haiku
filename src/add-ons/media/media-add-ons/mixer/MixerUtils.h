
void string_for_channel_mask(char *str, uint32 mask);
void fix_multiaudio_format(media_multi_audio_format *format);

#define PRINT_CHANNEL_MASK(fmt) do { char s[200]; string_for_channel_mask(s, (fmt).u.raw_audio.channel_mask); printf(" channel_mask 0x%08X %s\n", (fmt).u.raw_audio.channel_mask, s); } while (0)
