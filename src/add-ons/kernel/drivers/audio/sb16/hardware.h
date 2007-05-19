/*
 * @(#)hardware.h  1.0 1998/12/22 Carlos Hasan (chasan@dcc.uchile.cl)
 *
 * Hardware definitions for the Sound Blaster 16 device driver.
 *
 * Copyright (C) 1998 Carlos Hasan. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __HARDWARE_H
#define __HARDWARE_H

/*****************************************************************************
 * SOUND BLASTER 16 PLUG-AND-PLAY DEVICE IDENTIFIERS
 *****************************************************************************/

/*****************************************************************************
 * ==========================================================================
 * Product ID	Description
 * ==========================================================================
 * CTL0001	Creative Sound Blaster 16 Plug and Play
 *
 * CTL0021	Creative AWE32 Wavetable MIDI
 * CTL0022	Creative AWE64 Wavetable MIDI
 * CTL0023	Creative AWE64 Gold Wavetable MIDI
 * CTL0024	Creative AWE64 Compatible Wavetable MIDI
 *
 * CTL0031	Creative AWE32 16-bit Audio (SB16 compatible)
 *
 * CTL0041	Creative SB16 Value Plug and Play (V16CL/V32D/V32G)
 * CTL0042	Creative SB AWE32 Plug and Play
 * CTL0043	Creative ViBRA 16X Plug and Play
 * CTL0044	Creative SB AWE64 Gold Plug and Play
 * CTL0045	Creative SB AWE64
 * CTL0046	Creative SB AWE64 Compatible Plug and Play
 * CTL0047	Creative SB16 Plug and Play
 * CTL0048	Creative SB AWE64 Gold Plug and Play
 *
 * CTL0051	Creative 3D Stereo Enhancement
 *
 * CTL00F0	Creative ViBRA 16X Plug and Play
 *
 * CTL7001	Creative Programmable Game Port
 * CTL7002	Creative Programmable Game Port
 * CTL7005	Creative Programmable Game Port (1 I/O)
 *
 * CTL8001	SB Legacy Device
 * PNPB003	SB16 Legacy Device
 *
 * ==========================================================================
 * Serial  Card Name                       Device Name    Vendor ID Device ID
 * ==========================================================================
 * CT3980  Creative SB AWE32 PnP           Audio          CTL0042   CTL0031 
 *                                         WaveTable      CTL0042   CTL0021
 *                                         Game           CTL0042   CTL7001
 *                                         IDE            CTL0042   CTL2011
 *
 * CT2230  Creative ViBRA 16X PnP          Audio          CTL00f0   CTL0043
 *                                         Game           CTL00f0   CTL7005
 */

#define PNP_ISA_PRODUCT_ID(ch0, ch1, ch2, prod, rev) \
(((ch0 & 0x1f) <<  2) | ((ch1 & 0x18) >> 3) | \
 ((ch1 & 0x07) << 13) | ((ch2 & 0x1f) << 8) | \
 ((prod & 0xff0) << 12) | ((prod & 0x00f) << 28) | ((rev & 0xf) << 24))

#define PNP_IS_SB16_DEVICE(id) \
    (((id & 0xf0ffffff) == PNP_ISA_PRODUCT_ID('C', 'T', 'L', 0x000, 0x0)) || \
     ((id & 0xf0ffffff) == PNP_ISA_PRODUCT_ID('C', 'T', 'L', 0x003, 0x0)) || \
     ((id & 0xf0ffffff) == PNP_ISA_PRODUCT_ID('C', 'T', 'L', 0x004, 0x0)))

/*****************************************************************************
 * SOUND BLASTER 16 HARDWARE SPECIFICATIONS
 *****************************************************************************/

/*****************************************************************************
 * Sound Blaster 16 DSP I/O port register offsets
 */
    enum {
	SB16_MIXER_ADDRESS      = 0x004,
	SB16_MIXER_DATA         = 0x005,
	SB16_CODEC_RESET        = 0x006,
	SB16_CODEC_READ_DATA    = 0x00a,
	SB16_CODEC_WRITE_DATA   = 0x00c,
	SB16_CODEC_WRITE_STATUS = 0x00c,
	SB16_CODEC_READ_STATUS  = 0x00e,
	SB16_CODEC_ACK_8_BIT    = 0x00e,
	SB16_CODEC_ACK_16_BIT   = 0x00f
    };

/*****************************************************************************
 * Sound Blaster 16 DSP programming commands
 */
enum {
    SB16_SPEAKER_ENABLE     = 0xd1,
    SB16_SPEAKER_DISABLE    = 0xd3,
    SB16_SPEAKER_STATUS     = 0xd8,

    SB16_PLAYBACK_RATE      = 0x41,
    SB16_CAPTURE_RATE       = 0x42,

    SB16_PLAYBACK_8_BIT     = 0xc6,
    SB16_CAPTURE_8_BIT      = 0xce,
    SB16_DMA_DISABLE_8_BIT  = 0xd0,
    SB16_DMA_ENABLE_8_BIT   = 0xd4,
    SB16_DMA_EXIT_8_BIT     = 0xda,

    SB16_PLAYBACK_16_BIT    = 0xb6,
    SB16_CAPTURE_16_BIT     = 0xbe,
    SB16_DMA_DISABLE_16_BIT = 0xd5,
    SB16_DMA_ENABLE_16_BIT  = 0xd6,
    SB16_DMA_EXIT_16_BIT    = 0xd9,

    SB16_CODEC_VERSION      = 0xe1
};

/*****************************************************************************
 * Sound Blaster 16 mixer indirect registers
 */
enum {
    SB16_MIXER_RESET        = 0x00,

    SB16_LINE_OUT_LEFT      = 0x30,
    SB16_LINE_OUT_RIGHT     = 0x31,

    SB16_DAC_LEFT           = 0x32,
    SB16_DAC_RIGHT          = 0x33,

    SB16_SYNTH_LEFT         = 0x34,
    SB16_SYNTH_RIGHT        = 0x35,

    SB16_CD_LEFT            = 0x36,
    SB16_CD_RIGHT           = 0x37,

    SB16_LINE_IN_LEFT       = 0x38,
    SB16_LINE_IN_RIGHT      = 0x39,

    SB16_MIC                = 0x3a,
    SB16_PC_BEEP            = 0x3b,

    SB16_OUTPUT_MUX         = 0x3c,

    SB16_INPUT_MUX_LEFT     = 0x3d,
    SB16_INPUT_MUX_RIGHT    = 0x3e,

    SB16_ADC_GAIN_LEFT      = 0x3f,
    SB16_ADC_GAIN_RIGHT     = 0x40,

    SB16_DAC_GAIN_LEFT      = 0x41,
    SB16_DAC_GAIN_RIGHT     = 0x42,

    SB16_MIC_BOOST          = 0x43,

    SB16_TREBLE_LEFT        = 0x44,
    SB16_TREBLE_RIGHT       = 0x45,

    SB16_BASS_LEFT          = 0x46,
    SB16_BASS_RIGHT         = 0x47,

    SB16_IRQ_SETUP          = 0x80,
    SB16_DMA_SETUP          = 0x81,
    SB16_IRQ_STATUS         = 0x82,

    SB16_STEREO_ENHANCEMENT = 0x90
};

/*****************************************************************************
 * Sound Blaster 16 playback and capture sample formats
 */
enum {
    B_SB16_FORMAT_UNSIGNED  = 0x00,
    B_SB16_FORMAT_SIGNED    = 0x10,
    B_SB16_FORMAT_MONO      = 0x00,
    B_SB16_FORMAT_STEREO    = 0x20
};

/*****************************************************************************
 * Sound Blaster 16 playback and capture control bits
 */
enum {
    B_SB16_MUX_SYNTH_LEFT   = 0x20,
    B_SB16_MUX_SYNTH_RIGHT  = 0x40,
    B_SB16_MUX_LINE_LEFT    = 0x10,
    B_SB16_MUX_LINE_RIGHT   = 0x08,
    B_SB16_MUX_CD_LEFT      = 0x04,
    B_SB16_MUX_CD_RIGHT     = 0x02,
    B_SB16_MUX_MIC          = 0x01,
    B_SB16_MUX_ALL_LEFT     = 0x35,
    B_SB16_MUX_ALL_RIGHT    = 0x4b
};

/*****************************************************************************
 * Sound Blaster 16 Stereo Enhancenment
 */
enum {
    B_SB16_SE_ENABLE        = 0x01,
    B_SB16_SE_DETECT_ENABLE = 0x40,
    B_SB16_SE_DETECT_STATUS = 0x80
};

/*****************************************************************************
 * Sound Blaster 16 DSP I/O port delays
 */
enum {
    SB16_CODEC_IO_DELAY     = 200000,
    SB16_CODEC_RESET_DELAY  = 200
};

/*****************************************************************************
 * Sound Blaster 16 DMA low memory buffer
 */
typedef struct {
    area_id area;
    char *address;
    int size;
} hw_dma_buffer;

/*****************************************************************************
 * SOUND BLASTER 16 MPU-401 MIDI PORT HARDWARE SPECIFICATIONS
 *****************************************************************************/

/*****************************************************************************
 * Sound Blaster 16 MPU-401 I/O port register offsets
 */
enum {
    MPU401_DATA           = 0x000,
    MPU401_STATUS         = 0x001,
    MPU401_COMMAND        = 0x001
};

/*****************************************************************************
 * Sound Blaster 16 MPU-401 status register bit fields
 */
enum {
    B_MPU401_WRITE_BUSY   = 0x40,
    B_MPU401_READ_BUSY    = 0x80
};
  
/*****************************************************************************
 * Sound Blaster 16 MPU-401 MIDI port commands
 */
enum {
    MPU401_CMD_RESET      = 0xff,
    MPU401_CMD_UART_MODE  = 0x3f
};

/*****************************************************************************
 * Sound Blaster 16 MPU-401 port timeout and latency
 */
enum {
    MPU401_IO_LATENCY     = 1000,
    MPU401_IO_TIMEOUT     = 10000
};

#endif
