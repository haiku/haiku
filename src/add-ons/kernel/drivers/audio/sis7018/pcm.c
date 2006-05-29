/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#include "pcm.h"
#include "sis7018.h"
#include "sis7018hw.h" // ioctl definitions
#include "sound.h"
#include "ac97.h"
#include "R3MediaDefs.h" // sound_buffer definition
#include "t4dwave.h"

#if DEBUG
int32 int_cnt;
int32 put_cnt;
bigtime_t the_time;
#endif

/* pcm_open - handle open() calls */
static status_t pcm_open (const char *name, uint32 flags, void** cookie)
{
  int ix;
  pcm_dev *port = NULL;

  TRACE("pcm_open(%s, %lx)\n", name, flags);

  *cookie = NULL;
  for (ix=0; ix<num_cards; ix++)
    if (!strcmp(name, cards[ix].pcm.name))
      goto gotit;

  for (ix=0; ix<num_cards; ix++)
    if (!strcmp(name, cards[ix].pcm.oldname))
      goto gotit;

  TRACE("%s not found\n", name);
  return ENODEV;
  
gotit:
  *cookie = port = &cards[ix].pcm;
  acquire_sem(port->init_sem);
  
  if (atomic_add(&port->open_count, 1) == 0)
  {
    TRACE("pcm_open: first open\n");
    port->card = &cards[ix];
    
    port->open_mode = (flags & O_RWMASK);
    if (port->open_mode != O_RDONLY)
      ;// TODO start playback "dma"

   if (port->open_mode != O_WRONLY)
      ;//TODO start record "dma"
    
    port->old_cap_sem = -1;
    port->old_play_sem = -1;

    hw_pchannel_init(port);        
    hw_increment_interrupt_handler(port->card);
  }
  else
  {
    if ((flags & O_RWMASK) == O_RDWR)
    {
      if (port->open_mode == O_RDONLY)
        ; //TODO start playback "dma"
      else
        if (port->open_mode == O_WRONLY)
          ; //TODO start record "dma"
      port->open_mode = O_RDWR;
    }
    TRACE("pcm_open: next opens\n");
  }
  
  hw_start(port);
  
  release_sem(port->init_sem);

  return B_OK;
}

/* pcm_read - handle read() calls */
static status_t pcm_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
  pcm_dev * port = (pcm_dev *)cookie;
  
  if (port->open_mode == O_WRONLY)
    return EPERM;

//  TRACE("pcm_read(%p, %d, %p, %d)\n", cookie, position, buf, *num_bytes);
  *num_bytes = 0;        /* tell caller nothing was read */
  return B_IO_ERROR;
}

extern int intrs;

static status_t pcm_write(void * cookie, off_t pos, const void * data, size_t * nwritten)
{
  pcm_dev * port = (pcm_dev *)cookie;
  sis7018_ch *ch = &((pcm_dev *)cookie)->play_channel;
  status_t err;
  cpu_status cp;
  int written = 0;
  int to_write = *nwritten;  /*   in play bytes, not input bytes!  */
  int block;
  int bytes_xferred;

  if (port->open_mode == O_RDONLY)
    return EPERM;

  *nwritten = 0;

  if ((err = acquire_sem_etc(ch->wr_entry, 1, B_CAN_INTERRUPT, 0)) < B_OK)
  {
    TRACE("wr_entry acquisition failed\n");
    return err;
  }
  atomic_add(&ch->write_waiting, 1);

  while (to_write > 0)
  {
    /* wait to write */
    if ((err = acquire_sem_etc(ch->write_sem, 1, B_CAN_INTERRUPT, 0)) < B_OK)
    {
      TRACE("write_sem acquisition failed\n");
      release_sem(ch->wr_entry);
      return err;
    }

#if DEBUG
    put_cnt++;
    {
      bigtime_t delta = system_time() - the_time;
      if (delta < 1) {
        TRACE("delta %Ld (low!) #%ld\n", delta, put_cnt);
      }
      else if (delta > 2000) {
        TRACE("delta %Ld (high!) #%ld\n", delta, put_cnt);
      }
    }
    if (put_cnt != int_cnt) {
  static int last;
      if (last != int_cnt-put_cnt)
        TRACE("%ld mismatch\n", int_cnt-put_cnt);
      last = int_cnt-put_cnt;
    } 
#endif /* DEBUG */

  //  KTRACE();
    cp = disable_interrupts();
    acquire_spinlock(&ch->wr_lock);

    block = ch->wr_size-ch->was_written;
    if (block > to_write)
    {
      /* must let next guy in */
      if (atomic_add(&ch->write_waiting, -1) > 0)
        release_sem_etc(ch->write_sem, 1, B_DO_NOT_RESCHEDULE);
      else
        atomic_add(&ch->write_waiting, 1); /* undo damage */

      block = to_write;
    }
    else
      if (block < to_write)
        atomic_add(&ch->write_waiting, 1); /* we will loop back */

  /*  switch (port->config.format)
    {*/
  //  case 0x24:  /*  floats  */
    /*  copy_float_to_short((short *)(port->wr_cur+port->was_written), (const float *)data, 
        block, !B_HOST_IS_LENDIAN == !port->config.big_endian);
      bytes_xferred = block * 2;
      break; */
  //  case 0x02:  /*  shorts  */
  //    if (!B_HOST_IS_LENDIAN == !port->config.big_endian) {
        /*  we need to swap  */
  /*      swap_copy((short *)(port->wr_cur+port->was_written), (const short *)data, block);
        bytes_xferred = block;
        break;
      }
  */    /*  else fall through to default case  */
  //  case 0x11:  /*  bytes  */
  //  default:
//      my_copy((short *)(ch->wr_cur+ch->was_written), (const short *)data, block);
      memcpy((void *)(ch->wr_cur+ch->was_written), data, block);
      bytes_xferred = block;
  /*    break;
    }*/
    ch->was_written += block;
    ch->wr_silence = 0;

    release_spinlock(&ch->wr_lock);
    restore_interrupts(cp);

#if DEBUG
    if(ch->wr_cur == ch->wr_cur_prv)
    {
      int tt = (system_time() - ch->time_prv)/1000;
      TRACE("%d ms block - %x, to_write: %x ,size: %x,was_written: %x\n", tt, block, to_write, ch->wr_size, ch->was_written);
      ch->time_prv = system_time();
    }
    
    ch->wr_cur_prv = ch->wr_cur;
#endif

    data = ((char *)data)+bytes_xferred;
    written += bytes_xferred;
    to_write -= block;
  }

  *nwritten = written;
  release_sem(ch->wr_entry);

  return B_OK;
}

bool playback_interrupt(sis7018_ch * ch, bool bHalf)
{
  bool ret = false;
//  pcm_dev * port = &dev->pcm;
//  sis7018_ch *ch = &dev->pcm.play_channel; 
  volatile uchar * ptr;
//  uint32 addr;
  uint32 offs;
  bigtime_t st = system_time();
//  int32 ww;
//  sis7018_ch ch2 = *ch;

#if 0
ddprintf(("sonic_vibes: dma_a 0x%x+0x%x\n", PCI_IO_RD_32((int)port->dma_c), PCI_IO_RD_32((int)port->dma_c+4)));
#endif
//  KTRACE(); /* */
  acquire_spinlock(&ch->wr_lock);

  if (ch->write_sem < 0)
  {
    dprintf("spurious play interrupt!\n");
    release_spinlock(&ch->wr_lock);
    return false;
  }
  /* copy possible silence into playback buffer */

  if (ch->was_written > 0 && ch->was_written < ch->wr_size)
 // {
/*    if (port->config.format == 0x11)
    {
      memset((void *)(port->wr_cur+port->was_written), 0x80, port->wr_size-port->was_written);
    }
    else {*/
      memset((void *)(ch->wr_cur+ch->was_written), 0, ch->wr_size-ch->was_written);
  //  }
//  }

  /* because the system may be lacking and not hand us the */
  /* interrupt in time, we check which half is currently being */
  /* played, and set the pointer to the other half */

//  addr = PCI_IO_RD_32((uint32)port->dma_a);
//  if ((offs = addr-(uint32)port->card->low_phys) < port->wr_size)
//  hw_rdch(&ch2);
  
   // select channel ...
  offs = hw_read(ch->card, TR_REG_CIR, 4);
  offs &= ~TR_CIR_MASK;
  offs |= ch->index & 0x3f;
  hw_write(ch->card, TR_REG_CIR, offs, 4);

  // read current cso
  switch(ch->card->type->ids.chip_id)
  {
    case SPA_PCI_ID:
    case ALI_PCI_ID:
    case TDX_PCI_ID:
    case xDX_PCI_ID:
      offs=((hw_read(ch->card, TR_REG_CHNBASE, 4) & 0xffff0000) >> 16) * 4;
    break;
    case TNX_PCI_ID:
      offs=(hw_read(ch->card, TR_REG_CHNBASE, 4) & 0x00ffffff) * 4;
    break; 
  }
    
  if (offs < ch->wr_size)
    ptr = ch->wr_2;
  else
    ptr = ch->wr_1;
    
  ch->wr_total += ch->mem_size/2;
  /* compensate for interrupt latency */
  /* assuming 4 byte frames */
  ch->wr_time = st-(offs&(ch->mem_size/2-1))*250000LL/(int64)48000;
  
/*  if ((ww = atomic_add(&ch->wr_time_wait, -1)) > 0)
  {
    release_sem_etc(ch->wr_time_sem, 1, B_DO_NOT_RESCHEDULE);
    ret = true;
  }
  else
  {
*///    atomic_add(&ch->wr_time_wait, 1); /* re-set to 0 */
//  }

  if (ch->wr_cur == ptr)
  {
    ch->wr_skipped++;
    dprintf("write skipped %ld\n", ch->wr_skipped);
  }
  
  ch->wr_cur = ptr;
  ch->was_written = 0;

  /* check for client there to write into buffer */

  if (atomic_add(&ch->write_waiting, -1) > 0)
  {
#if DEBUG
    int_cnt++;
    the_time = st;
#endif
    release_sem_etc(ch->write_sem, 1, B_DO_NOT_RESCHEDULE);
    ret = true;
  }
  else
  {
    atomic_add(&ch->write_waiting, 1);
    /* if none there, fill with silence */
    if (ch->wr_silence < ch->mem_size)
    {
    /*  if (port->config.format == 0x11) {
        memset((void *)ptr, 0x80, port->wr_size);
      }
      else {*/
        memset((void *)ptr, 0, ch->wr_size);
      //}
      ch->wr_silence += ch->wr_size;
    }
  }
  /* copying will be done in user thread */

  release_spinlock(&ch->wr_lock);
  return ret;
}

/* pcm_control - handle ioctl calls */
static status_t pcm_control (void* cookie, uint32 op, void* arg, size_t len)
{
  status_t err = B_BAD_VALUE;
  pcm_dev * port = (pcm_dev *)cookie;
//  TRACE("pcm_control(%p, %lx)\n", cookie, op);
  switch (op)
  {
  case SOUND_GET_PARAMS:
  //  TRACE("SOUND_GET_PARAMS\n");
    err = ac97get_params(port->card, (sound_setup *)arg);
  break;
  case SOUND_SET_PARAMS:
  //  TRACE("SOUND_SET_PARAMS\n");
    err = ac97set_params(port->card, (sound_setup *)arg);
  break;
  case SOUND_SET_PLAYBACK_COMPLETION_SEM:
  //  TRACE("SOUND_SET_PLAYBACK_COMPLETION_SEM\n");
    port->old_play_sem = *(sem_id *)arg;
    err = B_OK;
    break;
  case SOUND_SET_CAPTURE_COMPLETION_SEM:
  //  TRACE("SOUND_SET_CAPTURE_COMPLETION_SEM\n");
    port->old_cap_sem = *(sem_id *)arg;
    err = B_OK;
    break;
  case SOUND_UNSAFE_WRITE:
  {
    sis7018_ch *ch = &((sis7018_dev *)cookie)->pcm.play_channel;
    audio_buffer_header * buf = (audio_buffer_header *)arg;
    size_t n = buf->reserved_1-sizeof(*buf);
    //TRACE("%ld %ld %ld %ld\n", buf->reserved_1, buf->reserved_2, buf->sample_clock, buf->time );
    //TRACE("SOUND_UNSAFE_WRITE %d\n", n);
    err = pcm_write(cookie, 0, buf+1, &n);
    buf->time = ch->wr_time;
    buf->sample_clock = ch->wr_total/4 * 1000 / 48;
    err |= release_sem(port->old_play_sem);
 //   err = B_OK;
  }
    break;
  case SOUND_UNSAFE_READ: 
    TRACE("SOUND_UNSAFE_READ\n");
 //   ASSERT(0);
    err = release_sem(port->old_cap_sem);
    err = B_IO_ERROR;
    break;
  case SOUND_LOCK_FOR_DMA:
    TRACE("SOUND_LOCK_FOR_DMA\n");
    err = B_OK;
    break;
  case SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE:
    {
      int32 new_size = (int32)arg;
      TRACE("SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE: 0x%x\n", new_size);
      hw_pchannel_setblocksize(port, new_size);
      err = B_OK;
    }
    break;
  case SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE:
   // TRACE("SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE\n");
  //  config.rec_buf_size = (int32)data;
  //  configure = true;
    err = B_OK;
    break;
  case SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE:
    TRACE("SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE\n");
    *(int32*)arg = port->play_channel.mem_size/2;
    err = B_OK;
    break;
  case SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE:
  // TRACE("SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE\n");
  //  *(int32*)data = config.rec_buf_size;
   err = B_OK;
   break;
  default:
    TRACE("pcm_control : unknown code %ld\n", op);
    err = B_BAD_VALUE;
    break;
  }
  return err;
}

/* pcm_close - handle close() calls */
static status_t pcm_close (void* cookie)
{
  pcm_dev * port = (pcm_dev *)cookie;
  TRACE("pcm_close(%p)\n", cookie);
  
  acquire_sem(port->init_sem);

  if (atomic_add(&port->open_count, -1) == 1)
  {
    TRACE("pcm_close: last close\n");
    hw_stop(port);
    hw_pchannel_free(port);
   }
  else
  {
    TRACE("pcm_close: previous closes\n");
  }

  release_sem(port->init_sem);
  return B_OK;
}


/* pcm_free - called after the last device is closed, and after all i/o is complete. */
static status_t pcm_free (void* cookie)
{
  pcm_dev *port = (pcm_dev *)cookie;
  TRACE("pcm_free(%p)\n", cookie);

  acquire_sem(port->init_sem);
  
  if (port->open_count == 0)
  {
    hw_decrement_interrupt_handler(port->card);
  }
  
  release_sem(port->init_sem);
  return B_OK;
}

/* function pointers for the device hooks entry points */
device_hooks pcm_hooks =
{
  pcm_open,    /* -> open entry point */
  pcm_close,   /* -> close entry point */
  pcm_free,    /* -> free cookie */
  pcm_control, /* -> control entry point */
  pcm_read,    /* -> read entry point */
  pcm_write    /* -> write entry point */
};
