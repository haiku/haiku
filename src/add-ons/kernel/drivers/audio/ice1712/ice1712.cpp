/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jérôme Lévêque, leveque.jerome@gmail.com
 */


#include "ice1712.h"
#include "ice1712_reg.h"
#include "io.h"
#include "multi.h"
#include "util.h"

#include <KernelExport.h>
#include <Drivers.h>
#include <OS.h>
#include <midi_driver.h>
#include <drivers/driver_settings.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static status_t ice1712Settings_load(ice1712 *card);
status_t ice1712Settings_apply(ice1712 *card);
static int32 ice1712HW_interrupt(void *arg);
static status_t ice1712HW_setup(ice1712 *ice);
static void ice1712HW_shutdown(ice1712 *ice);

extern device_hooks ice1712Midi_hooks;
extern device_hooks ice1712Audio_hooks;
void ice1712Midi_interrupt(int32 op, void *data);

pci_module_info *pci;
generic_mpu401_module *mpu401;

int32 num_cards = 0;
ice1712 cards[NUM_CARDS];

//3 interfaces (2 midi + 1 audio) can be defined for each card
static char *names[NUM_CARDS*3+1];
static int32 num_names = 0;

#define HMULTI_AUDIO_DEV_PATH "audio/hmulti/ice1712"

//Haiku Driver API
//----------------

int32 api_version = B_CUR_DRIVER_API_VERSION;

extern "C" status_t
init_hardware(void)
{
	int ix = 0;
	pci_info info;

	memset(cards, 0, sizeof(ice1712) * NUM_CARDS);
	ITRACE("@@ init_hardware()\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if ((info.vendor_id == ICE1712_VENDOR_ID)
			&& (info.device_id == ICE1712_DEVICE_ID)) {
			ITRACE("Found at least 1 card\n");
			put_module(B_PCI_MODULE_NAME);
			return B_OK;
		}
		ix++;
	}
	put_module(B_PCI_MODULE_NAME);
	return ENODEV;
}


extern "C" status_t
init_driver(void)
{
	int i = 0;
	status_t err;
	num_cards = 0;

	ITRACE("@@ init_driver()\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	if (get_module(B_MPU_401_MODULE_NAME, (module_info **) &mpu401)) {
		put_module(B_PCI_MODULE_NAME);
		return ENOSYS;
	}

	while ((*pci->get_nth_pci_info)(i, &cards[num_cards].info) == B_OK) {
		//TODO check other Vendor_ID and DEVICE_ID
		if ((cards[num_cards].info.vendor_id == ICE1712_VENDOR_ID)
			&& (cards[num_cards].info.device_id == ICE1712_DEVICE_ID)) {
			if (num_cards == NUM_CARDS) {
				ITRACE("Too many ice1712 cards installed!\n");
				break;
			}

			if ((err = (*pci->reserve_device)(cards[num_cards].info.bus,
				cards[num_cards].info.device,
				cards[num_cards].info.function,
				DRIVER_NAME, &cards[num_cards])) < B_OK) {
				ITRACE_VV("%s: failed to reserve_device(%d, %d, %d,): %s\n",
					DRIVER_NAME, cards[num_cards].info.bus,
					cards[num_cards].info.device,
					cards[num_cards].info.function, strerror(err));
				continue;
			}

			ice1712Settings_load(&cards[num_cards]);

			if (ice1712HW_setup(&cards[num_cards]) != B_OK) {
			//Vendor_ID and Device_ID has been modified
				ITRACE("Setup of ice1712 %" B_PRIu32 " failed\n", num_cards + 1);
				(*pci->unreserve_device)(cards[num_cards].info.bus,
					cards[num_cards].info.device,
					cards[num_cards].info.function,
					DRIVER_NAME, &cards[num_cards]);
			} else {
				num_cards++;
			}
		}
		i++;
	}

	ITRACE("Succesfully initialised card : %" B_PRIu32 "\n", num_cards);

	if (num_cards == 0) {
		put_module(B_PCI_MODULE_NAME);
		put_module(B_MPU_401_MODULE_NAME);
		return ENODEV;
	}
	return B_OK;
}


extern "C" void
uninit_driver(void)
{
	int ix, cnt = num_cards;

	ITRACE("@@ uninit_driver()\n");

	num_cards = 0;

	for (ix = 0; ix < cnt; ix++) {
		ice1712HW_shutdown(&cards[ix]);
		(*pci->unreserve_device)(cards[ix].info.bus,
			cards[ix].info.device, cards[ix].info.function,
			DRIVER_NAME, &cards[ix]);
	}
	memset(&cards, 0, sizeof(cards));
	put_module(B_MPU_401_MODULE_NAME);
	put_module(B_PCI_MODULE_NAME);
}


extern "C" const char **
publish_devices(void)
{
	int ix = 0;
	ITRACE("@@ publish_devices()\n");

	for (ix = 0; names[ix]; ix++) {
		ITRACE("publish %s\n", names[ix]);
	}
	return (const char **)names;
}


extern "C" device_hooks *
find_device(const char * name)
{
	int ix, midi;

	ITRACE("@@ find_device()\n");

	for (ix = 0; ix < num_cards; ix++) {
		for (midi = 0; midi < MAX_MIDI_INTERFACE; midi++) {
			if (!strcmp(cards[ix].midiItf[midi].name, name)) {
				return &ice1712Midi_hooks;
			}
		}

		if (!strcmp(cards[ix].name, name)) {
			return &ice1712Audio_hooks;
		}
	}
	ITRACE("!!! failed !!!\n");
	return NULL;
}

//ICE1712 driver - Hardware
//-------------------------

int32
ice1712HW_interrupt(void *arg)
{
	ice1712 *ice = (ice1712*)arg;
	uint8 reg8 = 0;
	uint16 reg16 = 0;
	uint32 status = B_UNHANDLED_INTERRUPT;

	// interrupt from DMA PATH
	reg8 = read_mt_uint8(ice, MT_DMA_INT_MASK_STATUS);
	if (reg8 != 0) {
		ice->buffer++;
		ice->played_time = system_time();
		ice->frames_count += ice->buffer_size;

		release_sem_etc(ice->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		write_mt_uint8(ice, MT_DMA_INT_MASK_STATUS, reg8);
		status = B_HANDLED_INTERRUPT;
	}

	// interrupt from Controller Registers
	reg8 = read_ccs_uint8(ice, CCS_INTERRUPT_STATUS);
	if (reg8 != 0) {
		bool ret;
		if (reg8 & CCS_INTERRUPT_MIDI_1) {
			ret = (*mpu401->interrupt_hook)(ice->midiItf[0].mpu401device);
			if (ret) {
				//Do not ack, cause more datas are available
				reg8 &= ~CCS_INTERRUPT_MIDI_1;
			}
		}

		if (reg8 & CCS_INTERRUPT_MIDI_2) {
			ret = (*mpu401->interrupt_hook)(ice->midiItf[1].mpu401device);
			if (ret) {
				//Do not ack, cause more datas are available
				reg8 &= ~CCS_INTERRUPT_MIDI_2;
			}
		}

		if (reg8 != 0) {
			write_ccs_uint8(ice, CCS_INTERRUPT_STATUS, reg8);
			status = B_HANDLED_INTERRUPT;
		}
	}

	// interrupt from DS PATH
	reg16 = read_ds_uint16(ice, DS_DMA_INT_STATUS);
	if (reg16 != 0) {
		//Ack interrupt
		write_ds_uint16(ice, DS_DMA_INT_STATUS, reg16);
		status = B_HANDLED_INTERRUPT;
	}

	return status;
}


status_t
ice1712HW_setup(ice1712 *ice)
{
	int i;
	uint8 reg8 = 0;
	uint16 mute;

	ice->irq = ice->info.u.h0.interrupt_line;
	ice->Controller = ice->info.u.h0.base_registers[0];
	ice->DDMA = ice->info.u.h0.base_registers[1];
	ice->DMA_Path = ice->info.u.h0.base_registers[2];
	ice->Multi_Track = ice->info.u.h0.base_registers[3];

	// Soft Reset
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x81);
	snooze(200000);
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x01);
	snooze(200000);

	read_eeprom(ice, ice->eeprom_data);

	write_ccs_uint8(ice, CCS_SERR_SHADOW, 0x01);

	//Write all configurations register from EEProm
	ice->info.device_id = ice->eeprom_data[E2PROM_MAP_SUBVENDOR_HIGH] << 8
		| ice->eeprom_data[E2PROM_MAP_SUBVENDOR_LOW];
	ice->info.vendor_id = ice->eeprom_data[E2PROM_MAP_SUBDEVICE_HIGH] << 8
		| ice->eeprom_data[E2PROM_MAP_SUBDEVICE_LOW];
	ice->config.product = (ice1712Product)(ice->info.vendor_id << 16
		| ice->info.device_id);
	ITRACE("Product ID : 0x%x\n", ice->config.product);

	write_cci_uint8(ice, CCI_GPIO_WRITE_MASK,
		ice->eeprom_data[E2PROM_MAP_GPIOMASK]);
	write_cci_uint8(ice, CCI_GPIO_DATA,
		ice->eeprom_data[E2PROM_MAP_GPIOSTATE]);
	write_cci_uint8(ice, CCI_GPIO_DIRECTION_CONTROL,
		ice->eeprom_data[E2PROM_MAP_GPIODIR]);

	ITRACE("CCI_GPIO_WRITE_MASK : 0x%x\n",
		ice->eeprom_data[E2PROM_MAP_GPIOMASK]);
	ITRACE("CCI_GPIO_DATA : 0x%x\n",
		ice->eeprom_data[E2PROM_MAP_GPIOSTATE]);
	ITRACE("CCI_GPIO_DIRECTION_CONTROL : 0x%x\n",
		ice->eeprom_data[E2PROM_MAP_GPIODIR]);

	//Write Configuration in the PCI configuration Register
	(pci->write_pci_config)(ice->info.bus, ice->info.device,
		ice->info.function, 0x60, 1, ice->eeprom_data[E2PROM_MAP_CONFIG]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device,
		ice->info.function, 0x61, 1, ice->eeprom_data[E2PROM_MAP_ACL]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device,
		ice->info.function, 0x62, 1, ice->eeprom_data[E2PROM_MAP_I2S]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device,
		ice->info.function, 0x63, 1, ice->eeprom_data[E2PROM_MAP_SPDIF]);

	ITRACE("E2PROM_MAP_CONFIG : 0x%x\n", ice->eeprom_data[E2PROM_MAP_CONFIG]);
	reg8 = ice->eeprom_data[E2PROM_MAP_CONFIG];
	//Bits signification for E2PROM_MAP_CONFIG Byte
	//
	// 8   7   6   5   4   3   2   1   0
	//           |-D-|-C-|---B---|---A---
	//
	// D : MPU401 number minus 1
	// C : AC'97
	// B : Stereo ADC number minus 1 (=> 1 to 4)
	// A : Stereo DAC number minus 1 (=> 1 to 4)

	ice->config.nb_DAC = ((reg8 & 0x03) + 1) * 2;
	reg8 >>= 2;
	ice->config.nb_ADC = ((reg8 & 0x03) + 1) * 2;
	reg8 >>= 2;

	if ((reg8 & 0x01) != 0) {//Consumer AC'97 Exist
		ITRACE("Consumer AC'97 does exist\n");
		//For now do nothing
/*		write_ccs_uint8(ice, CCS_CONS_AC97_COMMAND_STATUS, 0x40);
		snooze(10000);
		write_ccs_uint8(ice, CCS_CONS_AC97_COMMAND_STATUS, 0x00);
		snooze(20000);
*/	} else {
		ITRACE("Consumer AC'97 does NOT exist\n");
	}
	reg8 >>= 1;
	ice->config.nb_MPU401 = (reg8 & 0x1) + 1;

	if (ice->config.nb_MPU401 > 0) {
		sprintf(ice->midiItf[0].name, "midi/ice1712/%ld/1", ice - cards + 1);

		(*mpu401->create_device)(ice->Controller + CCS_MIDI_1_DATA,
			&ice->midiItf[0].mpu401device,
			0x14121712,
			ice1712Midi_interrupt,
			&ice->midiItf[0]);

		names[num_names++] = ice->midiItf[0].name;
		ice->midiItf[0].card = ice;
		ice->midiItf[0].int_mask = CCS_INTERRUPT_MIDI_1;
	}

	if (ice->config.nb_MPU401 > 1) {
		sprintf(ice->midiItf[1].name, "midi/ice1712/%ld/2", ice - cards + 1);

		(*mpu401->create_device)(ice->Controller + CCS_MIDI_2_DATA,
			&ice->midiItf[1].mpu401device,
			0x14121712,
			ice1712Midi_interrupt,
			&ice->midiItf[1]);

		names[num_names++] = ice->midiItf[1].name;
		ice->midiItf[1].card = ice;
		ice->midiItf[1].int_mask = CCS_INTERRUPT_MIDI_2;
	}

	ITRACE("E2PROM_MAP_SPDIF : 0x%x\n", ice->eeprom_data[E2PROM_MAP_SPDIF]);
	ice->config.spdif = ice->eeprom_data[E2PROM_MAP_SPDIF];

	switch (ice->config.product) {
		case ICE1712_SUBDEVICE_DELTA66 :
		case ICE1712_SUBDEVICE_DELTA44 :
			ice->CommLines.clock = DELTA66_CLK;
			ice->CommLines.data_in = 0;
			ice->CommLines.data_out = DELTA66_DOUT;
			ice->CommLines.cs_mask = DELTA66_CLK | DELTA66_DOUT
				| DELTA66_CS_MASK;
			break;
		case ICE1712_SUBDEVICE_DELTA410 :
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496 :
		case ICE1712_SUBDEVICE_DELTADIO2496 :
			ice->CommLines.clock = AP2496_CLK;
			ice->CommLines.data_in = AP2496_DIN;
			ice->CommLines.data_out = AP2496_DOUT;
			ice->CommLines.cs_mask = AP2496_CLK | AP2496_DIN
				| AP2496_DOUT | AP2496_CS_MASK;
			break;
		case ICE1712_SUBDEVICE_DELTA1010 :
		case ICE1712_SUBDEVICE_DELTA1010LT :
			ice->CommLines.clock = DELTA1010LT_CLK;
			ice->CommLines.data_in = DELTA1010LT_DIN;
			ice->CommLines.data_out = DELTA1010LT_DOUT;
			ice->CommLines.cs_mask = DELTA1010LT_CLK | DELTA1010LT_DIN
				| DELTA1010LT_DOUT | DELTA1010LT_CS_NONE;
			break;
		case ICE1712_SUBDEVICE_VX442 :
			ice->CommLines.clock = VX442_CLK;
			ice->CommLines.data_in = VX442_DIN;
			ice->CommLines.data_out = VX442_DOUT;
			ice->CommLines.cs_mask = VX442_CLK | VX442_DIN | VX442_DOUT
				| VX442_CS_MASK;
			break;
	}

	sprintf(ice->name, "%s/%ld", HMULTI_AUDIO_DEV_PATH, ice - cards + 1);
	names[num_names++] = ice->name;
	names[num_names] = NULL;

	ice->buffer_ready_sem = create_sem(0, "Buffer Exchange");
	if (ice->buffer_ready_sem < B_OK) {
		return ice->buffer_ready_sem;
	}

	install_io_interrupt_handler(ice->irq, ice1712HW_interrupt, ice, 0);

	ice->mem_id_pb = alloc_mem(&ice->phys_pb, &ice->log_addr_pb,
		PLAYBACK_BUFFER_TOTAL_SIZE, "playback buffer");
	if (ice->mem_id_pb < B_OK) {
		remove_io_interrupt_handler(ice->irq, ice1712HW_interrupt, ice);
		delete_sem(ice->buffer_ready_sem);
		return ice->mem_id_pb;
	}

	ice->mem_id_rec = alloc_mem(&ice->phys_rec, &ice->log_addr_rec,
		RECORD_BUFFER_TOTAL_SIZE, "record buffer");
	if (ice->mem_id_rec < B_OK) {
		remove_io_interrupt_handler(ice->irq, ice1712HW_interrupt, ice);
		delete_sem(ice->buffer_ready_sem);
		delete_area(ice->mem_id_pb);
		return(ice->mem_id_rec);
	}

	ice->config.samplingRate = 0x08;
	ice->buffer = 0;
	ice->frames_count = 0;
	ice->buffer_size = ice->settings.bufferSize;

	ice->total_output_channels = ice->config.nb_DAC;
	if (ice->config.spdif & SPDIF_OUT_PRESENT)
		ice->total_output_channels += 2;

	ice->total_input_channels = ice->config.nb_ADC + 2;
	if (ice->config.spdif & SPDIF_IN_PRESENT)
		ice->total_input_channels += 2;

	//Write bits in the GPIO
	write_cci_uint8(ice, CCI_GPIO_WRITE_MASK, ~(ice->CommLines.cs_mask));
	//Deselect CS
	write_cci_uint8(ice, CCI_GPIO_DATA, ice->CommLines.cs_mask);

	//Set the rampe volume to a faster one
	write_mt_uint16(ice, MT_VOLUME_CONTROL_RATE, 0x01);

	//All Analog outputs from DMA
	write_mt_uint16(ice, MT_ROUTING_CONTROL_PSDOUT, 0x0000);
	//All Digital output from DMA
	write_mt_uint16(ice, MT_ROUTING_CONTROL_SPDOUT, 0x0000);

	//Mute all input
	mute = (ICE1712_MUTE_VALUE << 0) | (ICE1712_MUTE_VALUE << 8);
	for (i = 0; i < 2 * ICE1712_HARDWARE_VOLUME; i++) {
		write_mt_uint8(ice, MT_VOLUME_CONTROL_CHANNEL_INDEX, i);
		write_mt_uint16(ice, MT_VOLUME_CONTROL_CHANNEL_INDEX, mute);
	}

	//Unmask Interrupt
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x41);

	reg8 = read_ccs_uint8(ice, CCS_INTERRUPT_MASK);
	ITRACE("-----CCS----- = %x\n", reg8);
	write_ccs_uint8(ice, CCS_INTERRUPT_MASK, 0xEF);

/*	reg16 = read_ds_uint16(ice, DS_DMA_INT_MASK);
	ITRACE("-----DS_DMA----- = %x\n", reg16);
	write_ds_uint16(ice, DS_DMA_INT_MASK, 0x0000);
*/
	reg8 = read_mt_uint8(ice, MT_DMA_INT_MASK_STATUS);
	ITRACE("-----MT_DMA----- = %x\n", reg8);
	write_mt_uint8(ice, MT_DMA_INT_MASK_STATUS, 0x00);

	return B_OK;
};


void
ice1712HW_shutdown(ice1712 *ice)
{
	delete_sem(ice->buffer_ready_sem);

	remove_io_interrupt_handler(ice->irq, ice1712HW_interrupt, ice);

	if (ice->mem_id_pb != B_ERROR)
		delete_area(ice->mem_id_pb);

	if (ice->mem_id_rec != B_ERROR)
		delete_area(ice->mem_id_rec);

	codec_write(ice, AK45xx_RESET_REGISTER, 0x00);
}

//ICE1712 driver - Hook
//---------------------

static status_t
ice1712Audio_open(const char *name, uint32 flags, void **cookie)
{
	int ix;
	ice1712 *card = NULL;
	ITRACE("** open(): %s\n", name);

	for (ix = 0; ix<num_cards; ix++) {
		if (!strcmp(cards[ix].name, name)) {
			card = &cards[ix];
		}
	}

	if (card == NULL) {
		ITRACE("open() card not found %s\n", name);
		for (ix=0; ix<num_cards; ix++) {
			ITRACE("open() card available %s\n", cards[ix].name);
		}
		return B_ERROR;
	}
	*cookie = cards;
	return B_OK;
}


static status_t
ice1712Audio_close(void *cookie)
{
	ITRACE("** close()\n");
	return B_OK;
}


static status_t
ice1712Audio_free(void *cookie)
{
	ITRACE("** free()\n");
	return B_OK;
}


static status_t
ice1712Audio_control(void *cookie, uint32 op, void *arg, size_t len)
{
	switch (op) {
		case B_MULTI_GET_DESCRIPTION :
			ITRACE("B_MULTI_GET_DESCRIPTION\n");
			return ice1712Get_Description((ice1712 *)cookie,
				(multi_description*)arg);
		case B_MULTI_GET_EVENT_INFO :
			ITRACE("B_MULTI_GET_EVENT_INFO\n");
			return B_ERROR;
		case B_MULTI_SET_EVENT_INFO :
			ITRACE("B_MULTI_SET_EVENT_INFO\n");
			return B_ERROR;
		case B_MULTI_GET_EVENT :
			ITRACE("B_MULTI_GET_EVENT\n");
			return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS :
			ITRACE("B_MULTI_GET_ENABLED_CHANNELS\n");
			return ice1712Get_Channel((ice1712*)cookie,
				(multi_channel_enable*)arg);
		case B_MULTI_SET_ENABLED_CHANNELS :
			ITRACE("B_MULTI_SET_ENABLED_CHANNELS\n");
			return ice1712Set_Channel((ice1712*)cookie,
				(multi_channel_enable*)arg);
		case B_MULTI_GET_GLOBAL_FORMAT :
			ITRACE("B_MULTI_GET_GLOBAL_FORMAT\n");
			return ice1712Get_Format((ice1712*)cookie,
				(multi_format_info *)arg);
		case B_MULTI_SET_GLOBAL_FORMAT :
			ITRACE("B_MULTI_SET_GLOBAL_FORMAT\n");
			return ice1712Set_Format((ice1712*)cookie,
				(multi_format_info *)arg);
		case B_MULTI_GET_CHANNEL_FORMATS :
			ITRACE("B_MULTI_GET_CHANNEL_FORMATS\n");
			return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS :
			ITRACE("B_MULTI_SET_CHANNEL_FORMATS\n");
			return B_ERROR;
		case B_MULTI_GET_MIX :
			ITRACE("B_MULTI_GET_MIX\n");
			return ice1712Get_MixValue((ice1712*)cookie,
				(multi_mix_value_info *)arg);
		case B_MULTI_SET_MIX :
			ITRACE("B_MULTI_SET_MIX\n");
			return ice1712Set_MixValue((ice1712*)cookie,
				(multi_mix_value_info *)arg);
		case B_MULTI_LIST_MIX_CHANNELS :
			ITRACE("B_MULTI_LIST_MIX_CHANNELS\n");
			return ice1712Get_MixValueChannel((ice1712*)cookie,
				(multi_mix_channel_info *)arg);
		case B_MULTI_LIST_MIX_CONTROLS :
			ITRACE("B_MULTI_LIST_MIX_CONTROLS\n");
			return ice1712Get_MixValueControls((ice1712*)cookie,
				(multi_mix_control_info *)arg);
		case B_MULTI_LIST_MIX_CONNECTIONS :
			ITRACE("B_MULTI_LIST_MIX_CONNECTIONS\n");
			return ice1712Get_MixValueConnections((ice1712*)cookie,
				(multi_mix_connection_info *)arg);
		case B_MULTI_GET_BUFFERS :
			ITRACE("B_MULTI_GET_BUFFERS\n");
			return ice1712Buffer_Get((ice1712*)cookie,
				(multi_buffer_list*)arg);
		case B_MULTI_SET_BUFFERS :
			ITRACE("B_MULTI_SET_BUFFERS\n");
			return B_ERROR;
		case B_MULTI_SET_START_TIME :
			ITRACE("B_MULTI_SET_START_TIME\n");
			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE :
//			ITRACE("B_MULTI_BUFFER_EXCHANGE\n");
			return ice1712Buffer_Exchange((ice1712*)cookie,
				(multi_buffer_info *)arg);
		case B_MULTI_BUFFER_FORCE_STOP :
			ITRACE("B_MULTI_BUFFER_FORCE_STOP\n");
			return ice1712Buffer_Stop((ice1712*)cookie);
		case B_MULTI_LIST_EXTENSIONS :
			ITRACE("B_MULTI_LIST_EXTENSIONS\n");
			return B_ERROR;
		case B_MULTI_GET_EXTENSION :
			ITRACE("B_MULTI_GET_EXTENSION\n");
			return B_ERROR;
		case B_MULTI_SET_EXTENSION :
			ITRACE("B_MULTI_SET_EXTENSION\n");
			return B_ERROR;
		case B_MULTI_LIST_MODES :
			ITRACE("B_MULTI_LIST_MODES\n");
			return B_ERROR;
		case B_MULTI_GET_MODE :
			ITRACE("B_MULTI_GET_MODE\n");
			return B_ERROR;
		case B_MULTI_SET_MODE :
			ITRACE("B_MULTI_SET_MODE\n");
			return B_ERROR;

		default :
			ITRACE("ERROR: unknown multi_control %#x\n", (int)op);
			return B_ERROR;
	}
}


static status_t
ice1712Audio_read(void *cookie, off_t position, void *buf,
	size_t *num_bytes)
{
	ITRACE("** read()\n");
	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t
ice1712Audio_write(void *cookie, off_t position, const void *buffer,
	size_t *num_bytes)
{
	ITRACE("** write()\n");
	*num_bytes = 0;
	return B_IO_ERROR;
}


device_hooks ice1712Audio_hooks =
{
	ice1712Audio_open,
	ice1712Audio_close,
	ice1712Audio_free,
	ice1712Audio_control,
	ice1712Audio_read,
	ice1712Audio_write,
	NULL,
	NULL,
	NULL,
	NULL
};


//ICE1712 Drivers - Settings
//--------------------------


status_t
ice1712Settings_load(ice1712 *card)
{
	// get driver settings
	void *settings_handle = load_driver_settings("ice1712.settings");

	//Use a large enough value for modern computer
	card->settings.bufferSize = 512;

	if (settings_handle != NULL) {
		const char *item;
		char *end;

		item = get_driver_parameter(settings_handle,
			"buffer_size", "512", "512");
		if (item != NULL) {
			uint32 value = strtoul(item, &end, 0);
			if ((*end == '\0')
				&& (value >= MIN_BUFFER_FRAMES)
				&& (value <= MAX_BUFFER_FRAMES)) {
					card->settings.bufferSize = value;
			}
		}

		unload_driver_settings(settings_handle);
	}

	return B_OK;
}


status_t
ice1712Settings_apply(ice1712 *card)
{
	int i;
	uint16 val, mt30 = 0;
	uint32 mt34 = 0;

	for (i = 0; i < ICE1712_HARDWARE_VOLUME; i++) {
		//Select the channel
		write_mt_uint8(card, MT_VOLUME_CONTROL_CHANNEL_INDEX, i);

		if (card->settings.playback[i].mute == true) {
			val = (ICE1712_MUTE_VALUE << 0) | (ICE1712_MUTE_VALUE << 8);
		} else {
			uint8 volume = card->settings.playback[i].volume / -1.5;
			if (i & 1) {//a right channel
				val = ICE1712_MUTE_VALUE << 0; //Mute left volume
				val |= volume << 8;
			} else {//a left channel
				val = ICE1712_MUTE_VALUE << 8; //Mute right volume
				val |= volume << 0;
			}
		}

		write_mt_uint16(card, MT_LR_VOLUME_CONTROL,	val);
		ITRACE_VV("Apply Settings %d : 0x%x\n", i, val);
	}

	for (i = 0; i < ICE1712_HARDWARE_VOLUME; i++) {
		//Select the channel
		write_mt_uint8(card, MT_VOLUME_CONTROL_CHANNEL_INDEX,
			i + ICE1712_HARDWARE_VOLUME);

		if (card->settings.record[i].mute == true) {
			val = (ICE1712_MUTE_VALUE << 0) | (ICE1712_MUTE_VALUE << 8);
		} else {
			uint8 volume = card->settings.record[i].volume / -1.5;
			if (i & 1) {//a right channel
				val = ICE1712_MUTE_VALUE << 0; //Mute left volume
				val |= volume << 8;
			} else {//a left channel
				val = ICE1712_MUTE_VALUE << 8; //Mute right volume
				val |= volume << 0;
			}
		}

		write_mt_uint16(card, MT_LR_VOLUME_CONTROL,	val);
		ITRACE_VV("Apply Settings %d : 0x%x\n", i, val);
	}

	//Analog output selection
	for (i = 0; i < 4; i++) {
		uint8 out = card->settings.output[i];
		if (out == 0) {
			ITRACE_VV("Output %d is haiku output\n", i);
			//Nothing to do
		} else if (out <= (card->config.nb_ADC / 2)) {
			uint8 mt34_c;
			out--;
			ITRACE_VV("Output %d is input %d\n", i, out);
			mt34_c = (out * 2);
			mt34_c |= (out * 2 + 1) << 4;
			mt30 |= 0x0202 << (2 * i);
			mt30 |= mt34_c << (8 * i);
		} else if (out == ((card->config.nb_ADC / 2) + 1)
				&& (card->config.spdif & SPDIF_IN_PRESENT) != 0) {
			ITRACE_VV("Output %d is digital input\n", i);
			mt30 |= 0x0303 << (2 * i);
			mt34 |= 0x80 << (8 * i);
		} else {
			ITRACE_VV("Output %d is digital Mixer\n", i);
			mt30 |= 0x0101;
		}
	}
	write_mt_uint16(card, MT_ROUTING_CONTROL_PSDOUT, mt30);
	write_mt_uint32(card, MT_CAPTURED_DATA, mt34);

	//Digital output
	if ((card->config.spdif & SPDIF_OUT_PRESENT) != 0) {
		uint16 mt32 = 0;
		uint8 out = card->settings.output[4];
		if (out == 0) {
			ITRACE_VV("Digital output is haiku output\n");
			//Nothing to do
		} else if (out <= (card->config.nb_ADC / 2)) {
			out--;
			ITRACE_VV("Digital output is input %d\n", out);
			mt32 |= 0x0202;
			mt32 |= (out * 2) << 8;
			mt32 |= (out * 2 + 1) << 12;
		} else if (out == ((card->config.nb_ADC / 2) + 1)
				&& (card->config.spdif & SPDIF_IN_PRESENT) != 0) {
			ITRACE_VV("Digital output is digital input\n");
			mt32 |= 0x800F;
		} else {
			ITRACE_VV("Digital output is digital Mixer\n");
			mt32 |= 0x0005;
		}

		write_mt_uint16(card, MT_ROUTING_CONTROL_SPDOUT, mt32);
	}

	return B_OK;
}
