/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * See the file "License" for details.
 */
#include <stdio.h> //sprintf
#include <string.h>
#include <unistd.h> //posix file i/o - create, write, close 
#include <driver_settings.h>
#include "sis7018.h"
#include "sis7018hw.h"
#include "ac97.h"

#if DO_PCM
#include "pcm.h"
#endif

#if DO_MIDI
#include "midi_driver.h"
#include "midi.h"
#endif

#define NUM_CARDS 3

int32 num_cards;
sis7018_dev cards[NUM_CARDS];
int num_names;
char * names[NUM_CARDS*7+1];

#if DEBUG
bool b_log = true;
#else
bool b_log = false;
#endif
bool b_log_file = false;
bool b_log_append = false;
bool b_trace_ac97 = false;
bool use_io_regs = false;

static const char *private_log_path="/boot/home/sis7018.log";
static sem_id loglock;

static void reload_sis7018_setting()
{
  void *settingshandle; 
  settingshandle = load_driver_settings(CHIPNAME); 
#if !DEBUG
  b_log = get_driver_boolean_parameter(settingshandle, "debug_output", b_log, true);
#endif
  b_log_file = get_driver_boolean_parameter(settingshandle, "debug_output_in_file", b_log_file, true);
  b_log_append = ! get_driver_boolean_parameter(settingshandle, "debug_output_file_rewrite", !b_log_append, true);
  b_trace_ac97 = get_driver_boolean_parameter(settingshandle, "debug_trace_ac97", b_trace_ac97, true);
  use_io_regs = get_driver_boolean_parameter(settingshandle, "use_io_registers", use_io_regs, true);
  unload_driver_settings(settingshandle);
}

void create_log()
{
  int flags = O_WRONLY | O_CREAT | ((!b_log_append) ? O_TRUNC : 0);
  if(!b_log_file)
    return;
  close(open(private_log_path, flags, 0666));
  loglock = create_sem(1,"sis7018 logfile");
}

void sis7018_trace(bool b_force, char *fmt, ...)
{
  if(!(b_force || b_log))
    return;
  {
    va_list arg_list;
    static char *prefix = "\33[32m"CHIPNAME":\33[0m";
    static char buf[1024];
    char *buf_ptr = buf;
    if(!b_log_file)
    {
      strcpy(buf, prefix);
      buf_ptr += strlen(prefix);
    }

    va_start(arg_list, fmt);
    vsprintf(buf_ptr, fmt, arg_list);
    va_end(arg_list);

    if(b_log_file)
    {
      int fd;
      if(!b_trace_ac97)
        acquire_sem(loglock);
      fd = open(private_log_path, O_WRONLY | O_APPEND);
      write(fd, buf, strlen(buf));
      close(fd);
      if(!b_trace_ac97)
        release_sem(loglock);
    }
    else
      dprintf(buf);
  }
}

card_type card_types[]=
{
  {{TDX_PCI_ID}, "Trident DX"},
  {{xDX_PCI_ID}, "Trident XX"},
  {{TNX_PCI_ID}, "Trident NX"},
  {{ALI_PCI_ID}, "ALi 5451"},
  {{SPA_PCI_ID}, "SiS 7018"},
};

#ifdef DEBUG 
int _assert_(char *a, int b, char *c) 
{
  char sz[1024];
  sprintf(sz, CHIPNAME":tripped assertion in %s/%d (%s)", a, b, c);
  TRACE("tripped assertion in %s/%d (%s)\n", a, b, c); 
  kernel_debugger(sz); 
  return 0; 
}
#endif

static void make_device_names(sis7018_dev * card)
{
  char * name = card->name;
  sprintf(name, "%s/%ld", card->type->chip_name, card-cards+1);

#if DO_MIDI
  sprintf(card->midi.name, "midi/%s", name);
  names[num_names++] = card->midi.name;
#endif //DO_MIDI

#if DO_PCM
//  sprintf(card->pcm.name, "audio/raw/%s", name);
//  names[num_names++] = card->pcm.name;
  sprintf(card->pcm.oldname, "audio/old/%s", name);
  names[num_names++] = card->pcm.oldname;
#endif //DO_PCM

  names[num_names] = NULL;
}

static status_t setup_hardware(  sis7018_dev * card)
{
  status_t err = B_OK;
  int32 command_reg;

  TRACE("setup_hardware(%p)\n", card);

  if ((card->pcm.init_sem = create_sem(1, "sis7018 pcm init")) < B_OK)
    goto bail;

#if DO_MIDI //TODO: below?
  if ((*mpu401->create_device)(card->info.u.h0.base_registers[3], &card->midi.driver,
      0, midi_interrupt_op, &card->midi) < B_OK)
        goto bail3;
  TRACE("midi %p\n", card->midi.driver);
  card->midi.card = card;
#endif //DO_MIDI

  make_device_names(card);
  
  command_reg = (*pci->read_pci_config)(card->info.bus, card->info.device, card->info.function, 
                                            PCI_command, 2); 
//  TRACE("PCI command config was: %lx\n", command_reg);
  command_reg |= PCI_command_io | PCI_command_memory | PCI_command_master; 
  (*pci->write_pci_config)(card->info.bus, card->info.device, card->info.function,
                            PCI_command, 2, command_reg); 
//  TRACE("PCI command config set to: %lx\n", command_reg);
  
 if(use_io_regs)
 {  
  card->io_base = card->info.u.h0.base_registers[0];
  TRACE("%s base at %x\n", card->name, card->io_base);
 }
 else // USE_IO_RANGE
 {
  TRACE("%s memory %lx length %lx\n", card->name, card->info.u.h0.base_registers[1], card->info.u.h0.base_register_sizes[1]);
  card->regs_area = map_physical_memory("Regs sis7018",
                          (void *)card->info.u.h0.base_registers[1], 
                             card->info.u.h0.base_register_sizes[1], 
                               B_ANY_KERNEL_ADDRESS, 
                                  B_READ_AREA + B_WRITE_AREA,
                                    &card->regs_mem_base);

  TRACE("%s registers memory base at 0x%p\n", card->name, card->regs_mem_base);
 }//  USE_IO_RANGE

  err = hw_initialize(card);
  return err;

#if DO_MIDI 
bail3:
#endif //DO_MIDI
  delete_sem(card->pcm.init_sem);
bail:
  return err < B_OK ? err : B_ERROR;
}

#if DEBUG
static int debug_sis7018(int argc, char * argv[])
{
  int err = (argc < 2 || argc > 3); 
  int card_id = 0;
  
  if(!err)
  {
    if(argc == 3)
      card_id = parse_expression(argv[2]);
    
    if(card_id < 0 || card_id > num_cards-1)
    {
      kprintf("ERROR: Wrong card_id: %d\n", card_id); 
      kprintf("\n"); 
      err = 1;
    }
    else
    {
     switch((char)*(argv[1]))
     {
       case 'd': hw_dump_device(card_id); break;
       case 'i': hw_dump_pci_info(card_id); break;
       case 'p': hw_dump_pcm_device(card_id); break;
       case 'm': hw_dump_midi_device(card_id); break;
       case 'c': hw_dump_play_channel(card_id); break;
       case 'a': ac97dump(card_id); break;
       default:
         kprintf("ERROR: Wrong command: %s\n",argv[1]);
         kprintf("\n");
         err=1; 
       break;
     }
    }
  }

  if (err) 
  { 
    kprintf("Syntax: "CHIPNAME" < command > [card id]\n"); 
    kprintf(" - where commands is:\n"); 
    kprintf("    d - dump device info\n"); 
    kprintf("    i - dump pci_info\n"); 
    kprintf("    p - dump pcm info\n"); 
    kprintf("    m - dump midi info\n"); 
    kprintf("    c - dump playback channel info\n"); 
    kprintf("    a - dump AC'97 registers info\n"); 
    kprintf(" - and card id is zero-based id of card to be investigated\n"); 
    kprintf("    default card id is 0\n");
    kprintf("    current cards count is %ld\n", num_cards); 
    kprintf("\n"); 
  }

  return 0;
}
#endif /* DEBUG */

/************** SYSTEM CALLBACKS ************************************/

int32 api_version = B_CUR_DRIVER_API_VERSION;

/* init_hardware - called once the first time the driver is loaded */
status_t init_hardware (void)
{
  int ix = 0, jx = 0;
  pci_info info;
  status_t err = ENODEV;

  TRACE("init_hardware()\n");
  
  if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
    return ENOSYS;

  while ((*pci->get_nth_pci_info)(ix, &info) == B_OK)
  {
    for(jx = 0; jx < sizeof(card_types)/sizeof(card_types[0]); jx++)
      if (info.vendor_id == card_types[jx].ids.ids.vendor_id
               && info.device_id == card_types[jx].ids.ids.card_id)
      {
        err = B_OK;
      }
    ix++;
  }
    
  put_module(B_PCI_MODULE_NAME);
  return err;
}

/* init_driver - optional function - called every time the drive is loaded. */
status_t init_driver (void)
{
  int ix=0, jx=0;
  pci_info info;
  num_cards = 0;
 
  reload_sis7018_setting();
  create_log();
  
  TRACE("\n>>> init_driver()\n");
  load_driver_symbols(CHIPNAME);

  if (get_module(B_PCI_MODULE_NAME, (module_info **) &pci))
    return ENOSYS;

#if DO_MIDI
  TRACE("MPU\n");
  if (get_module(B_MPU_401_MODULE_NAME, (module_info **) &mpu401))
  {
    put_module(B_PCI_MODULE_NAME);
    return ENOSYS;
  }
  TRACE("MPU: %p\n", mpu401);
#endif //DO_MIDI

  while ((*pci->get_nth_pci_info)(ix, &info) == B_OK)
  {
    for(jx = 0; jx < sizeof(card_types)/sizeof(card_types[0]); jx++)
     if (info.vendor_id == card_types[jx].ids.ids.vendor_id
               && info.device_id == card_types[jx].ids.ids.card_id)
     {
       if (num_cards == NUM_CARDS)
       {
         TRACE_ALWAYS("Too many cards installed! Only %d will be used.\n", NUM_CARDS);
         break;
       }
       
       memset(&cards[num_cards], 0, sizeof(sis7018_dev));
       cards[num_cards].info = info;
       cards[num_cards].type = &card_types[jx];
       if (setup_hardware(&cards[num_cards]))
       {
         TRACE_ALWAYS("Setup of %s driver  for %ld-th card failed\n", CHIPNAME, num_cards+1);
       }
       else
         num_cards++;
         
       TRACE_ALWAYS("%s <vendor:%x, card:%x> found.\n",
                card_types[jx].chip_name,
                 card_types[jx].ids.ids.vendor_id,
                  card_types[jx].ids.ids.card_id);
     }
    ix++;
  }
 
  if (!num_cards)
  {
#if DO_MIDI  
    put_module(B_MPU_401_MODULE_NAME);
#endif //DO_MIDI
    put_module(B_PCI_MODULE_NAME);
    TRACE("No suitable cards found\n");
    return ENODEV;
  }

#if DEBUG
  add_debugger_command(CHIPNAME, debug_sis7018, CHIPNAME" driver info.");
#endif
  return B_OK;
}

static void teardown_card(sis7018_dev *card)
{
  /* remove created devices */
#if DO_MIDI
  (*mpu401->delete_device)(card->midi.driver);
#endif //DO_MIDI

  if(!use_io_regs)
    delete_area(card->regs_area);

  delete_sem(card->pcm.init_sem);
}

/* uninit_driver - optional function - called every time the driver is unloaded */
void uninit_driver (void)
{
  int ix, cnt = num_cards;
  num_cards = 0;

  TRACE("<<< uninit_driver()\n\n");

#if DEBUG  
  remove_debugger_command(CHIPNAME, debug_sis7018);
#endif

  for (ix=0; ix<cnt; ix++)
  {
    teardown_card(&cards[ix]);
  }

  memset(&cards, 0, sizeof(cards));
#if DO_MIDI
  put_module(B_MPU_401_MODULE_NAME);
#endif // DO_MIDI  
  put_module(B_PCI_MODULE_NAME);
}

/* publish_devices - return a null-terminated array of devices
   supported by this driver. */
const char** publish_devices()
{
  int ix = 0;
  TRACE("publish_devices()\n");

  for (ix=0; names[ix]; ix++)
    TRACE("publish %s\n", names[ix]);
  
  return (const char **)names;
}

/* find_device - return ptr to device hooks structure for a
   given device name */
device_hooks* find_device(const char* name)
{
  int ix;
  TRACE("find_device(%s)\n", name);

  for (ix=0; ix<num_cards; ix++)
  {
#if DO_MIDI
    if (!strcmp(cards[ix].midi.name, name))
    {
      return &midi_hooks;
    }
#endif /* DO_MIDI */

#if DO_PCM
    if (!strcmp(cards[ix].pcm.name, name))
    {
      return &pcm_hooks;
    }
  
    if (!strcmp(cards[ix].pcm.oldname, name))
    {
      return &pcm_hooks;
    }
#endif //DO_PCM
  }
  
  TRACE("find_device(%s) failed\n", name);
  return NULL;
}

