/*
 * Copyright (c) 2005 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */
 
/* This is the driver ioctl interface used for DVB on BeOS
 * it is not final, and will be changed before a public release
 * is made of the DVB framework (media add-on and example player).
 *
 * The driver should publish in /dev/dvb/cardname/n (with n=1..).
 * It must implement the ioctl codes from dvb_ioctl_t.
 * The driver must have buffers readable by userspace, it
 * should directly use DMA buffers for that.
 * The DVB add-on expects DVB_CAPTURE to block until data is
 * available, or a timeout has expired. If the timeout expired,
 * it must return B_TIMED_OUT. This ensures that the driver 
 * doesn't block when reception conditions are going bad, and
 * allows user space programs to recognize this.
 * If the ioctl succeeds, the driver must record the capture end time 
 * and report the data pointer and size. The data must stay valid
 * for a few ms, to give the user space thread, running at realtime
 * priority, a chance to read it. The driver must do at least
 * double buffering to achieve this. 
 * For DVB-T, two buffers of 61 kByte give about 16 ms validity period
 * for each buffer, thats enough. A 100 ms timout is used, to ensure
 * that DVB_CAPTURE doesn't block when no data is available.
 *
 * A normal ioctl sequence is:
 *  - DVB_GET_INTERFACE_INFO
 *  - DVB_SET_TUNING_PARAMETERS
 *  - DVB_GET_STATUS
 *  - DVB_START_CAPTURE
 *  - DVB_CAPTURE (repeated)
 *  - DVB_STOP_CAPTURE
 *
 * The following ioctls can be called at any time, 
 * including when capture is active:
 *  - DVB_GET_INTERFACE_INFO
 *  - DVB_GET_FREQUENCY_INFO
 *  - DVB_GET_TUNING_PARAMETERS
 *  - DVB_GET_STATUS
 *  - DVB_GET_SS
 *  - DVB_GET_BER
 *  - DVB_GET_SNR
 *  - DVB_GET_UPC
 *
 * In future releases, other ioctls that are not yet defined may
 * be called. It's important that the driver does not crash, but 
 * instead returnes an error code.
 *
 * Parameter reference for ioctl() functions:
 *
 * opcode                    | parameter               | description
 * --------------------------+-------------------------+-----------------------------------------
 * DVB_GET_INTERFACE_INFO    | dvb_interface_info_t    | get interface capabilites (memset to 0 first)
 * DVB_GET_FREQUENCY_INFO    | dvb_frequency_info_t    | get interface capabilites (memset to 0 first)
 * DVB_SET_TUNING_PARAMETERS | dvb_tuning_parameters_t | perform tuning information
 * DVB_GET_TUNING_PARAMETERS | dvb_tuning_parameters_t | get active parameters (after autotuning) (memset to 0 first)
 * DVB_START_CAPTURE         | (none)                  | start the capture
 * DVB_STOP_CAPTURE          | (none)                  | stop the capture
 * DVB_GET_STATUS            | dvb_status_t            | get current status
 * DVB_GET_SS                | uint32                  | signal strength, range 0 to 1000
 * DVB_GET_BER               | uint32                  | block error rate, range 0 to 1000
 * DVB_GET_SNR               | uint32                  | signal noise ratio, range 0 to 1000
 * DVB_GET_UPC               | uint32                  | XXX ???, range 0 to 1000
 * DVB_CAPTURE               | dvb_capture_t           | get information about captured data
 *
 */
 
 
#ifndef __DVB_H
#define __DVB_H

#include <Drivers.h>

typedef enum {
	DVB_TYPE_UNKNOWN			= -1,
	DVB_TYPE_DVB_C				= 0x10,
	DVB_TYPE_DVB_H				= 0x20,
	DVB_TYPE_DVB_S				= 0x30,
	DVB_TYPE_DVB_T				= 0x40,
} dvb_type_t;


// channel bandwith
typedef enum {
	DVB_BANDWIDTH_AUTO	 		= 0,
	DVB_BANDWIDTH_5_MHZ			= 5000,
	DVB_BANDWIDTH_6_MHZ			= 6000,
	DVB_BANDWIDTH_7_MHZ			= 7000,
	DVB_BANDWIDTH_8_MHZ			= 8000,
} dvb_bandwidth_t;


// channel polarity
typedef enum {
	DVB_POLARITY_UNKNOWN		= -1,
	DVB_POLARITY_VERTICAL		= 1,
	DVB_POLARITY_HORIZONTAL		= 2,
} dvb_polarity_t;


// "modulation" or "constellation"
typedef enum {
	DVB_MODULATION_AUTO,
	DVB_MODULATION_QPSK,
	DVB_MODULATION_16_QAM,
	DVB_MODULATION_32_QAM,
	DVB_MODULATION_64_QAM,
	DVB_MODULATION_128_QAM,
	DVB_MODULATION_256_QAM,
} dvb_modulation_t;


// dvb_cofdm_mode, dvb_transmission_mode
typedef enum {
	DVB_TRANSMISSION_MODE_AUTO,
	DVB_TRANSMISSION_MODE_2K,
	DVB_TRANSMISSION_MODE_4K,
	DVB_TRANSMISSION_MODE_8K,
} dvb_transmission_mode_t;


// code rate, specified by inner FEC (forward error correction) scheme
typedef enum {
	DVB_FEC_AUTO,
	DVB_FEC_NONE,
	DVB_FEC_1_2,
	DVB_FEC_2_3,
	DVB_FEC_3_4,
	DVB_FEC_4_5,
	DVB_FEC_5_6,
	DVB_FEC_6_7,
	DVB_FEC_7_8,
	DVB_FEC_8_9,
} dvb_code_rate_t;


// guard interval
typedef enum {
	DVB_GUARD_INTERVAL_AUTO,
	DVB_GUARD_INTERVAL_1_4,
	DVB_GUARD_INTERVAL_1_8,
	DVB_GUARD_INTERVAL_1_16,
	DVB_GUARD_INTERVAL_1_32,
} dvb_guard_interval_t;


// alpha value for hierarchical transmission
typedef enum {
	DVB_HIERARCHY_AUTO,
	DVB_HIERARCHY_NONE,
	DVB_HIERARCHY_1,
	DVB_HIERARCHY_2,
	DVB_HIERARCHY_4,
} dvb_hierarchy_t;


// spectral inversion
typedef enum {
	DVB_INVERSION_AUTO,
	DVB_INVERSION_ON,
	DVB_INVERSION_OFF,
} dvb_inversion_t;


typedef enum {
	DVB_STATUS_SIGNAL		= 0x0001,
	DVB_STATUS_CARRIER		= 0x0002,
	DVB_STATUS_LOCK			= 0x0004,
	DVB_STATUS_VITERBI		= 0x0008,
	DVB_STATUS_SYNC			= 0x0010,
} dvb_status_t;


typedef enum {
	DVB_GET_INTERFACE_INFO		= (B_DEVICE_OP_CODES_END + 0x100),
	DVB_GET_FREQUENCY_INFO 		= (B_DEVICE_OP_CODES_END + 0x120),
	DVB_SET_TUNING_PARAMETERS,
	DVB_GET_TUNING_PARAMETERS,
	DVB_START_CAPTURE,
	DVB_STOP_CAPTURE,
	DVB_GET_STATUS,
	DVB_GET_SS,
	DVB_GET_BER,
	DVB_GET_SNR,
	DVB_GET_UPC,
	DVB_CAPTURE,
} dvb_ioctl_t;


typedef struct {
	uint32					version;	// set this to 1
	uint32					flags;		// set this to 0
	dvb_type_t				type;
	uint32					_res1[16];
	char					name[100];
	char					info[500];
	uint32					_res2[16];
} dvb_interface_info_t;


typedef struct {
	uint32					_res1[16];
	uint64					frequency_min;
	uint64					frequency_max;
	uint64					frequency_step;
	uint32					_res2[64];
} dvb_frequency_info_t;


typedef struct {
	uint64					frequency;
	dvb_inversion_t			inversion;
	dvb_bandwidth_t			bandwidth;
	dvb_modulation_t		modulation;
	dvb_hierarchy_t			hierarchy;
	dvb_code_rate_t			code_rate_hp;
	dvb_code_rate_t			code_rate_lp;
	dvb_transmission_mode_t	transmission_mode;
	dvb_guard_interval_t	guard_interval;
} dvb_t_tuning_parameters_t;


typedef struct {
	uint64					frequency;
	dvb_inversion_t			inversion;
	dvb_modulation_t		modulation;
	uint32					symbolrate;
	dvb_polarity_t			polarity;
} dvb_s_tuning_parameters_t;


typedef struct {
	union {
		dvb_t_tuning_parameters_t	dvb_t;
		dvb_s_tuning_parameters_t	dvb_s;
		uint32						_pad[32];
	} u;
} dvb_tuning_parameters_t;


typedef struct {
	void *					data;
	size_t					size;
	bigtime_t				end_time;
	uint32					_res[2];
} dvb_capture_t;

#endif
