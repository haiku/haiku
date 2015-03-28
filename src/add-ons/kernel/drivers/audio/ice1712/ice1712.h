/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */
#ifndef _ICE1712_H_
#define _ICE1712_H_


#include <PCI.h>
#include <KernelExport.h>

#include "debug.h"
#include "hmulti_audio.h"


#define DRIVER_NAME "ice1712"
#define VERSION "0.6"

#define ICE1712_VENDOR_ID 0x1412
#define ICE1712_DEVICE_ID 0x1712

typedef enum ice1712Product {
	ICE1712_SUBDEVICE_DELTA1010			= 0x121430d6,
	ICE1712_SUBDEVICE_DELTADIO2496		= 0x121431d6,
	ICE1712_SUBDEVICE_DELTA66			= 0x121432d6,
	ICE1712_SUBDEVICE_DELTA44			= 0x121433d6,
	ICE1712_SUBDEVICE_AUDIOPHILE_2496	= 0x121434d6,
	ICE1712_SUBDEVICE_DELTA410			= 0x121438d6,
	ICE1712_SUBDEVICE_DELTA1010LT		= 0x12143bd6,
	ICE1712_SUBDEVICE_VX442				= 0x12143cd6,
} ice1712Product;

#define NUM_CARDS					4
#define MAX_ADC						12
// 5 stereo output + the Digital mixer
#define MAX_DAC						10
#define MAX_MIDI_INTERFACE			2
#define SWAPPING_BUFFERS			2
#define SAMPLE_SIZE					4
#define MIN_BUFFER_FRAMES			64
#define MAX_BUFFER_FRAMES			2048

#define ICE1712_HARDWARE_VOLUME		10
#define ICE1712_MUTE_VALUE			0x7F

#define PLAYBACK_BUFFER_SIZE		(MAX_BUFFER_FRAMES * MAX_DAC * SAMPLE_SIZE)
#define RECORD_BUFFER_SIZE			(MAX_BUFFER_FRAMES * MAX_ADC * SAMPLE_SIZE)

#define PLAYBACK_BUFFER_TOTAL_SIZE	(PLAYBACK_BUFFER_SIZE * SWAPPING_BUFFERS)
#define RECORD_BUFFER_TOTAL_SIZE	(RECORD_BUFFER_SIZE * SWAPPING_BUFFERS)

#define SPDIF_LEFT					8
#define SPDIF_RIGHT					9
#define MIXER_OUT_LEFT				10
#define MIXER_OUT_RIGHT				11

#define SPDIF_OUT_PRESENT			1
#define SPDIF_IN_PRESENT			2

#define ICE1712_SAMPLERATE_96K		0x7
#define ICE1712_SAMPLERATE_48K		0x0
#define ICE1712_SAMPLERATE_88K2		0xB
#define ICE1712_SAMPLERATE_44K1		0x8

struct ice1712;

typedef struct ice1712Midi {
	struct ice1712 *card;
	void *mpu401device;
	uint8 int_mask;
	char name[64];
} ice1712Midi;

typedef struct _codecCommLines
{
	uint8 clock;
	uint8 data_in;
	uint8 data_out;
	uint8 cs_mask;
	//a Mask for removing all Chip select
} codecCommLines;

typedef struct ice1712Volume
{
	float volume;
	bool mute;
} ice1712Volume;

typedef struct ice1712Settings
{
	ice1712Volume playback[ICE1712_HARDWARE_VOLUME];
	ice1712Volume record[ICE1712_HARDWARE_VOLUME];

	uint32 bufferSize;

	//General Settings
	uint8 clock;		//an index

	//S/PDif Settings
	uint8 outFormat;	//an index
	uint8 emphasis;		//an index
	uint8 copyMode;		//an index

	//Output settings
	uint8 output[5];	//an index

	uint8 reserved[32];
} ice1712Settings;

typedef struct ice1712HW
{
	int8 nb_ADC;		//Mono Channel
	int8 nb_DAC;		//Mono Channel
	int8 nb_MPU401;
	int8 spdif;

	//in the format of the register
	uint8 samplingRate;
	uint32 lockSource;

	ice1712Product product;
} ice1712HW;

typedef struct ice1712
{
	uint32 irq;
	pci_info info;
	char name[128];

	ice1712Midi midiItf[MAX_MIDI_INTERFACE];

	uint32 Controller;	//PCI_10
	uint32 DDMA;		//PCI_14
	uint32 DMA_Path;	//PCI_18
	uint32 Multi_Track;	//PCI_1C

	uint8 eeprom_data[32];

	//We hope all manufacturers will use same
	//communication lines for speaking with codec
	codecCommLines CommLines;

	uint32 buffer;
	bigtime_t played_time;
	uint32 buffer_size; //in frames
	uint32 frames_count;

	//Output
	area_id mem_id_pb;
	physical_entry phys_pb;
	addr_t log_addr_pb;
	uint8 total_output_channels;

	//Input
	area_id mem_id_rec;
	physical_entry phys_rec;
	addr_t log_addr_rec;
	uint8 total_input_channels;

	sem_id buffer_ready_sem;

	ice1712HW config;
	ice1712Settings settings;
} ice1712;

//CSS_INTERRUPT_MASK
#define CCS_INTERRUPT_MIDI_1			0x80
#define CCS_INTERRUPT_MIDI_2			0x20

//???????
#define GPIO_SPDIF_STATUS				0x02	//Status
#define GPIO_SPDIF_CCLK					0x04	//data Clock
#define GPIO_SPDIF_DOUT					0x08	//data output

//For Delta 66 / Delta 44
#define DELTA66_DOUT					0x10	// data output
#define DELTA66_CLK						0x20	// clock
#define DELTA66_CODEC_CS_0				0x40	// AK4524 #0
#define DELTA66_CODEC_CS_1				0x80	// AK4524 #1
#define DELTA66_CS_MASK					0xD0	// Chip Select mask

//For AudioPhile 2496 / Delta 410
#define AP2496_CLK						0x02	// clock
#define AP2496_DIN						0x04	// data input
#define AP2496_DOUT						0x08	// data output
#define AP2496_SPDIF_CS					0x10	// CS8427 chip select
#define AP2496_CODEC_CS					0x20	// AK4528 chip select
#define AP2496_CS_MASK					0x30	// Chip Select Mask

//For Delta 1010 LT
#define DELTA1010LT_CLK					0x02	// clock
#define DELTA1010LT_DIN					0x04	// data input
#define DELTA1010LT_DOUT				0x08	// data output
#define DELTA1010LT_CODEC_CS_0			0x00	// AK4524 #0
#define DELTA1010LT_CODEC_CS_1			0x10	// AK4524 #1
#define DELTA1010LT_CODEC_CS_2			0x20	// AK4524 #2
#define DELTA1010LT_CODEC_CS_3			0x30	// AK4524 #3
#define DELTA1010LT_SPDIF_CS			0x40	// CS8427
#define DELTA1010LT_CS_NONE				0x70	// All CS deselected

//For VX442
#define VX442_CLK						0x02	// clock
#define VX442_DIN						0x04	// data input
#define VX442_DOUT						0x08	// data output
#define VX442_SPDIF_CS					0x10	// CS8427
#define VX442_CODEC_CS_0				0x20	// ?? #0
#define VX442_CODEC_CS_1				0x40	// ?? #1
#define VX442_CS_MASK					0x70	// Chip Select Mask

#define GPIO_I2C_DELAY					5		//Clock Delay for writing
												//I2C data throw GPIO

//Register definition for the AK45xx codec (xx = 24 or 28)
#define AK45xx_CHIP_ADDRESS				0x02	//Chip address of the codec
#define AK45xx_RESET_REGISTER			0x01
#define AK45xx_CLOCK_FORMAT_REGISTER	0x02
//Other register are not defined cause they are not used, I'm very lazy...

//Register definition for the CS84xx codec (xx = 27)
#define CS84xx_CHIP_ADDRESS				0x10	//Chip address of the codec
#define CS84xx_CONTROL_1_PORT_REG		0x01
#define CS84xx_CONTROL_2_PORT_REG		0x02
#define CS84xx_DATA_FLOW_CONTROL_REG	0x03
#define CS84xx_CLOCK_SOURCE_REG			0x04
#define CS84xx_SERIAL_INPUT_FORMAT_REG	0x05
#define CS84xx_SERIAL_OUTPUT_FORMAT_REG	0x06

#define CS84xx_VERSION_AND_CHIP_ID		0x7F
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

//This map comes from ALSA sound drivers
typedef enum ice1712EEprom {
	E2PROM_MAP_SUBVENDOR_LOW	= 0x00,
	E2PROM_MAP_SUBVENDOR_HIGH,
	E2PROM_MAP_SUBDEVICE_LOW,
	E2PROM_MAP_SUBDEVICE_HIGH,
	E2PROM_MAP_SIZE,
	E2PROM_MAP_VERSION,
	E2PROM_MAP_CONFIG,
	E2PROM_MAP_ACL,
	E2PROM_MAP_I2S,
	E2PROM_MAP_SPDIF,
	E2PROM_MAP_GPIOMASK,
	E2PROM_MAP_GPIOSTATE,
	E2PROM_MAP_GPIODIR,
	E2PROM_MAP_AC97MAIN,
	E2PROM_MAP_AC97PCM			= 0x0F,
	E2PROM_MAP_AC97REC			= 0x11,
	E2PROM_MAP_AC97REC_SOURCE	= 0x13,
	E2PROM_MAP_DAC_ID			= 0x14,
	E2PROM_MAP_ADC_ID			= 0x18,
	E2PROM_MAP_EXTRA			= 0x1C
} ice1712EEprom;

#endif
