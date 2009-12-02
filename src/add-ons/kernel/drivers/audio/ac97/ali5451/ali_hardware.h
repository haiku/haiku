/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ALI_HARDWARE_H
#define _ALI_HARDWARE_H


// AC97 codec registers

#define AC97_REG_RESET 0x00
#define AC97_MIX_MASTER 0x02
#define AC97_MIX_HEADPHONE 0x04
#define AC97_MIX_MONO 0x06
#define AC97_MIX_PHONE 0x0c
#define AC97_MIX_MIC 0x0e
#define AC97_MIX_LINEIN 0x10
#define AC97_MIX_CD 0x12
#define AC97_MIX_AUX 0x16
#define AC97_MIX_PCM 0x18
#define AC97_MIX_RECORDSELECT 0x1a
#define AC97_MIX_RECORDGAIN 0x1c
#define AC97_REG_GEN 0x20
#define AC97_REG_POWER 0x26
#define AC97_FRONT_DAC_SR 0x2c
#define AC97_ADC_RATE 0x32
#define AC97_JACK_SENSE 0x72
#define AC97_REG_ID1 0x7c
#define AC97_REG_ID2 0x7e


uint16 ac97_read(ali_dev *card, uint8 reg);
void ac97_write(ali_dev *card, uint8 reg, uint16 val);

status_t hardware_init(ali_dev *card);
void hardware_terminate(ali_dev *card);

uint32 ali_read_int(ali_dev *card);

bool ali_voice_start(ali_stream *stream);
void ali_voice_stop(ali_stream *stream);
void ali_voice_clr_int(ali_stream *stream);

#endif // _ALI_HARDWARE_H
