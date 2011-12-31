/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "cmedia_pci.h"
#include "cm_private.h"

#include <string.h>
#include <stdio.h>

#if !defined(_KERNEL_EXPORT_H)
#include <KernelExport.h>
#endif


#if DEBUG
#define KPRINTF(x) kprintf x
#else
#define KPRINTF(x)
#endif

EXPORT status_t init_hardware(void);
EXPORT status_t init_driver(void);
EXPORT void uninit_driver(void);
EXPORT const char ** publish_devices(void);
EXPORT device_hooks * find_device(const char *);


static char pci_name[] = B_PCI_MODULE_NAME;
static pci_module_info	*pci;
static char gameport_name[] = "generic/gameport/v1";
generic_gameport_module * gameport;
static char mpu401_name[] = B_MPU_401_MODULE_NAME;
generic_mpu401_module * mpu401;

#define DO_JOY 1
#define DO_MIDI 1
#define DO_PCM 1
#define DO_MUX 0
#define DO_MIXER 0

#if DO_MIDI
extern device_hooks midi_hooks;
#endif /* DO_MIDI */
#if DO_JOY
extern device_hooks joy_hooks;
#endif /* DO_JOY */
#if DO_PCM
extern device_hooks pcm_hooks;
#endif /* DO_PCM */
#if DO_MUX
extern device_hooks mux_hooks;
#endif /* DO_MUX */
#if DO_MIXER
extern device_hooks mixer_hooks;
#endif /* DO_MIXER */


int32 num_cards;
cmedia_pci_dev cards[NUM_CARDS];
int num_names;
char * names[NUM_CARDS * 7 + 1];
/* vuchar *io_base; */


/* ----------
	PCI_IO_RD - read a byte from pci i/o space
----- */

uint8
PCI_IO_RD (int offset)
{
	return (*pci->read_io_8) (offset);
}


/* ----------
	PCI_IO_RD_32 - read a 32 bit value from pci i/o space
----- */

uint32
PCI_IO_RD_32 (int offset)
{
	return (*pci->read_io_32) (offset);
}
/* ----------
	PCI_IO_WR - write a byte to pci i/o space
----- */

void
PCI_IO_WR (int offset, uint8 val)
{
	(*pci->write_io_8) (offset, val);
}


/* detect presence of our hardware */
status_t
init_hardware(void)
{
	int ix=0;
	pci_info info;
	status_t err = ENODEV;

	ddprintf(("cmedia_pci: init_hardware()\n"));

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == CMEDIA_PCI_VENDOR_ID &&
			(info.device_id == CMEDIA_8338A_DEVICE_ID ||
			 info.device_id == CMEDIA_8338B_DEVICE_ID ||
			 info.device_id == CMEDIA_8738A_DEVICE_ID ||
			 info.device_id == CMEDIA_8738B_DEVICE_ID )) {
			err = B_OK;
		}
		ix++;
	}
#if defined(__POWERPC__) && 0
	{
		char		area_name [32];
		area_info	area;
		area_id		id;

		sprintf (area_name, "pci_bus%d_isa_io", info.bus);
		id = find_area (area_name);
		if (id < 0)
			err = id;
		else if ((err = get_area_info (id, &area)) == B_OK)
			io_base = area.address;
	}
#endif

	put_module(pci_name);

	return err;
}


void set_direct(cmedia_pci_dev * card, int regno, uchar value, uchar mask)
{
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD(card->enhanced+regno);
		value = (value&mask)|(old&~mask);
	}
	PCI_IO_WR(card->enhanced+regno, value);
	ddprintf(("cmedia_pci: CM%02x  = %02x\n", regno, value));
}


uchar get_direct(cmedia_pci_dev * card, int regno)
{
	uchar ret = PCI_IO_RD(card->enhanced+regno);
	return ret;
}



void set_indirect(cmedia_pci_dev * card, int regno, uchar value, uchar mask)
{
	PCI_IO_WR(card->enhanced+0x23, regno);
	EIEIO();
	if (mask == 0) {
		return;
	}
	if (mask != 0xff) {
		uchar old = PCI_IO_RD(card->enhanced+0x22);
		value = (value&mask)|(old&~mask);
	}
	PCI_IO_WR(card->enhanced+0x22, value);
	EIEIO();
	ddprintf(("cmedia_pci: CMX%02x = %02x\n", regno, value));
}



uchar get_indirect(cmedia_pci_dev * card,int regno)
{
	uchar ret;
	PCI_IO_WR(card->enhanced+0x23, regno);
	EIEIO();
	ret = PCI_IO_RD(card->enhanced+0x22);
	return ret;
}


#if 0
void dump_card(cmedia_pci_dev * card)
{
	int ix;
	dprintf("\n");
	dprintf("CM:   ");
	for (ix=0; ix<6; ix++) {
		if (ix == 2 || ix == 3) dprintf("   ");
		else dprintf(" %02x", get_direct(card, ix));
	}
	for (ix=0; ix<0x32; ix++) {
		if (!(ix & 7)) {
			dprintf("\nCMX%02x:", ix);
		}
		dprintf(" %02x", get_indirect(card, ix));
	}
	dprintf("\n");
	dprintf("\n");
}
#else
void dump_card(cmedia_pci_dev * card)
{
}
#endif


static void
disable_card_interrupts(cmedia_pci_dev * card)
{
	set_direct(card, 0x0e, 0x00, 0x03);
}


static status_t
setup_dma(cmedia_pci_dev * card)
{
	/* we're appropriating some ISA space here... */
	/* need kernel support to do it right */
	const uint16 base = card->enhanced + 0x80;
	ddprintf(("cmedia_pci: dma base is 0x%04x\n", base));
	if (base == 0)
		return B_DEV_RESOURCE_CONFLICT;
	card->dma_base = base;
	return B_OK;
}


static void
set_default_registers(cmedia_pci_dev * card)
{
	static uchar values[] = {
#ifdef DO_JOY
		0x04, 0x02, 0x02,	/* enable joystick */
#endif

		0x0a, 0x01, 0x01,	/* enable SPDIF inverse before SPDIF_LOOP */
		0x04, 0x80, 0x80,	/* enable SPDIF_LOOP */

		0x1a, 0x00, 0x20,	/* SPD32SEL disable */
		0x1a, 0x00, 0x10,	/* SPDFLOOPI disable */

		0x1b, 0x04, 0x04,	/* dual channel mode enable */
		0x1a, 0x00, 0x80,	/* Double DAC structure disable */

		0x24, 0x00, 0x02,	/* 3D surround disable */

		0x24, 0x00, 0x01,	/* disable SPDIF_IN PCM to DAC */
#ifdef DO_MIDI
		0x04, 0x04, 0x04,	/* enable MPU-401 */
		0x17, 0x00, 0x60,	/* default at 0x330 */
#endif
	};
	uchar * ptr = values;

	while (ptr < values+sizeof(values)) {
		set_direct(card, ptr[0], ptr[1], ptr[2]);
		ptr += 3;
	}
}


static void
make_device_names(cmedia_pci_dev * card)
{
	char * name = card->name;
	sprintf(name, "cmedia_pci/%ld", card-cards + 1);

#if DO_MIDI
	sprintf(card->midi.name, "midi/%s", name);
	names[num_names++] = card->midi.name;
#endif /* DO_MIDI */
#if DO_JOY
	sprintf(card->joy.name1, "joystick/%s", name);
	names[num_names++] = card->joy.name1;
#endif /* DO_JOY */
#if DO_PCM
	/* cmedia_pci DMA doesn't work when physical NULL isn't NULL from PCI */
	/* this is a hack to not export bad devices on BeBox hardware */
	if ((*pci->ram_address)(NULL) == NULL) {
		sprintf(card->pcm.name, "audio/raw/%s", name);
		names[num_names++] = card->pcm.name;
		sprintf(card->pcm.oldname, "audio/old/%s", name);
		names[num_names++] = card->pcm.oldname;
	}
#endif /* DO_PCM */
#if DO_MUX
	sprintf(card->mux.name, "audio/mux/%s", name);
	names[num_names++] = card->mux.name;
#endif /* DO_MUX */
#if DO_MIXER
	sprintf(card->mixer.name, "audio/mix/%s", name);
	names[num_names++] = card->mixer.name;
#endif /* DO_MIXER */
	names[num_names] = NULL;
}


/* We use the SV chip in ISA DMA addressing mode, which is 24 bits */
/* so we need to find suitable, locked, contiguous memory in that */
/* physical address range. */

static status_t
find_low_memory(cmedia_pci_dev * card)
{
	size_t low_size = (MIN_MEMORY_SIZE + (B_PAGE_SIZE - 1))
		&~ (B_PAGE_SIZE - 1);
	physical_entry where;
	size_t trysize;
	area_id curarea;
	void * addr;
	char name[DEVNAME];

	sprintf(name, "%s_low", card->name);
	if (low_size < MIN_MEMORY_SIZE) {
		low_size = MIN_MEMORY_SIZE;
	}
	trysize = low_size;

	curarea = find_area(name);
	if (curarea >= 0) {	/* area there from previous run */
		area_info ainfo;
		ddprintf(("cmedia_pci: testing likely candidate...\n"));
		if (get_area_info(curarea, &ainfo)) {
			ddprintf(("cmedia_pci: no info\n"));
			goto allocate;
		}
		/* test area we found */
		trysize = ainfo.size;
		addr = ainfo.address;
		if (trysize < low_size) {
			ddprintf(("cmedia_pci: too small (%lx)\n", trysize));
			goto allocate;
		}
		if (get_memory_map(addr, trysize, &where, 1) < B_OK) {
			ddprintf(("cmedia_pci: no memory map\n"));
			goto allocate;
		}
		if ((where.address & ~(phys_addr_t)0xffffff) != 0) {
			ddprintf(("cmedia_pci: bad physical address\n"));
			goto allocate;
		}
		if (ainfo.lock < B_FULL_LOCK || where.size < low_size) {
			ddprintf(("cmedia_pci: lock not contiguous\n"));
			goto allocate;
		}
dprintf("cmedia_pci: physical %#" B_PRIxPHYSADDR "  logical %p\n",
where.address, ainfo.address);
		goto a_o_k;
	}

allocate:
	if (curarea >= 0) {
		delete_area(curarea); /* area didn't work */
		curarea = -1;
	}
	ddprintf(("cmedia_pci: allocating new low area\n"));

	curarea = create_area(name, &addr, B_ANY_KERNEL_ADDRESS,
		trysize, B_LOMEM, B_READ_AREA | B_WRITE_AREA);
	ddprintf(("cmedia_pci: create_area(%lx) returned %lx logical %p\n",
		trysize, curarea, addr));
	if (curarea < 0) {
		goto oops;
	}
	if (get_memory_map(addr, low_size, &where, 1) < 0) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	ddprintf(("cmedia_pci: physical %p\n", where.address));
	if ((where.address & ~(phys_addr_t)0xffffff) != 0) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	if (((where.address + low_size) & ~(phys_addr_t)0xffffff) != 0) {
		delete_area(curarea);
		curarea = B_ERROR;
		goto oops;
	}
	/* hey, it worked! */
	if (trysize > low_size) {	/* don't waste */
		resize_area(curarea, low_size);
	}

oops:
	if (curarea < 0) {
		dprintf("cmedia_pci: failed to create low_mem area\n");
		return curarea;
	}
a_o_k:
	ddprintf(("cmedia_pci: successfully found or created low area!\n"));
	card->low_size = low_size;
	card->low_mem = addr;
	card->low_phys = (vuchar *)(addr_t)where.address;
	card->map_low = curarea;
	return B_OK;
}


static status_t
setup_cmedia_pci(cmedia_pci_dev * card)
{
	status_t err = B_OK;
/*	cpu_status cp; */

	ddprintf(("cmedia_pci: setup_cmedia_pci(%p)\n", card));

	if ((card->pcm.init_sem = create_sem(1, "cm pcm init")) < B_OK)
		goto bail;
#if 1
	if ((*mpu401->create_device)(0x330, &card->midi.driver,
#else
	if ((*mpu401->create_device)(card->info.u.h0.base_registers[3], &card->midi.driver,
#endif
		0, midi_interrupt_op, &card->midi) < B_OK)
		goto bail3;
#if 1
	if ((*gameport->create_device)(0x201, &card->joy.driver) < B_OK)
#else
	if ((*gameport->create_device)(card->info.u.h0.base_registers[4], &card->joy.driver) < B_OK)
#endif
		goto bail4;
	ddprintf(("midi %p  gameport %p\n", card->midi.driver, card->joy.driver));
	card->midi.card = card;

	err = find_low_memory(card);
	if (err < B_OK) {
		goto bail5;
	}

	//cp = disable_interrupts();
	//acquire_spinlock(&card->hardware);

	make_device_names(card);
	card->enhanced = card->info.u.h0.base_registers[0];
	ddprintf(("cmedia_pci: %s enhanced at %x\n", card->name, card->enhanced));

	ddprintf(("cmedia_pci: revision %x\n", get_indirect(card, 0x15)));

	disable_card_interrupts(card);
	if (setup_dma(card) != B_OK) {
		dprintf("cmedia pci: can't setup DMA\n");
		goto bail6;
	}

	set_default_registers(card);

	//release_spinlock(&card->hardware);
	//restore_interrupts(cp);

	return B_OK;

bail6:
	//	deallocate low memory
bail5:
	(*gameport->delete_device)(card->joy.driver);
bail4:
	(*mpu401->delete_device)(card->midi.driver);
bail3:
	delete_sem(card->pcm.init_sem);
bail:
	return err < B_OK ? err : B_ERROR;
}


static int
debug_cmedia(int argc, char * argv[])
{
	int ix = 0;
	if (argc == 2) {
		ix = parse_expression(argv[1]) - 1;
	}
	if (argc > 2 || ix < 0 || ix >= num_cards) {
		dprintf("cmedia_pci: dude, you gotta watch your syntax!\n");
		return -1;
	}
	dprintf("%s: enhanced registers at 0x%x\n", cards[ix].name,
		cards[ix].enhanced);
	dprintf("%s: open %ld   dma_a at 0x%x   dma_c 0x%x\n", cards[ix].pcm.name,
		cards[ix].pcm.open_count, cards[ix].pcm.dma_a, cards[ix].pcm.dma_c);
	if (cards[ix].pcm.open_count) {
		dprintf("    dma_a: 0x%lx+0x%lx   dma_c: 0x%lx+0x%lx\n",
			PCI_IO_RD_32((int)cards[ix].pcm.dma_a), PCI_IO_RD_32((int)cards[ix].pcm.dma_a+4),
			PCI_IO_RD_32((int)cards[ix].pcm.dma_c), PCI_IO_RD_32((int)cards[ix].pcm.dma_c+4));
	}
	return 0;
}


status_t
init_driver(void)
{
	pci_info info;
	int ix = 0;
	status_t err;

	num_cards = 0;

	ddprintf(("cmedia_pci: init_driver()\n"));

	if (get_module(pci_name, (module_info **)&pci))
		return ENOSYS;

	if (get_module(gameport_name, (module_info **)&gameport)) {
		put_module(pci_name);
		return ENOSYS;
	}
	ddprintf(("MPU\n"));
	if (get_module(mpu401_name, (module_info **)&mpu401)) {
		put_module(gameport_name);
		put_module(pci_name);
		return ENOSYS;
	}

	ddprintf(("MPU: %p\n", mpu401));

	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == CMEDIA_PCI_VENDOR_ID &&
			(info.device_id == CMEDIA_8338A_DEVICE_ID ||
			 info.device_id == CMEDIA_8338B_DEVICE_ID ||
			 info.device_id == CMEDIA_8738A_DEVICE_ID ||
			 info.device_id == CMEDIA_8738B_DEVICE_ID )) {
			if (num_cards == NUM_CARDS) {
				dprintf("Too many C-Media cards installed!\n");
				break;
			}
			memset(&cards[num_cards], 0, sizeof(cmedia_pci_dev));
			cards[num_cards].info = info;
#ifdef __HAIKU__
			if ((err = (*pci->reserve_device)(info.bus, info.device, info.function,
				DRIVER_NAME, &cards[num_cards])) < B_OK) {
				dprintf("%s: failed to reserve_device(%d, %d, %d,): %s\n",
					DRIVER_NAME, info.bus, info.device, info.function,
					strerror(err));
				continue;
			}
#endif
			if (setup_cmedia_pci(&cards[num_cards])) {
				dprintf("Setup of C-Media %ld failed\n", num_cards+1);
#ifdef __HAIKU__
				(*pci->unreserve_device)(info.bus, info.device, info.function,
					DRIVER_NAME, &cards[num_cards]);
#endif
			}
			else {
				num_cards++;
			}
		}
		ix++;
	}
	if (!num_cards) {
		KPRINTF(("no cards\n"));
		put_module(mpu401_name);
		put_module(gameport_name);
		put_module(pci_name);
		ddprintf(("cmedia_pci: no suitable cards found\n"));
		return ENODEV;
	}

#if DEBUG
	add_debugger_command("cmedia", debug_cmedia, "cmedia [card# (1-n)]");
#endif
	return B_OK;
}


static void
teardown_cmedia_pci(cmedia_pci_dev * card)
{
	static uchar regs[] = {
#ifdef DO_JOY
		0x04, 0x00, 0x02,	/* enable joystick */
#endif
#ifdef DO_MIDI
		0x04, 0x00, 0x04,	/* enable MPU-401 */
#endif
	};
	uchar * ptr = regs;
	cpu_status cp;

	/* remove created devices */
	(*gameport->delete_device)(card->joy.driver);
	(*mpu401->delete_device)(card->midi.driver);

	cp = disable_interrupts();
	acquire_spinlock(&card->hardware);

	while (ptr < regs + sizeof(regs)) {
		set_direct(card, ptr[0], ptr[1], ptr[2]);
		ptr += 3;
	}
	disable_card_interrupts(card);

	release_spinlock(&card->hardware);
	restore_interrupts(cp);

	delete_sem(card->pcm.init_sem);

#ifdef __HAIKU__
	(*pci->unreserve_device)(card->info.bus, card->info.device,
		card->info.function, DRIVER_NAME, card);
#endif
}


void
uninit_driver(void)
{
	int ix, cnt = num_cards;
	num_cards = 0;

	ddprintf(("cmedia_pci: uninit_driver()\n"));
	remove_debugger_command("cmedia", debug_cmedia);

	for (ix = 0; ix < cnt; ix++) {
		teardown_cmedia_pci(&cards[ix]);
	}
	memset(&cards, 0, sizeof(cards));
	put_module(mpu401_name);
	put_module(gameport_name);
	put_module(pci_name);
}


const char **
publish_devices(void)
{
	int ix = 0;
	ddprintf(("cmedia_pci: publish_devices()\n"));

	for (ix = 0; names[ix]; ix++) {
		ddprintf(("cmedia_pci: publish %s\n", names[ix]));
	}
	return (const char **)names;
}


device_hooks *
find_device(const char * name)
{
	int ix;

	ddprintf(("cmedia_pci: find_device(%s)\n", name));

	for (ix = 0; ix < num_cards; ix++) {
#if DO_MIDI
		if (!strcmp(cards[ix].midi.name, name)) {
			return &midi_hooks;
		}
#endif /* DO_MIDI */
#if DO_JOY
		if (!strcmp(cards[ix].joy.name1, name)) {
			return &joy_hooks;
		}
#endif /* DO_JOY */
#if DO_PCM
		if (!strcmp(cards[ix].pcm.name, name)) {
			return &pcm_hooks;
		}
		if (!strcmp(cards[ix].pcm.oldname, name)) {
			return &pcm_hooks;
		}
#endif /* DO_PCM */
#if DO_MUX
		if (!strcmp(cards[ix].mux.name, name)) {
			return &mux_hooks;
		}

#endif /* DO_MUX */
#if DO_MIXER
		if (!strcmp(cards[ix].mixer.name, name)) {
			return &mixer_hooks;
		}
#endif /* DO_MIXER */
	}
	ddprintf(("cmedia_pci: find_device(%s) failed\n", name));
	return NULL;
}

int32	api_version = B_CUR_DRIVER_API_VERSION;

static int32
cmedia_pci_interrupt(void * data)
{
	cpu_status cp = disable_interrupts();
	cmedia_pci_dev * card = (cmedia_pci_dev *)data;
	uchar status;
	int32 handled = B_UNHANDLED_INTERRUPT;

/*	KTRACE(); / * */
	acquire_spinlock(&card->hardware);

	status = get_direct(card, 0x10);

#if DEBUG
/*	kprintf("%x\n", status); / * */
#endif
#if DO_PCM
	if (status & 0x02) {
		if (dma_c_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			handled = B_HANDLED_INTERRUPT;
		}
		/* acknowledge interrupt */
		set_direct(card, 0x0e, 0x00, 0x02);
		set_direct(card, 0x0e, 0x02, 0x02);
	}
	if (status & 0x01) {
		if (dma_a_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
		else {
			handled = B_HANDLED_INTERRUPT;
		}
		/* acknowledge interrupt */
		set_direct(card, 0x0e, 0x00, 0x01);
		set_direct(card, 0x0e, 0x01, 0x01);
	}
#endif
#if DO_MIDI
	status = get_direct(card, 0x12);
	if (status & 0x01) {
		if (midi_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		} else {
			handled = B_HANDLED_INTERRUPT;
		}
	}
#endif

	/*  Sometimes, the Sonic Vibes will receive a byte of Midi data...
	**  And duly note it in the MPU401 status register...
	**  And generate an interrupt...
	**  But not bother setting the midi interrupt bit in the ISR.
	**  Thanks a lot, S3.
	*/
	if (handled == B_UNHANDLED_INTERRUPT) {
		if (midi_interrupt(card)) {
			handled = B_INVOKE_SCHEDULER;
		}
	}

/*	KTRACE(); / * */
	release_spinlock(&card->hardware);
	restore_interrupts(cp);

	return handled;
//	return (handled == B_INVOKE_SCHEDULER) ? B_HANDLED_INTERRUPT : handled;
}


void
increment_interrupt_handler(cmedia_pci_dev * card)
{
	KPRINTF(("cmedia_pci: increment_interrupt_handler()\n"));
	if (atomic_add(&card->inth_count, 1) == 0) {
	// !!!
		KPRINTF(("cmedia_pci: intline %d int %p\n", card->info.u.h0.interrupt_line, cmedia_pci_interrupt));
		install_io_interrupt_handler(card->info.u.h0.interrupt_line,
			cmedia_pci_interrupt, card, 0);
	}
}


void
decrement_interrupt_handler(cmedia_pci_dev * card)
{
	KPRINTF(("cmedia_pci: decrement_interrupt_handler()\n"));
	if (atomic_add(&card->inth_count, -1) == 1) {
		KPRINTF(("cmedia_pci: remove_io_interrupt_handler()\n"));
		remove_io_interrupt_handler(card->info.u.h0.interrupt_line, cmedia_pci_interrupt, card);
	}
}


