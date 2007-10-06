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

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include <malloc.h>
#include <fcntl.h>
#include <image.h>
#include <string.h>
#include <PCI.h>

#include "debug.h"
#include "ice1712.h"
#include "ice1712_reg.h"
#include "io.h"
#include "midi_driver.h"
#include "multi.h"
#include "multi_audio.h"
#include "util.h"


//------------------------------------------------------
//------------------------------------------------------

status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char **publish_devices(void);
device_hooks *find_device(const char *);

//------------------------------------------------------

int32 ice_1712_int(void *arg);

//------------------------------------------------------

static char pci_name[] = B_PCI_MODULE_NAME;
pci_module_info *pci;
static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module *mpu401;

int32 num_cards = 0;
ice1712 cards[NUM_CARDS];
int32 num_names = 0;
char *names[NUM_CARDS*20+1];

//------------------------------------------------------
//------------------------------------------------------

int32 api_version = B_CUR_DRIVER_API_VERSION;

#define MODULE_TEST_PATH "audio/multi/ice1712"

//------------------------------------------------------
//------------------------------------------------------

//void republish_devices(void);

//extern image_id	load_kernel_addon(const char *path);
//extern status_t	unload_kernel_addon(image_id imid);

//------------------------------------------------------
//------------------------------------------------------

status_t init_hardware(void)
{
	int ix = 0;
	pci_info info;

	memset(cards, 0, sizeof(ice1712) * NUM_CARDS);
	LOG_CREATE_ICE();
	TRACE_ICE(("===init_hardware()===\n"));
	
	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;
	
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if ((info.vendor_id == ICE1712_VENDOR_ID) && 
			(info.device_id == ICE1712_DEVICE_ID)) {
			TRACE_ICE(("Found at least 1 card\n"));
			put_module(pci_name);
			return B_OK;
		}
		ix++;
	}
	put_module(pci_name);
	return B_ERROR;
}


int32 
ice_1712_int(void *arg)
{
	ice1712 *ice = arg;
	uint8 reg8 = 0;
	uint16 reg16 = 0;
	uint32 status = B_UNHANDLED_INTERRUPT;

// interrupt from DMA PATH
	reg8 = read_mt_uint8(ice, MT_DMA_INT_MASK_STATUS);
	if (reg8 != 0) {
		ice->buffer++;
		ice->played_time = system_time();

		release_sem_etc(ice->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		write_mt_uint8(ice, MT_DMA_INT_MASK_STATUS, reg8);
		status = B_HANDLED_INTERRUPT;
	}

// interrupt from Controller Registers
	reg8 = read_ccs_uint8(ice, CCS_INTERRUPT_STATUS);
	if (reg8 != 0) {
		write_ccs_uint8(ice, CCS_INTERRUPT_STATUS, reg8);
		status = B_HANDLED_INTERRUPT;
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


static status_t 
ice1712_setup(ice1712 *ice)
{
	int result, i;
	long result_l = 0;
	uint8 eeprom_data[32];
	uint8 reg8 = 0;

	ice->irq			= ice->info.u.h0.interrupt_line;
	ice->Controller		= ice->info.u.h0.base_registers[0];
	ice->DDMA			= ice->info.u.h0.base_registers[1];
	ice->DMA_Path		= ice->info.u.h0.base_registers[2];
	ice->Multi_Track	= ice->info.u.h0.base_registers[3];
	
	// Soft Reset
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x81);
	snooze(200000);
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x01);
	snooze(200000);

	result = read_eeprom(ice, eeprom_data);

	TRACE_ICE(("EEprom -> "));
	for (i = 0; i < 32; i++)
		TRACE_ICE(("%x, ", eeprom_data[i]));
	TRACE_ICE(("<- EEprom\n"));

	write_ccs_uint8(ice, CCS_SERR_SHADOW, 0x01);

	//Write all configurations register from EEProm

	//Write enable SVID
//	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x42, 2, 0x86);
	//Change SVID
	ice->info.device_id = eeprom_data[1] << 8 | eeprom_data[0];
	ice->info.vendor_id = eeprom_data[3] << 8 | eeprom_data[2];
	ice->product = ice->info.vendor_id << 16 | ice->info.device_id;
//	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x2C, 4, ice->product);

	//Write Disable SVID
//	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x42, 2, 0x06);

	TRACE_ICE(("Product ID : 0x%x\n", ice->product));

	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x60, 1, eeprom_data[6]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x61, 1, eeprom_data[7]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x62, 1, eeprom_data[8]);
	(pci->write_pci_config)(ice->info.bus, ice->info.device, ice->info.function, 0x63, 1, eeprom_data[9]);

	write_cci_uint8(ice, CCI_GPIO_WRITE_MASK,			eeprom_data[10]);
	write_cci_uint8(ice, CCI_GPIO_DATA,					eeprom_data[11]);
	write_cci_uint8(ice, CCI_GPIO_DIRECTION_CONTROL,	eeprom_data[12]);
	write_cci_uint8(ice, CCI_CONS_POWER_DOWN,			eeprom_data[13]);
	write_cci_uint8(ice, CCI_MULTI_POWER_DOWN,			eeprom_data[14]);

	ice->nb_MPU401 = ((eeprom_data[6] & 0x20) >> 5) + 1;
	ice->nb_ADC = (((eeprom_data[6] & 0x0C) >> 2) + 1 ) * 2;
	ice->nb_DAC = ((eeprom_data[6] & 0x03) + 1) * 2;
	ice->spdif_config = eeprom_data[9] & 0x03;

	for (i = 0; i < ice->nb_MPU401; i++) {
		sprintf(ice->midi_interf[i].name, "midi/ice1712/%ld/%d", ice - cards + 1, i + 1);
		names[num_names++] = ice->midi_interf[i].name;
	}
	
	switch (ice->product)
	{
		case ICE1712_SUBDEVICE_DELTA66 :
		case ICE1712_SUBDEVICE_DELTA44 :
			ice->gpio_cs_mask = DELTA66_CLK | DELTA66_DOUT | DELTA66_CODEC_CS_0 | DELTA66_CODEC_CS_1;
			ice->analog_codec = (codec_info){DELTA66_CLK, 0, DELTA66_DOUT};
			break;
		case ICE1712_SUBDEVICE_DELTA410 :
		case ICE1712_SUBDEVICE_AUDIOPHILE_2496 :
		case ICE1712_SUBDEVICE_DELTADIO2496 :
			ice->gpio_cs_mask = AP2496_CLK | AP2496_DIN | AP2496_DOUT | AP2496_SPDIF_CS | AP2496_CODEC_CS;
			ice->analog_codec = (codec_info){AP2496_CLK, AP2496_DIN, AP2496_DOUT};
			break;
		case ICE1712_SUBDEVICE_DELTA1010 :
		case ICE1712_SUBDEVICE_DELTA1010LT :
			ice->gpio_cs_mask = DELTA1010LT_CLK | DELTA1010LT_DIN | DELTA1010LT_DOUT | DELTA1010LT_CS_NONE;
			ice->analog_codec = (codec_info){DELTA1010LT_CLK, DELTA1010LT_DIN, DELTA1010LT_DOUT};
			break;
		case ICE1712_SUBDEVICE_VX442 :
			ice->gpio_cs_mask = VX442_SPDIF_CS | VX442_CODEC_CS_0 | VX442_CODEC_CS_1;
			ice->analog_codec = (codec_info){VX442_CLK, VX442_DIN, VX442_DOUT};
			break;
	}

	sprintf(ice->name, "%s/%ld", MODULE_TEST_PATH, ice - cards + 1);
	names[num_names++] = ice->name;
	names[num_names] = NULL;
	
	if ((eeprom_data[6] & 0x10) == 0) {//Consumer AC'97 Exist
		TRACE_ICE(("Consumer AC'97 does exist\n"));
		write_ccs_uint8(ice, CCS_CONS_AC97_COMMAND_STATUS, 0x40);
		snooze(10000);
		write_ccs_uint8(ice, CCS_CONS_AC97_COMMAND_STATUS, 0x00);
		snooze(20000);
	} else {
		TRACE_ICE(("Consumer AC'97 does NOT exist\n"));
	}
	
	ice->buffer_ready_sem = create_sem(0, "Buffer Exchange");
	
//	TRACE_ICE(("installing interrupt : %0x\n", ice->irq));
	result_l = install_io_interrupt_handler(ice->irq, ice_1712_int, ice, 0);
	if (result_l == B_OK)
		TRACE_ICE(("Install Interrupt Handler == B_OK\n"));
	else
		TRACE_ICE(("Install Interrupt Handler != B_OK\n"));
	
	ice->mem_id_pb = alloc_mem(&ice->phys_addr_pb, &ice->log_addr_pb, 
								PLAYBACK_BUFFER_TOTAL_SIZE,
								"playback buffer");

	ice->mem_id_rec = alloc_mem(&ice->phys_addr_rec, &ice->log_addr_rec, 
								RECORD_BUFFER_TOTAL_SIZE,
								"record buffer");

	memset(ice->log_addr_pb, 0, PLAYBACK_BUFFER_TOTAL_SIZE);
	memset(ice->log_addr_rec, 0, RECORD_BUFFER_TOTAL_SIZE);
	
	ice->sampling_rate = 0x08;
	ice->buffer = 0;
	ice->output_buffer_size = MAX_BUFFER_FRAMES;
	ice->input_buffer_size = MAX_BUFFER_FRAMES;
	
	ice->total_output_channels = ice->nb_DAC;
	if (ice->spdif_config & NO_IN_YES_OUT)
		ice->total_output_channels += 2;
		
	ice->total_input_channels = ice->nb_ADC + 2;
	if (ice->spdif_config & YES_IN_NO_OUT)
		ice->total_input_channels += 2;

	//Write bits in the GPIO
	write_cci_uint8(ice, CCI_GPIO_WRITE_MASK, ~(ice->gpio_cs_mask));
	//Deselect CS
	write_cci_uint8(ice, CCI_GPIO_DATA, ice->gpio_cs_mask);

	//Just to route input to output
//	write_mt_uint16(ice, MT_ROUTING_CONTROL_PSDOUT,	0x0101);
//	write_mt_uint32(ice, MT_CAPTURED_DATA,	0x0000);

	//Unmask Interrupt
	write_ccs_uint8(ice, CCS_CONTROL_STATUS, 0x41);

	reg8 = read_ccs_uint8(ice, CCS_INTERRUPT_MASK);
	TRACE_ICE(("-----CCS----- = %x\n", reg8));
	write_ccs_uint8(ice, CCS_INTERRUPT_MASK, 0x6F);

/*	reg16 = read_ds_uint16(ice, DS_DMA_INT_MASK);
	TRACE_ICE(("-----DS_DMA----- = %x\n", reg16));
	write_ds_uint16(ice, DS_DMA_INT_MASK, 0x0000);
*/
	reg8 = read_mt_uint8(ice, MT_DMA_INT_MASK_STATUS);
	TRACE_ICE(("-----MT_DMA----- = %x\n", reg8));
	write_mt_uint8(ice, MT_DMA_INT_MASK_STATUS, 0x00);

	return B_OK;
};


status_t 
init_driver(void)
{
	int i = 0;
	num_cards = 0;

	TRACE_ICE(("===init_driver()===\n"));

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	if (get_module(mpu401_name, (module_info **) &mpu401)) {
		put_module(pci_name);
		return ENOSYS;
	}

	while ((*pci->get_nth_pci_info)(i, &cards[num_cards].info) == B_OK)
	{//TODO check other Vendor_ID and DEVICE_ID
		if ((cards[num_cards].info.vendor_id == ICE1712_VENDOR_ID) && 
			(cards[num_cards].info.device_id == ICE1712_DEVICE_ID))
		{
			if (num_cards == NUM_CARDS)
			{
				TRACE_ICE(("Too many ice1712 cards installed!\n"));
				break;
			}

			if (ice1712_setup(&cards[num_cards]) != B_OK)
			//Vendor_ID and Device_ID has been modified
			{
				TRACE_ICE(("Setup of ice1712 %d failed\n", num_cards + 1));
			}
			else
			{
				num_cards++;
			}
		}
		i++;
	}
	
	TRACE_ICE(("Number of succesfully initialised card : %d\n", num_cards));

	if (num_cards == 0) {
		put_module(pci_name);
		put_module(mpu401_name);
		return ENODEV;
	}	
	return B_OK;
}


static void 
ice_1712_shutdown(ice1712 *ice)
{
	long result_l;

	result_l = remove_io_interrupt_handler(ice->irq, ice_1712_int, ice);
	if (result_l == B_OK)
		TRACE_ICE(("remove Interrupt result == B_OK\n"));
	else
		TRACE_ICE(("remove Interrupt result != B_OK\n"));

	if (ice->mem_id_pb != B_ERROR)
		delete_area(ice->mem_id_pb);

	if (ice->mem_id_rec != B_ERROR)
		delete_area(ice->mem_id_rec);
	
	codec_write(ice, AK45xx_RESET_REGISTER, 0x00);
}


void 
uninit_driver(void)
{
	int ix, cnt = num_cards;

	TRACE_ICE(("===uninit_driver()===\n"));

	num_cards = 0;

	for (ix = 0; ix < cnt; ix++) {
		ice_1712_shutdown(&cards[ix]);
	}
	memset(&cards, 0, sizeof(cards));
	put_module(mpu401_name);
	put_module(pci_name);
}

//------------------------------------------------------

const char **
publish_devices(void)
{
	int ix = 0;
	TRACE_ICE(("===publish_devices()===\n"));

	for (ix=0; names[ix]; ix++) {
		TRACE_ICE(("publish %s\n", names[ix]));
	}
	return (const char **)names;
}


static status_t 
ice_1712_open(const char *name, uint32 flags, void **cookie)
{//Not Implemented (partialy only)
	TRACE_ICE(("===open()===\n"));
	*cookie = cards;
	return B_OK;
}


static status_t 
ice_1712_close(void *cookie)
{
	TRACE_ICE(("===close()===\n"));
	return B_ERROR;
}


static status_t 
ice_1712_free(void *cookie)
{
	TRACE_ICE(("===free()===\n"));
	return B_ERROR;
}


static status_t 
ice_1712_control(void *cookie, uint32 op, void *arg, size_t len)
{
	switch (op)
	{
		case B_MULTI_GET_DESCRIPTION :
			TRACE_ICE(("B_MULTI_GET_DESCRIPTION\n"));
			return ice1712_get_description((ice1712 *)cookie, (multi_description*)arg);
		case B_MULTI_GET_EVENT_INFO :
			TRACE_ICE(("B_MULTI_GET_EVENT_INFO\n"));
			return B_ERROR;
		case B_MULTI_SET_EVENT_INFO :
			TRACE_ICE(("B_MULTI_SET_EVENT_INFO\n"));
			return B_ERROR;
		case B_MULTI_GET_EVENT :
			TRACE_ICE(("B_MULTI_GET_EVENT\n"));
			return B_ERROR;
		case B_MULTI_GET_ENABLED_CHANNELS :
			TRACE_ICE(("B_MULTI_GET_ENABLED_CHANNELS\n"));
			return ice1712_get_enabled_channels((ice1712*)cookie, (multi_channel_enable*)arg);
		case B_MULTI_SET_ENABLED_CHANNELS :
			TRACE_ICE(("B_MULTI_SET_ENABLED_CHANNELS\n"));
			return ice1712_set_enabled_channels((ice1712*)cookie, (multi_channel_enable*)arg);
		case B_MULTI_GET_GLOBAL_FORMAT :
			TRACE_ICE(("B_MULTI_GET_GLOBAL_FORMAT\n"));
			return ice1712_get_global_format((ice1712*)cookie, (multi_format_info *)arg);
		case B_MULTI_SET_GLOBAL_FORMAT :
			TRACE_ICE(("B_MULTI_SET_GLOBAL_FORMAT\n"));
			return ice1712_set_global_format((ice1712*)cookie, (multi_format_info *)arg);
		case B_MULTI_GET_CHANNEL_FORMATS :
			TRACE_ICE(("B_MULTI_GET_CHANNEL_FORMATS\n"));
			return B_ERROR;
		case B_MULTI_SET_CHANNEL_FORMATS :
			TRACE_ICE(("B_MULTI_SET_CHANNEL_FORMATS\n"));
			return B_ERROR;
		case B_MULTI_GET_MIX :
			TRACE_ICE(("B_MULTI_GET_MIX\n"));
			return ice1712_get_mix((ice1712*)cookie, (multi_mix_value_info *)arg);
		case B_MULTI_SET_MIX :
			TRACE_ICE(("B_MULTI_SET_MIX\n"));
			return ice1712_set_mix((ice1712*)cookie, (multi_mix_value_info *)arg);
		case B_MULTI_LIST_MIX_CHANNELS :
			TRACE_ICE(("B_MULTI_LIST_MIX_CHANNELS\n"));
			return ice1712_list_mix_channels((ice1712*)cookie, (multi_mix_channel_info *)arg);
		case B_MULTI_LIST_MIX_CONTROLS :
			TRACE_ICE(("B_MULTI_LIST_MIX_CONTROLS\n"));
			return ice1712_list_mix_controls((ice1712*)cookie, (multi_mix_control_info *)arg);
		case B_MULTI_LIST_MIX_CONNECTIONS :
			TRACE_ICE(("B_MULTI_LIST_MIX_CONNECTIONS\n"));
			return ice1712_list_mix_connections((ice1712*)cookie, (multi_mix_connection_info *)arg);
		case B_MULTI_GET_BUFFERS :
			TRACE_ICE(("B_MULTI_GET_BUFFERS\n"));
			return ice1712_get_buffers((ice1712*)cookie, (multi_buffer_list*)arg);
		case B_MULTI_SET_BUFFERS :
			TRACE_ICE(("B_MULTI_SET_BUFFERS\n"));
			return B_ERROR;
		case B_MULTI_SET_START_TIME :
			TRACE_ICE(("B_MULTI_SET_START_TIME\n"));
			return B_ERROR;
		case B_MULTI_BUFFER_EXCHANGE :
//			TRACE_ICE(("B_MULTI_BUFFER_EXCHANGE\n"));
			return ice1712_buffer_exchange((ice1712*)cookie, (multi_buffer_info *)arg);
		case B_MULTI_BUFFER_FORCE_STOP :
			TRACE_ICE(("B_MULTI_BUFFER_FORCE_STOP\n"));
			return ice1712_buffer_force_stop((ice1712*)cookie);
		case B_MULTI_LIST_EXTENSIONS :
			TRACE_ICE(("B_MULTI_LIST_EXTENSIONS\n"));
			return B_ERROR;
		case B_MULTI_GET_EXTENSION :
			TRACE_ICE(("B_MULTI_GET_EXTENSION\n"));
			return B_ERROR;
		case B_MULTI_SET_EXTENSION :
			TRACE_ICE(("B_MULTI_SET_EXTENSION\n"));
			return B_ERROR;
		case B_MULTI_LIST_MODES :
			TRACE_ICE(("B_MULTI_LIST_MODES\n"));
			return B_ERROR;
		case B_MULTI_GET_MODE :
			TRACE_ICE(("B_MULTI_GET_MODE\n"));
			return B_ERROR;
		case B_MULTI_SET_MODE :
			TRACE_ICE(("B_MULTI_SET_MODE\n"));
			return B_ERROR;
			
		default :
			TRACE_ICE(("ERROR: unknown multi_control %#x\n", op));
			return B_ERROR;
	}
}


static status_t 
ice_1712_read(void *cookie, off_t position, void *buf, size_t *num_bytes)
{
	TRACE_ICE(("===read()===\n"));
	*num_bytes = 0;
	return B_IO_ERROR;
}


static status_t 
ice_1712_write(void *cookie, off_t position, const void *buffer, size_t *num_bytes)
{
	TRACE_ICE(("===write()===\n"));
	*num_bytes = 0;
	return B_IO_ERROR;
}


device_hooks ice_1712_hooks =
{
	ice_1712_open,
	ice_1712_close,
	ice_1712_free,
	ice_1712_control,
	ice_1712_read,
	ice_1712_write,
	NULL,
	NULL,
	NULL,
	NULL
};


device_hooks *
find_device(const char * name)
{
	int ix;

	PRINT_ICE(("ice1712: find_device(%s)\n", name));

	for (ix=0; ix<num_cards; ix++) {
/*#if MIDI
		if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
#endif
*/		if (!strcmp(cards[ix].name, name)) {
			return &ice_1712_hooks;
		}
	}
	PRINT_ICE(("ice1712: find_device(%s) failed\n", name));
	return NULL;
}
