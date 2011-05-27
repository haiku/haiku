/*
 * ice1712 BeOS/Haiku Driver for VIA - VT1712 Multi Channel Audio Controller
 *
 * Copyright (c) 2007, Jerome Leveque	(leveque.jerome@neuf.fr)
 *
 * All rights reserved
 * Distributed under the terms of the MIT license.
 */

#ifndef _ICE1712_REG_H_
#define _ICE1712_REG_H_

//------------------------------------------------------
//------------------------------------------------------
//PCI Interface and Configuration Registers (Page 3.1)
//Table 3.1
/*
#define PCI_VENDOR_ID					0x00 //2 bytes
#define PCI_DEVICE_ID					0x02 //2 bytes
#define PCI_COMMAND						0x04 //2 bytes
#define PCI_DEVICE_STATUS				0x06 //2 bytes
#define PCI_REVISION_ID					0x08 //1 byte
#define PCI_CLASS_CODE					0x0A //2 bytes
#define PCI_LATENCY_TIMER				0x0D //1 byte
#define PCI_HEADER_TYPE					0x0E //1 byte
#define PCI_BIST						0x0F //1 byte
#define PCI_CONTROLLER_BASE_AD			0x10 //4 bytes
#define PCI_DDMA_BASE_AD				0x14 //4 bytes
#define PCI_DMA_BASE_AD					0x18 //4 bytes
#define PCI_MULTI_BASE_AD				0x1C //4 bytes
#define PCI_SUB_VENDOR_ID				0x2C //2 bytes
#define PCI_SUB_SYSTEM_ID				0x2E //2 bytes
#define PCI_CAPABILITY_POINTER			0x34 //4 bytes
#define PCI_INT_PIN_LINE				0x3C //2 bytes
#define PCI_LATENCY_GRANT				0x3E //2 bytes
#define PCI_LEGACY_AUDIO_CONTROL		0x40 //2 bytes
#define PCI_LEGACY_CONF_CONTROL			0x42 //2 bytes
#define PCI_HARD_CONF_CONTROL			0x60 //4 bytes
#define PCI_CAPABILITY_ID				0x80 //1 byte
#define PCI_NEXT_ITEM_POINTER			0x81 //1 byte
#define PCI_POWER_CAPABILITY			0x82 //2 bytes
#define PCI_POWER_CONTROL_STATUS		0x84 //2 bytes
#define PCI_PMCSR_EXT_DATA				0x86 //2 bytes
*/
//------------------------------------------------------
//------------------------------------------------------
//CCSxx Controller Register Map (Page 4.3)
//Table 4.2
#define	CCS_CONTROL_STATUS				0x00 //1 byte
#define	CCS_INTERRUPT_MASK				0x01 //1 byte
#define	CCS_INTERRUPT_STATUS			0x02 //1 byte
#define	CCS_CCI_INDEX					0x03 //1 byte
#define	CCS_CCI_DATA					0x04 //1 byte
#define	CCS_NMI_STATUS_1				0x05 //1 byte
#define	CCS_NMI_DATA					0x06 //1 byte
#define	CCS_NMI_INDEX					0x07 //1 byte
#define	CCS_CONS_AC97_INDEX				0x08 //1 byte
#define	CCS_CONS_AC97_COMMAND_STATUS	0x09 //1 byte
#define	CCS_CONS_AC97_DATA				0x0A //2 bytes
#define	CCS_MIDI_1_DATA					0x0C //1 byte
#define	CCS_MIDI_1_COMMAND_STATUS		0x0D //1 byte
#define	CCS_NMI_STATUS_2				0x0E //1 byte
#define	CCS_GAME_PORT					0x0F //1 byte
#define	CCS_I2C_DEV_ADDRESS				0x10 //1 byte
#define	CCS_I2C_BYTE_ADDRESS			0x11 //1 byte
#define	CCS_I2C_DATA					0x12 //1 byte
#define	CCS_I2C_CONTROL_STATUS			0x13 //1 byte
#define	CCS_CONS_DMA_BASE_ADDRESS		0x14 //4 bytes
#define	CCS_CONS_DMA_COUNT_ADDRESS		0x18 //2 bytes
#define	CCS_SERR_SHADOW					0x1B //1 byte
#define	CCS_MIDI_2_DATA					0x1C //1 byte
#define	CCS_MIDI_2_COMMAND_STATUS		0x1D //1 byte
#define	CCS_TIMER						0x1E //2 bytes
//------------------------------------------------------
//------------------------------------------------------
//Controller Indexed Register (Page 4.12)
#define	CCI_PB_TERM_COUNT_HI			0x00 //1 byte
#define	CCI_PB_TERM_COUNT_LO			0x01 //1 byte
#define	CCI_PB_CONTROL					0x02 //1 byte
#define	CCI_PB_LEFT_VOLUME				0x03 //1 byte
#define	CCI_PB_RIGHT_VOLUME				0x04 //1 byte
#define	CCI_SOFT_VOLUME					0x05 //1 byte
#define	CCI_PB_SAMPLING_RATE_LO			0x06 //1 byte
#define	CCI_PB_SAMPLING_RATE_MI			0x07 //1 byte
#define	CCI_PB_SAMPLING_RATE_HI			0x08 //1 byte
#define	CCI_REC_TERM_COUNT_HI			0x10 //1 byte
#define	CCI_REC_TERM_COUNT_LO			0x11 //1 byte
#define	CCI_REC_CONTROL					0x12 //1 byte
#define	CCI_GPIO_DATA					0x20 //1 byte
#define	CCI_GPIO_WRITE_MASK				0x21 //1 byte
#define	CCI_GPIO_DIRECTION_CONTROL		0x22 //1 byte
#define	CCI_CONS_POWER_DOWN				0x30 //1 byte
#define	CCI_MULTI_POWER_DOWN			0x31 //1 byte
//------------------------------------------------------
//------------------------------------------------------
//Consumer Section DMA Channel Registers (Page 4.20)
//Table 4.4
#define	DS_DMA_INT_MASK					0x00 //2 bytes
#define	DS_DMA_INT_STATUS				0x02 //2 bytes
#define	DS_CHANNEL_DATA					0x04 //4 bytes
#define	DS_CHANNEL_INDEX				0x08 //1 byte
//------------------------------------------------------
//------------------------------------------------------
//Professional Multi-Track Control Registers (Page 4.24)
//Table 4.7
#define	MT_DMA_INT_MASK_STATUS			0x00 //1 byte
#define	MT_SAMPLING_RATE_SELECT			0x01 //1 byte
#define	MT_I2S_DATA_FORMAT				0x02 //1 byte
#define	MT_PROF_AC97_INDEX				0x04 //1 byte
#define	MT_PROF_AC97_COMMAND_STATUS		0x05 //1 byte
#define	MT_PROF_AC97_DATA				0x06 //2 bytes
#define	MT_PROF_PB_DMA_BASE_ADDRESS		0x10 //4 bytes
#define	MT_PROF_PB_DMA_COUNT_ADDRESS	0x14 //2 bytes
#define	MT_PROF_PB_DMA_TERM_COUNT		0x16 //2 bytes
#define	MT_PROF_PB_CONTROL				0x18 //1 byte
#define	MT_PROF_REC_DMA_BASE_ADDRESS	0x20 //4 bytes
#define	MT_PROF_REC_DMA_COUNT_ADDRESS	0x24 //2 bytes
#define	MT_PROF_REC_DMA_TERM_COUNT		0x26 //2 bytes
#define	MT_PROF_REC_CONTROL				0x28 //1 byte
#define	MT_ROUTING_CONTROL_PSDOUT		0x30 //2 bytes
#define	MT_ROUTING_CONTROL_SPDOUT		0x32 //2 bytes
#define	MT_CAPTURED_DATA				0x34 //4 bytes
#define	MT_LR_VOLUME_CONTROL			0x38 //2 bytes
#define	MT_VOLUME_CONTROL_CHANNEL_INDEX	0x3A //1 byte
#define	MT_VOLUME_CONTROL_RATE			0x3B //1 byte
#define	MT_MIXER_MONITOR_RETURN			0x3C //1 byte
#define	MT_PEAK_METER_INDEX				0x3E //1 byte
#define	MT_PEAK_METER_DATA				0x3F //1 byte
//------------------------------------------------------
//------------------------------------------------------
#define I2C_EEPROM_ADDRESS_READ			0xA0 //1010 0000
#define I2C_EEPROM_ADDRESS_WRITE		0xA1 //1010 0001 
//------------------------------------------------------
//------------------------------------------------------
#define SPDIF_STEREO_IN					0x02 //0000 0010
#define SPDIF_STEREO_OUT				0x01 //0000 0001
//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

//------------------------------------------------------
//------------------------------------------------------

#endif
