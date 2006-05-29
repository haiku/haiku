/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#include <stdio.h> //sprintf
#include <string.h>

#include "sis7018.h"
#include "sis7018hw.h"
#include "t4dwave.h"
#include "ac97.h"
#include "pcm.h"

pci_module_info* pci;

#define MIN_MEMORY_SIZE 0x2000
#define PCHANNEL_ID 0x00

int intrs;

uint32 hw_read(sis7018_dev *dev, int offset, int size)
{
  uint32 ret = 0xffffffff;
  if(use_io_regs){
    switch(size){
      case 1: ret = (*pci->read_io_8)(dev->io_base + offset); break;
      case 2: ret = (*pci->read_io_16)(dev->io_base + offset); break;
      case 4: ret = (*pci->read_io_32)(dev->io_base + offset); break;
      default: break;
    }
  }
  else{
    switch(size){
      case 1: ret = *(int8 *)((char *)dev->regs_mem_base + offset); break;
      case 2: ret = *(int16 *)((char *)dev->regs_mem_base + offset); break;
      case 4: ret = *(int32 *)((char *)dev->regs_mem_base + offset); break;
      default: break;
    }
  }
 // TRACE("tr_rd off=%x ret=%lx\n", offset, ret);
  return ret;
}

void hw_write(sis7018_dev *dev, int offset, uint32 value, int size)
{
  //TRACE("tr_wr off=%x value = %lx\n", offset, value);
  if(use_io_regs){
    switch(size){
      case 1: (*pci->write_io_8)(dev->io_base + offset, (uint8) value); break;
      case 2: (*pci->write_io_16)(dev->io_base + offset, (uint16) value); break;
      case 4: (*pci->write_io_32)(dev->io_base + offset, value); break;
      default: break;
    }
  }
  else{
    switch(size){
      case 1: *(int8 *)((char *)dev->regs_mem_base + offset) = (int8)value; break;
      case 2: *(int16 *)((char *)dev->regs_mem_base + offset) = (int16)value; break;
      case 4: *(int32 *)((char *)dev->regs_mem_base + offset) = (int32)value; break;
      default: break;
    }
  }
}

static void hw_clrint(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int bank, chan;

  bank = (ch->index & 0x20) ? 1 : 0;
  chan = ch->index & 0x1f;
  hw_write(dev, bank? TR_REG_ADDRINTB : TR_REG_ADDRINTA, 1 << chan, 4);
}

static void hw_enaint(sis7018_ch *ch, int enable)
{
  sis7018_dev *dev = ch->card;
  int32 i, reg;
  int bank, chan;
  cpu_status cp;
 
  cp = disable_interrupts();
  acquire_spinlock(&dev->hardware);

  bank = (ch->index & 0x20) ? 1 : 0;
  chan = ch->index & 0x1f;
  reg = bank? TR_REG_INTENB : TR_REG_INTENA;

  i = hw_read(dev, reg, 4);
  i &= ~(1 << chan);
  i |= (enable? 1 : 0) << chan;

  hw_clrint(ch);
  hw_write(dev, reg, i, 4);

  release_spinlock(&dev->hardware);
  restore_interrupts(cp);
}

static void hw_selch(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int32 i;

  i = hw_read(dev, TR_REG_CIR, 4);
  i &= ~TR_CIR_MASK;
  i |= ch->index & 0x3f;
  hw_write(dev, TR_REG_CIR, i, 4);
}

static void hw_startch(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int bank, chan;

  bank = (ch->index & 0x20) ? 1 : 0;
  chan = ch->index & 0x1f;
  hw_write(dev, bank? TR_REG_STARTB : TR_REG_STARTA, 1 << chan, 4);
}

static void hw_stopch(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int bank, chan;

  bank = (ch->index & 0x20) ? 1 : 0;
  chan = ch->index & 0x1f;
  hw_write(dev, bank? TR_REG_STOPB : TR_REG_STOPA, 1 << chan, 4);
}

static void hw_wrch(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int32 cr[TR_CHN_REGS], i;
  cpu_status cp;

  ch->gvsel &= 0x00000001;
  ch->fmc   &= 0x00000003;
  ch->fms   &= 0x0000000f;
  ch->ctrl  &= 0x0000000f;
  ch->pan   &= 0x0000007f;
  ch->rvol  &= 0x0000007f;
  ch->cvol  &= 0x0000007f;
  ch->vol   &= 0x000000ff;
  ch->ec    &= 0x00000fff;
  ch->alpha &= 0x00000fff;
  ch->delta &= 0x0000ffff;
  ch->lba   &= 0x3fffffff;

  cr[1]=ch->lba;
  cr[3]=(ch->fmc<<14) | (ch->rvol<<7) | (ch->cvol);
  cr[4]=(ch->gvsel<<31) | (ch->pan<<24) | (ch->vol<<16) | (ch->ctrl<<12) | (ch->ec);

  switch (ch->card->type->ids.chip_id)
  {
  case SPA_PCI_ID:
  case ALI_PCI_ID:
  case TDX_PCI_ID:
  case xDX_PCI_ID:
    ch->cso &= 0x0000ffff;
    ch->eso &= 0x0000ffff;
    cr[0]=(ch->cso<<16) | (ch->alpha<<4) | (ch->fms);
    cr[2]=(ch->eso<<16) | (ch->delta);
    break;
  case TNX_PCI_ID:
    ch->cso &= 0x00ffffff;
    ch->eso &= 0x00ffffff;
    cr[0]=((ch->delta & 0xff)<<24) | (ch->cso);
    cr[2]=((ch->delta>>8)<<24) | (ch->eso);
    cr[3]|=(ch->alpha<<20) | (ch->fms<<16) | (ch->fmc<<14);
    break;
  } 
  cp = disable_interrupts();
  acquire_spinlock(&dev->hardware);
  
  hw_selch(ch);
  for (i=0; i<TR_CHN_REGS; i++)
    hw_write(dev, TR_REG_CHNBASE+(i<<2), cr[i], 4);

  release_spinlock(&dev->hardware);
  restore_interrupts(cp);
}

void hw_rdch(sis7018_ch *ch)
{
  sis7018_dev *dev = ch->card;
  int32 cr[5], i;
  cpu_status cp;
 
  cp = disable_interrupts();
  acquire_spinlock(&dev->hardware);

  hw_selch(ch);
  for (i=0; i<5; i++)
    cr[i]=hw_read(dev, TR_REG_CHNBASE+(i<<2), 4);

  release_spinlock(&dev->hardware);
  restore_interrupts(cp);

  ch->lba=   (cr[1] & 0x3fffffff);
  ch->fmc=   (cr[3] & 0x0000c000) >> 14;
  ch->rvol=  (cr[3] & 0x00003f80) >> 7;
  ch->cvol=  (cr[3] & 0x0000007f);
  ch->gvsel= (cr[4] & 0x80000000) >> 31;
  ch->pan=   (cr[4] & 0x7f000000) >> 24;
  ch->vol=   (cr[4] & 0x00ff0000) >> 16;
  ch->ctrl=  (cr[4] & 0x0000f000) >> 12;
  ch->ec=    (cr[4] & 0x00000fff);
  
  switch(ch->card->type->ids.chip_id)
  {
  case SPA_PCI_ID:
  case ALI_PCI_ID:
  case TDX_PCI_ID: 
  case xDX_PCI_ID: 
    ch->cso=   (cr[0] & 0xffff0000) >> 16;
    ch->alpha= (cr[0] & 0x0000fff0) >> 4;
    ch->fms=   (cr[0] & 0x0000000f);
    ch->eso=   (cr[2] & 0xffff0000) >> 16;
    ch->delta= (cr[2] & 0x0000ffff);
    break;
  case TNX_PCI_ID:
    ch->cso=   (cr[0] & 0x00ffffff);
    ch->eso=   (cr[2] & 0x00ffffff);
    ch->delta= ((cr[2] & 0xff000000) >> 16) | ((cr[0] & 0xff000000) >> 24);
    ch->alpha= (cr[3] & 0xfff00000) >> 20;
    ch->fms=   (cr[3] & 0x000f0000) >> 16;
    break;
  }
}

status_t hw_pchannel_init(pcm_dev *dev)
{
  char name_buf[256];
  sis7018_ch *ch = &dev->play_channel;
  ch->index = PCHANNEL_ID;
  hw_pchannel_setblocksize(dev, 8192);
  ch->ctrl = 0x0f; // 16-bit stereo signed + loop mode enabled 
  ch->delta = (48000 << 12 ) / 48000;
  ch->card = dev->card;
  ch->areaid=-1;
  
  ch->wr_lock = 0;
  ch->write_waiting = 0;
  sprintf(name_buf, "WS:%s", dev->name);
  name_buf[B_OS_NAME_LENGTH-1] = 0;
  ch->write_sem = create_sem(0, name_buf);
  if (ch->write_sem < B_OK)
    return ch->write_sem;

  set_sem_owner(ch->write_sem, B_SYSTEM_TEAM);
  name_buf[0] = 'W'; name_buf[1] = 'E';
  ch->wr_entry = create_sem(1, name_buf);
  if (ch->wr_entry < B_OK){
    delete_sem(ch->write_sem);
    return ch->wr_entry;
  }
  set_sem_owner(ch->wr_entry, B_SYSTEM_TEAM);
  ch->wr_time = 0;
  return B_OK;
}

void hw_pchannel_free(pcm_dev *dev)
{
  sis7018_ch *ch = &dev->play_channel;
  if(ch->areaid >= 0){
    delete_area(ch->areaid);
    ch->areaid = -1;
    
    delete_sem(ch->write_sem);
    delete_sem(ch->wr_entry);
    ch->write_sem = -1;
    ch->wr_entry = -1;
  }
}

void hw_start(pcm_dev *dev)
{
  sis7018_ch *ch = &dev->play_channel;

  if(ch->active)
    return;
    
    /* start out with a clean slate */
  TRACE("hw_start()\n");
/*  if (port->config.format == 0x11)
  {
    memset((void*)port->card->low_mem, 0x80, port->config.play_buf_size + 
      port->config.rec_buf_size);
  }
  else {*/
  
  // TODO?
  //  memset((void *)port->card->low_mem, 0, port->config.play_buf_size + 
  //    port->config.rec_buf_size);
  //}

  ch->wr_cur = ch->wr_2;
  ch->wr_silence = ch->mem_size;
  ch->was_written = 0;
  ch->wr_total = 0;
  
  ch->fmc = 3;
  ch->fms = 0;
  ch->ec = 0;
  ch->alpha = 0;
  ch->lba = (int32)ch->mem_phys;
  ch->cso = 0;
  ch->eso = ch->mem_size/(sizeof(int16)*2)-1;
  ch->rvol = ch->cvol = 0x7f;
  ch->gvsel = 0;
  ch->pan = 0;
  ch->vol = 0;

  hw_wrch(ch);
  hw_enaint(ch, 1);
  hw_startch(ch);

  ch->active = 1;
}

void hw_stop(pcm_dev *dev)
{
  sis7018_ch *ch = &dev->play_channel;
  if(!ch->active)
    return;
    
  hw_stopch(ch);
  ch->active = 0;
}

static int32 hw_interrupt_handler(void * data)
{
  int32 active, mask, bufhalf, chnum, intsrc;
  sis7018_dev *card = (sis7018_dev *)data;
  sis7018_ch *ch;
  int32 handled = B_UNHANDLED_INTERRUPT;
  intrs++;
  
  acquire_spinlock(&card->hardware);
  intsrc = hw_read(card, TR_REG_MISCINT, 4);

#if DO_PCM
  if (intsrc & TR_INT_ADDR){
    chnum = PCHANNEL_ID;
    mask = 0x00000001 << PCHANNEL_ID;
    active = hw_read(card, (chnum < 32)? TR_REG_ADDRINTA : TR_REG_ADDRINTB, 4);
    bufhalf = hw_read(card, (chnum < 32)? TR_REG_CSPF_A : TR_REG_CSPF_B, 4);
    if (active & mask){
      ch = &card->pcm.play_channel;
      if(playback_interrupt(ch, (bufhalf & mask)))
        handled = B_INVOKE_SCHEDULER;
      else
        handled = B_HANDLED_INTERRUPT;
    }
    hw_write(card, (chnum <= 32)? TR_REG_ADDRINTA : TR_REG_ADDRINTB, active, 4);
  }
#endif
#if DO_MIDI
  ssssssss
  /*if (status & 0x80) {
    if (midi_interrupt(card)) {
      handled = B_INVOKE_SCHEDULER;
    } else {
      handled = B_HANDLED_INTERRUPT;
    }
  }*/
  handled = B_HANDLED_INTERRUPT;
#endif

  release_spinlock(&card->hardware);
  return handled;
}

void hw_increment_interrupt_handler(sis7018_dev * card)
{
  TRACE("hw_increment_interrupt_handler()\n");
  if (atomic_add(&card->inth_count, 1) == 0){
    TRACE("intline %d int %p\n", card->info.u.h0.interrupt_line, hw_interrupt_handler);
    install_io_interrupt_handler(card->info.u.h0.interrupt_line, hw_interrupt_handler, card, 0);
  }
}

void hw_decrement_interrupt_handler(sis7018_dev * card)
{
  TRACE("hw_decrement_interrupt_handler()\n");
  if (atomic_add(&card->inth_count, -1) == 1){
    TRACE("remove_io_interrupt_handler()\n");
    remove_io_interrupt_handler(card->info.u.h0.interrupt_line, hw_interrupt_handler, card);
  }
}

status_t hw_initialize(sis7018_dev *card)
{
  status_t err = B_OK;
  
  cpu_status cp = disable_interrupts();
  acquire_spinlock(&card->hardware);
  
  switch (card->type->ids.chip_id){
  case SPA_PCI_ID:
    hw_write(card, SPA_REG_GPIO, 0, 4);
    hw_write(card, SPA_REG_CODECST, SPA_RST_OFF, 4);
    break;
  case TDX_PCI_ID:
  case xDX_PCI_ID:
    hw_write(card, TDX_REG_CODECST, TDX_CDC_ON, 4);
    break;
  case TNX_PCI_ID:
    hw_write(card, TNX_REG_CODECST, TNX_CDC_ON, 4);
    break;
  }
  
  hw_write(card, TR_REG_CIR, TR_CIR_MIDENA | TR_CIR_ADDRENA, 4);

  release_spinlock(&card->hardware);
  restore_interrupts(cp);

  err |= ac97init(card);

  return err;
}

void hw_dump_device(int card_id)
{
  kprintf("interrupt handler called: %d\n", intrs);
}

void hw_dump_pci_info(int card_id)
{
  kprintf("dump_pci_info is not yet implemented.\n");
}

void hw_dump_pcm_device(int card_id)
{
  kprintf("dump_pcm_device is not yet implemented.\n");
}

void hw_dump_midi_device(int card_id)
{
  kprintf("dump_midi_device is not yet implemented.\n");
}

static void hw_dump_channel(sis7018_ch *ch)
{
  kprintf("Pointer : %p\n", ch);
  kprintf("  cso:%8lx, alpha:%8lx,     fms:%8lx,  fmc:%8lx,     ec:%lx\n",
            ch->cso, ch->alpha, ch->fms, ch->fmc, ch->ec);
  kprintf("  lba:%8lx,   eso:%8lx,   delta:%8lx, rvol:%8lx,   cvol:%lx\n",
            ch->lba, ch->eso, ch->delta, ch->rvol, ch->cvol);
  kprintf("gvsel:%8lx,   pan:%8lx,     vol:%8lx, ctrl:%8lx, active:%x\n",
            ch->gvsel, ch->pan, ch->vol, ch->ctrl, (int)ch->active);
  kprintf("index:%8x,  card:0x%8p, mem_ptr:0x%8p, mem_size:%8lx, mem_phys:0x%8p\n",
            ch->index, ch->card, ch->mem_ptr, ch->mem_size, ch->mem_phys);
  kprintf("\n");
}

void hw_dump_play_channel(int card_id)
{
  sis7018_ch ch;
  memcpy(&ch, &cards[card_id].pcm.play_channel, sizeof(sis7018_ch));
  kprintf("Dumping play channel information:\n");
  kprintf(" - cached:\n");
  hw_dump_channel(&ch);
  kprintf(" - readed from hardware:\n");
  hw_rdch(&ch);
  hw_dump_channel(&ch);
}

static status_t hw_find_memory(sis7018_ch *ch, size_t blocksize)
{
  size_t low_size = (blocksize * 2+(B_PAGE_SIZE-1))&~(B_PAGE_SIZE-1);
  physical_entry where;
  size_t trysize;
  area_id curarea;
  void * addr;
  char name[DEVNAME];

  sprintf(name, "%s_area", ch->card->name);
  if (low_size < MIN_MEMORY_SIZE)
    low_size = MIN_MEMORY_SIZE;
  trysize = low_size;

  curarea = find_area(name);
  if (curarea != B_NAME_NOT_FOUND){
    TRACE("delete previously allocated area\n");
    delete_area(curarea);
    curarea = -1;
  }
  
  TRACE("allocating new area\n");

  curarea = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, 
                          trysize, B_LOMEM, B_READ_AREA | B_WRITE_AREA);
  TRACE("create_area(%lx) returned %lx logical %p\n", trysize, curarea, addr);
  
  if (curarea < 0)
    goto oops;

  if (get_memory_map(addr, low_size, &where, 1) < 0){
    delete_area(curarea);
    curarea = B_ERROR;
    goto oops;
  }
  TRACE("physical %p\n", where.address);
  if ((uint32)where.address & 0xff000000){
    delete_area(curarea);
    curarea = B_ERROR;
    goto oops;
  }
  if ((((uint32)where.address)+low_size) & 0xff000000){
    delete_area(curarea);
    curarea = B_ERROR;
    goto oops;
  }
  /* hey, it worked! */
  if (trysize > low_size) /* don't waste */
    resize_area(curarea, low_size);

oops:
  if (curarea < 0){
    TRACE("failed to create area\n");
    return curarea;
  }
//a_o_k:
  TRACE("successfully created area!\n");

  ch->mem_size = low_size;
  ch->mem_ptr = addr;
  ch->mem_phys = (vuchar *)where.address;
  
  ch->wr_1 = ch->mem_ptr;
  ch->wr_2 = ch->wr_1+ch->mem_size/2;
  ch->wr_size = ch->mem_size/2;
  
  ch->areaid = curarea;
  
  if(ch->active){
    hw_stop(&ch->card->pcm);
    hw_start(&ch->card->pcm);   
  }
  
  return B_OK;
}

size_t hw_pchannel_setblocksize(pcm_dev *dev, size_t blocksize)
{
  sis7018_ch *ch = &dev->play_channel;

  if(ch->mem_size/2 != blocksize){
    if(ch->areaid >= 0){
      TRACE("delete area on resize\n");
      delete_area(ch->areaid);
      ch->areaid = -1;
    }
    //TODO : error cheking.
    hw_find_memory(ch, blocksize);
  }
  return blocksize;
}
