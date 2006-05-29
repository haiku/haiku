/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#ifndef _SIS7018HW_H_
#define _SIS7018HW_H_

#ifndef _SOUND_H
#include "sound.h"
#endif

extern pci_module_info* pci;

uint32 hw_read(sis7018_dev *dev, int offset, int size);
void   hw_write(sis7018_dev *dev, int offset, uint32 value, int size);
status_t hw_pchannel_init(pcm_dev *dev);
void hw_pchannel_free(pcm_dev *dev);
size_t hw_pchannel_setblocksize(pcm_dev *dev, size_t blocksize);
void hw_start(pcm_dev *dev);
void hw_stop(pcm_dev *dev);
void hw_increment_interrupt_handler(sis7018_dev * card);
void hw_decrement_interrupt_handler(sis7018_dev * card);
status_t hw_initialize(sis7018_dev *card);
void hw_rdch(sis7018_ch *ch);

void hw_dump_device(int card_id);
void hw_dump_pci_info(int card_id);
void hw_dump_pcm_device(int card_id);
void hw_dump_midi_device(int card_id);
void hw_dump_play_channel(int card_id);

#endif //_SIS7018HW_H_ 
