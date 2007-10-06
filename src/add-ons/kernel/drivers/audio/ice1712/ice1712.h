/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2002, Jerome Duval		(jerome.duval@free.fr)
 * Copyright (c) 2003, Marcus Overhagen	(marcus@overhagen.de)
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#ifndef _ICE1712_H_
#define _ICE1712_H_

#include <PCI.h>

#define DRIVER_NAME "ice1712"
#define VERSION "0.3"

#define ICE1712_VENDOR_ID			0x1412
#define ICE1712_DEVICE_ID			0x1712


typedef enum product_t {
	ICE1712_SUBDEVICE_DELTA1010			= 0x121430d6,
	ICE1712_SUBDEVICE_DELTADIO2496		= 0x121431d6,
	ICE1712_SUBDEVICE_DELTA66			= 0x121432d6,
	ICE1712_SUBDEVICE_DELTA44			= 0x121433d6,
	ICE1712_SUBDEVICE_AUDIOPHILE_2496	= 0x121434d6,
	ICE1712_SUBDEVICE_DELTA410			= 0x121438d6,
	ICE1712_SUBDEVICE_DELTA1010LT		= 0x12143bd6,
	ICE1712_SUBDEVICE_VX442				= 0x12143cd6,
} product_t;

#define NUM_CARDS					4
#define MAX_ADC						12	// + the output of the Digital mixer
#define MAX_DAC						10
#define SWAPPING_BUFFERS			2
#define SAMPLE_SIZE					4
#define MIN_BUFFER_FRAMES			64
#define MAX_BUFFER_FRAMES			2048

#define PLAYBACK_BUFFER_SIZE		(MAX_BUFFER_FRAMES * MAX_DAC * SAMPLE_SIZE)
#define RECORD_BUFFER_SIZE			(MAX_BUFFER_FRAMES * MAX_ADC * SAMPLE_SIZE)

#define PLAYBACK_BUFFER_TOTAL_SIZE	(PLAYBACK_BUFFER_SIZE * SWAPPING_BUFFERS)
#define RECORD_BUFFER_TOTAL_SIZE	(RECORD_BUFFER_SIZE * SWAPPING_BUFFERS)

#define SPDIF_LEFT					8
#define SPDIF_RIGHT					9
#define MIXER_OUT_LEFT				10
#define MIXER_OUT_RIGHT				11

typedef enum {
	NO_IN_NO_OUT = 0,
	NO_IN_YES_OUT = 1,
	YES_IN_NO_OUT = 2,
	YES_IN_YES_OUT = 3,
} _spdif_config_ ;

typedef struct _midi_dev {
	struct _ice1712_	*card;
	void				*driver;
	void				*cookie;
	int32				count;
	char				name[64];
} midi_dev;

typedef struct codec_info
{
	uint8	clock;
	uint8	data_in;
	uint8	data_out;
	uint8	reserved[5];
} codec_info;

typedef struct _ice1712_
{
	uint32 irq;
	pci_info info;
	char name[128];
	
	midi_dev midi_interf[2];
	
	uint32 Controller;	//PCI_10
	uint32 DDMA;		//PCI_14
	uint32 DMA_Path;	//PCI_18
	uint32 Multi_Track;	//PCI_1C

	int8 nb_ADC; //Mono Channel
	int8 nb_DAC; //Mono Channel
	_spdif_config_ spdif_config;
	int8 nb_MPU401;

	product_t product;
	uint8 gpio_cs_mask; //a Mask for removing all Chip select

	//We hope all manufacturer will not use different codec on the same card
	codec_info analog_codec;
	codec_info digital_codec;

	uint32 buffer;
	bigtime_t played_time;

	//Output
	area_id mem_id_pb;
	void *phys_addr_pb, *log_addr_pb;
	uint32 output_buffer_size; //in frames
	uint8 total_output_channels;

	//Input
	area_id mem_id_rec;
	void *phys_addr_rec, *log_addr_rec;
	uint32 input_buffer_size; //in frames
	uint8 total_input_channels;
	
	sem_id buffer_ready_sem;
	
	uint8 sampling_rate; //in the format of the register

} ice1712;

extern int32 num_cards;
extern ice1712 cards[NUM_CARDS];

//???????
#define GPIO_SPDIF_STATUS				0x02	//Status 
#define GPIO_SPDIF_CCLK					0x04	//data Clock
#define GPIO_SPDIF_DOUT					0x08	//data output

//For Delta 66 / Delta 44
#define DELTA66_DOUT					0x10	// data output
#define DELTA66_CLK						0x20	// clock
#define DELTA66_CODEC_CS_0				0x40	// AK4524 #0
#define DELTA66_CODEC_CS_1				0x80	// AK4524 #1

//For AudioPhile 2496 / Delta 410
#define AP2496_CLK						0x02	// clock
#define AP2496_DIN						0x04	// data input
#define AP2496_DOUT						0x08	// data output
#define AP2496_SPDIF_CS					0x10	// CS8427 chip select
#define AP2496_CODEC_CS					0x20	// AK4528 chip select

//For Delta 1010 LT
#define DELTA1010LT_CLK					0x02	// clock
#define DELTA1010LT_DIN					0x04	// data input
#define DELTA1010LT_DOUT				0x08	// data output
#define DELTA1010LT_CODEC_CS_0			0x00	// AK4524 #0
#define DELTA1010LT_CODEC_CS_1			0x10	// AK4524 #1
#define DELTA1010LT_CODEC_CS_2			0x20	// AK4524 #2
#define DELTA1010LT_CODEC_CS_3			0x30	// AK4524 #3
#define DELTA1010LT_SPDIF_CS			0x40	// CS8427
#define DELTA1010LT_CS_NONE				0x50	// All CS deselected

//For VX442
#define VX442_CLK						0x02	// clock
#define VX442_DIN						0x04	// data input
#define VX442_DOUT						0x08	// data output
#define VX442_SPDIF_CS					0x10	// CS8427
#define VX442_CODEC_CS_0				0x20	// ?? #0
#define VX442_CODEC_CS_1				0x40	// ?? #1

#define AK45xx_BITS_TO_WRITE			16
//2 - Chip Address (10b)
//1 - R/W (Always 1 for Writing)
//5 - Register Address
//8 - Data

//Register definition for the AK45xx codec (xx = 24 or 28)
#define AK45xx_DELAY					100		//Clock Delay
#define AK45xx_CHIP_ADDRESS				0x02	//Chip address of the codec
#define AK45xx_RESET_REGISTER			0x01
#define AK45xx_CLOCK_FORMAT_REGISTER	0x02
//Other register are not defined cause they are not used, I'm very lazy...

#define CS84xx_BITS_TO_WRITE			24
//7 - Chip Address (0010000b)
//1 - R/W (1 for Reading)
//8 - Register MAP
//8 - Data

//Register definition for the CS84xx codec (xx = 27)
#define CS84xx_DELAY					100		//Clock Delay
#define CS84xx_CHIP_ADDRESS				0x10	//Chip address of the codec
#define CS84xx_CONTROL_1_PORT_REG		0x01
#define CS84xx_CONTROL_2_PORT_REG		0x02
#define CS84xx_DATA_FLOW_CONTROL_REG	0x03
#define CS84xx_CLOCK_SOURCE_REG			0x04
#define CS84xx_SERIAL_INPUT_FORMAT_REG	0x05
#define CS84xx_SERIAL_OUTPUT_FORMAT_REG	0x06
//Other register are not defined cause they are not used, I'm very lazy...


/* A default switch for all suported product
	switch (card->product)
	{
		case ICE1712_SUBDEVICE_DELTA1010 :
			break;
		case ICE1712_SUBDEVICE_DELTADIO2496 :
			break;
		case ICE1712_SUBDEVICE_DELTA66 :
			break;
		case ICE1712_SUBDEVICE_DELTA44 :
			break;
		case ICE1712_SUBDEVICE_AUDIOPHILE :
			break;
		case ICE1712_SUBDEVICE_DELTA410 :
			break;
		case ICE1712_SUBDEVICE_DELTA1010LT :
			break;
		case ICE1712_SUBDEVICE_VX442 :
			break;
	}
*/

#endif
