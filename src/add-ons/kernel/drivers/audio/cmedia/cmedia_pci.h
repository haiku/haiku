/* cmedia_pci.h -- specifics for S3-based PCI audio cards */
/* $Id: cmedia_pci.h,v 1.3 1999/10/13 02:29:19 cltien Exp $ */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(_CMEDIA_PCI_H)
#define _CMEDIA_PCI_H

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include "audio_driver.h"
#include "midi_driver.h"
#include "joystick_driver.h"


#define CMEDIA_PCI_VENDOR_ID	0x13F6	/* C-Media Inc	*/
#define CMEDIA_8338A_DEVICE_ID	0x0100	/* CM8338A		*/
#define CMEDIA_8338B_DEVICE_ID	0x0101	/* CM8338B		*/
#define CMEDIA_8738A_DEVICE_ID	0x0111	/* CM8738A		*/
#define CMEDIA_8738B_DEVICE_ID	0x0112	/* CM8738B		*/

#define CMEDIA_PCI_JOYSTICK_MIN_LATENCY 5000		/* 200 times a second! */
#define CMEDIA_PCI_JOYSTICK_MAX_LATENCY 100000		/* 10 times a second */

#define DRIVER_NAME "cmedia"

typedef struct joystick cmedia_pci_joystick;


typedef struct audio_format cmedia_pci_audio_format;
typedef struct audio_buf_header cmedia_pci_audio_buf_header;


/* the mux devices use these records */
typedef audio_routing cmedia_pci_routing;

/* this is the argument for ioctl() */
typedef audio_routing_cmd cmedia_pci_routing_cmd;


/* selectors for routing */
#define CMEDIA_PCI_INPUT_MUX				B_AUDIO_INPUT_SELECT
#define CMEDIA_PCI_MIC_BOOST				B_AUDIO_MIC_BOOST
#define CMEDIA_PCI_MIDI_OUTPUT_TO_SYNTH	B_AUDIO_MIDI_OUTPUT_TO_SYNTH
#define CMEDIA_PCI_MIDI_INPUT_TO_SYNTH		B_AUDIO_MIDI_INPUT_TO_SYNTH
#define CMEDIA_PCI_MIDI_OUTPUT_TO_PORT		B_AUDIO_MIDI_OUTPUT_TO_PORT

/* input MUX source values */
#define CMEDIA_PCI_INPUT_CD				B_AUDIO_INPUT_CD
#define CMEDIA_PCI_INPUT_DAC				B_AUDIO_INPUT_DAC
#define CMEDIA_PCI_INPUT_AUX2				B_AUDIO_INPUT_AUX2
#define CMEDIA_PCI_INPUT_LINE				B_AUDIO_INPUT_LINE_IN
#define CMEDIA_PCI_INPUT_AUX1				B_AUDIO_INPUT_AUX1
#define CMEDIA_PCI_INPUT_MIC				B_AUDIO_INPUT_MIC
#define CMEDIA_PCI_INPUT_MIX_OUT			B_AUDIO_INPUT_MIX_OUT


/* the mixer devices use these records */
typedef audio_level cmedia_pci_level;

/* this is the arg to ioctl() */
typedef audio_level_cmd cmedia_pci_level_cmd;

/* bitmask for the flags */
#define CMEDIA_PCI_LEVEL_MUTED				B_AUDIO_LEVEL_MUTED

/* selectors for levels */
#define CMEDIA_PCI_LEFT_ADC_INPUT_G		B_AUDIO_MIX_ADC_LEFT
#define CMEDIA_PCI_RIGHT_ADC_INPUT_G 		B_AUDIO_MIX_ADC_RIGHT
#define CMEDIA_PCI_LEFT_AUX1_LOOPBACK_GAM	B_AUDIO_MIX_VIDEO_LEFT
#define CMEDIA_PCI_RIGHT_AUX1_LOOPBACK_GAM	B_AUDIO_MIX_VIDEO_RIGHT
#define CMEDIA_PCI_LEFT_CD_LOOPBACK_GAM	B_AUDIO_MIX_CD_LEFT
#define CMEDIA_PCI_RIGHT_CD_LOOPBACK_GAM	B_AUDIO_MIX_CD_RIGHT
#define CMEDIA_PCI_LEFT_LINE_LOOPBACK_GAM	B_AUDIO_MIX_LINE_IN_LEFT
#define CMEDIA_PCI_RIGHT_LINE_LOOPBACK_GAM	B_AUDIO_MIX_LINE_IN_RIGHT
#define CMEDIA_PCI_MIC_LOOPBACK_GAM		B_AUDIO_MIX_MIC
#define CMEDIA_PCI_LEFT_SYNTH_OUTPUT_GAM	B_AUDIO_MIX_SYNTH_LEFT
#define CMEDIA_PCI_RIGHT_SYNTH_OUTPUT_GAM	B_AUDIO_MIX_SYNTH_RIGHT
#define CMEDIA_PCI_LEFT_AUX2_LOOPBACK_GAM	B_AUDIO_MIX_AUX_LEFT
#define CMEDIA_PCI_RIGHT_AUX2_LOOPBACK_GAM	B_AUDIO_MIX_AUX_RIGHT
#define CMEDIA_PCI_LEFT_MASTER_VOLUME_AM	B_AUDIO_MIX_LINE_OUT_LEFT
#define CMEDIA_PCI_RIGHT_MASTER_VOLUME_AM	B_AUDIO_MIX_LINE_OUT_RIGHT
#define CMEDIA_PCI_LEFT_PCM_OUTPUT_GAM		B_AUDIO_MIX_DAC_LEFT
#define CMEDIA_PCI_RIGHT_PCM_OUTPUT_GAM	B_AUDIO_MIX_DAC_RIGHT
#define CMEDIA_PCI_DIGITAL_LOOPBACK_AM		B_AUDIO_MIX_LOOPBACK_LEVEL


/* secret handshake ioctl()s */
#define SV_SECRET_HANDSHAKE 10100
typedef struct {
	bigtime_t	wr_time;
	bigtime_t	rd_time;
	uint32		wr_skipped;
	uint32		rd_skipped;
	uint64		wr_total;
	uint64		rd_total;
	uint32		_reserved_[6];
} sv_handshake;
#define SV_RD_TIME_WAIT 10101
#define SV_WR_TIME_WAIT 10102
typedef struct {
	bigtime_t	time;
	bigtime_t	bytes;
	uint32		skipped;
	uint32		_reserved_[3];
} sv_timing;



#endif	/* _CMEDIA_PCI_H */

