#ifndef _OLD_SOUND_DRIVER_H_
#define _OLD_SOUND_DRIVER_H_
 
/*
 * The updated BeOS R3 style legacy sound card driver interface
 * Assembled from the Be Newsletter, Volume II, Issue 22; June 3, 1998
 * and Be Newsletter, Volume III, Issue 12, March 24, 1999
 */
 
enum { 
  SOUND_GET_PARAMS = B_DEVICE_OP_CODES_END, 
  SOUND_SET_PARAMS, 
  SOUND_SET_PLAYBACK_COMPLETION_SEM, 
  SOUND_SET_CAPTURE_COMPLETION_SEM, 
  SOUND_RESERVED_1,   /* unused */ 
  SOUND_RESERVED_2,   /* unused */ 
  SOUND_DEBUG_ON,     /* unused */ 
  SOUND_DEBUG_OFF,    /* unused */ 
  SOUND_WRITE_BUFFER, 
  SOUND_READ_BUFFER, 
  SOUND_LOCK_FOR_DMA, 
  SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE, 
  SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE, 
  SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE, 
  SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE 
};

typedef struct audio_buffer_header {
  int32 buffer_number;
  int32 subscriber_count;
  bigtime_t time;
  int32 reserved_1;
  int32 reserved_2;
  bigtime_t sample_clock;
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
       /* adc input source */ 
  char adc_gain; 
       /* 0..15 adc gain, in 1.5 dB steps */ 
  char mic_gain_enable; 
       /* non-zero enables 20 dB MIC input gain */ 
  char cd_mix_gain; 
       /* 0..31 cd mix to output gain in -1.5dB steps */ 
  char cd_mix_mute; 
       /* non-zero mutes cd mix */ 
  char aux2_mix_gain; 
       /* unused */ 
  char aux2_mix_mute; 
       /* unused */ 
  char line_mix_gain; 
       /* 0..31 line mix to output gain in -1.5dB steps */ 
  char line_mix_mute; 
       /* non-zero mutes line mix */ 
  char dac_attn; 
       /* 0..61 dac attenuation, in -1.5 dB steps */ 
  char dac_mute; 
       /* non-zero mutes dac output */ 
}; 

typedef struct sound_setup { 
  struct channel left; 
       /* left channel setup */ 
  struct channel right; 
       /* right channel setup */ 
  enum sample_rate sample_rate; 
       /* sample rate */ 
  enum sample_format playback_format; 
       /* ignore (always 16bit-linear) */ 
  enum sample_format capture_format; 
       /* ignore (always 16bit-linear) */ 
  char dither_enable; 
       /* non-zero enables dither on 16 => 8 bit */ 
  char mic_attn; 
       /* 0..64 mic input level */ 
  char mic_enable; 
       /* non-zero enables mic input */ 
  char output_boost; 
       /* ignore (always on) */ 
  char highpass_enable; 
       /* ignore (always on) */ 
  char mono_gain; 
       /* 0..64 mono speaker gain */ 
  char mono_mute; 
       /* non-zero mutes speaker */ 
} sound_setup; 

#endif
