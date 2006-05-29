/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#ifndef _SIS7018_H_
 #define _SIS7018_H_

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <PCI.h>

#define CHIPNAME "sis7018"

void sis7018_trace(bool b_force, char *fmt, ...);

//#define TRACE_ALWAYS(x...) dprintf("\33[32m"CHIPNAME":\33[0m"x);
#define TRACE_ALWAYS(x...) sis7018_trace(true, x);
#define TRACE(x...) sis7018_trace(false, x);

extern bool b_trace_ac97;

#if DEBUG
//#define TRACE(x...) TRACE_ALWAYS(x)
int _assert_(char *,int,char *); 
#define ASSERT(c) (!(c) ? _assert_(__FILE__, __LINE__, #c) : 0) 
#else
//#define TRACE(x...)
#define ASSERT(c) 
#endif

#define DEVNAME 32

#define TDX_PCI_ID   0x20001023 // Trident 4D Wave/DX 
#define xDX_PCI_ID   0x20000003 // Hypotese? 
#define TNX_PCI_ID   0x20011023 // Trident 4D Wave/NX
#define ALI_PCI_ID  0x545110b9 // Acer Labs 5451 Audio
#define SPA_PCI_ID  0x70181039 // SiS PCI Audio

typedef struct _card_type 
{
  union _ids
  {
    uint32 chip_id;
    struct {
      uint16 vendor_id;
      uint16 card_id;
    } ids; 
  } ids;
  char chip_name[DEVNAME];
} card_type;

#define DO_PCM  1
#define DO_MIDI 0

//#define USE_IO_REGS 0
extern bool use_io_regs;

typedef struct _midi_dev {
  struct _sis7018_dev *card;
  void *    driver;
  void *    cookie;
  int32    count;
  char    name[64];
} midi_dev;

/* channel registers */
typedef struct _sis7018_ch {
  int32 cso, alpha, fms, fmc, ec;
  int32 lba;
  int32 eso, delta;
  int32 rvol, cvol;
  int32 gvsel, pan, vol, ctrl;
  int32 active/*:1, was_active:1*/;
  int index/*, bufhalf*/;
  struct _sis7018_dev *card;
  size_t    mem_size;    /* size of low memory */
  vuchar *  mem_ptr;
  vuchar *  mem_phys;    /* physical address */
  area_id    areaid;    /* area pointing to low memory */

  vuchar *  wr_1;
  vuchar *  wr_2;
  vuchar *  wr_cur;
#if DEBUG
  vuchar *  wr_cur_prv;
  bigtime_t time_prv;
  bool prevInt;
#endif
  size_t    wr_size;

  int32    wr_lock;
  int      wr_silence;
  int32    write_waiting;
  sem_id    write_sem;
  size_t    was_written;
  uint32    wr_skipped;
  sem_id    wr_entry;
  bigtime_t  wr_time;
  uint64    wr_total;
//  sem_id    wr_time_sem;
//  int32    wr_time_wait;  
} sis7018_ch;

typedef struct {
  struct _sis7018_dev * card;
  char    name[DEVNAME];
  int32  open_count;
  int32  open_mode;
  sem_id init_sem;

  sem_id  old_cap_sem;
  sem_id  old_play_sem;

  char    oldname[DEVNAME];
  struct _sis7018_ch play_channel;
} pcm_dev;

typedef struct _ac97_codec
{
  bool wide_main_gain;
} ac97_codec;

typedef struct _sis7018_dev
{
  char    name[DEVNAME];  /* used for resources */
  int32    hardware;    /* spinlock */
//#if(USE_IO_REGS)
  int      io_base;       /* offset to port */
//#else
  void *regs_mem_base;
//#endif
  int32    inth_count;
/*  int      dma_base;
  size_t    low_size;*/    /* size of low memory */
/*  vuchar *  low_mem;
  vuchar *  low_phys;*/    /* physical address */
//  area_id    map_low;    /* area pointing to low memory */
//#if(!USE_IO_REGS)
  area_id regs_area;
//#endif
  pci_info  info;
  midi_dev  midi;
/*  joy_dev    joy;*/
  pcm_dev    pcm;
/*  mux_dev    mux;
  mixer_dev  mixer;*/
  card_type *type;
  ac97_codec ac97;
} sis7018_dev;

extern int32 num_cards;
extern sis7018_dev cards[];

/*uint32 io_rd(sis7018_dev *dev, int offset, int size);
void io_wr(sis7018_dev *dev, int offset, uint32 value, int size);
int rd_cd(sis7018_dev *dev, int regno);
int wr_cd(sis7018_dev *dev, int regno, uint32 data);

void increment_interrupt_handler(sis7018_dev * card);
void decrement_interrupt_handler(sis7018_dev * card);
*/
#endif //_SIS7018_H_
