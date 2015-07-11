/******************************************************************************

	File:	MediaDefs.h

	Copyright 1995-97, Be Incorporated

******************************************************************************/
#ifndef _MEDIA_DEFS_H
#define _MEDIA_DEFS_H

#include <SupportDefs.h>
#include <Errors.h>

/* Buffer header for audio server */

typedef struct audio_buffer_header {
  int32 buffer_number;
  int32 subscriber_count;
  bigtime_t time;
  int32 reserved_1;
  int32 reserved_2;
  int32 reserved_3;
  int32 reserved_4;
} audio_buffer_header;


#define		B_MEDIA_LEVEL	B_REAL_TIME_PRIORITY

#define 	B_NO_CHANGE (-1)

/* ================
   Subscriber IDs and special values
   ================ */

#define			B_NO_SUBSCRIBER_ID		((subscriber_id)-1)
#define			B_NO_SUBSCRIBER_NAME		"not subscribed"

#define			B_SHARED_SUBSCRIBER_ID	((subscriber_id)-2)
#define			B_SHARED_SUBSCRIBER_NAME	"shared subscriber"

/* ================
   Values for sound files and audio streams 
   ================ */

/* values for byte_ordering */
enum { B_BIG_ENDIAN, B_LITTLE_ENDIAN };

/* values for sample_format */
enum { 
  B_UNDEFINED_SAMPLES,
  B_LINEAR_SAMPLES,
  B_FLOAT_SAMPLES,
  B_MULAW_SAMPLES
  };

/* Audio device codes for BAudioSubscriber's
 * Get/SetVolume() and EnableDevice() calls 
 */
enum  {
	B_CD_THROUGH=0,
	B_LINE_IN_THROUGH,
	B_ADC_IN,
	B_LOOPBACK,
	B_DAC_OUT,
	B_MASTER_OUT,
	B_SPEAKER_OUT,
	B_SOUND_DEVICE_END
  };

/* ADC input codes */
enum {
	B_CD_IN,
	B_LINE_IN,
	B_MIC_IN 
  };


enum {
  B_DAC_STREAM = 2354,
  B_ADC_STREAM
  };

#endif	// #ifndef _MEDIA_DEFS_H
