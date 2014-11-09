/*
 * Copyright 2009-2010, François Revol, <revol@free.fr>.
 * Sponsored by TuneTracker Systems.
 * Based on the Haiku usb_serial driver which is:
 *
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PC_SERIAL_DRIVER_H_
#define _PC_SERIAL_DRIVER_H_

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <ISA.h>
#include <PCI.h>
#include <config_manager.h>
#include <string.h>

#include <lock.h>
#include <new>

#include "Tracing.h"

extern "C" {
#include <tty_module.h>
}


// whether we should handle default COM ports
#define HANDLE_ISA_COM

#define DRIVER_NAME		"pc_serial"		// driver name for debug output
#define DEVICES_COUNT	20				// max simultaneously open devices

// avoid clashing with BeOS zz driver
#define DEVFS_BASE		"ports/pc_serial"
//#define DEVFS_BASE		"ports/serial"


// no user serviceable part beyond this point

// more PCI serial APIs
#ifndef PCI_serial_16650
#define PCI_serial_16650        0x03                    /* 16650-compatible serial controller */
#define PCI_serial_16750        0x04                    /* 16750-compatible serial controller */
#define PCI_serial_16850        0x05                    /* 16850-compatible serial controller */
#define PCI_serial_16950        0x06                    /* 16950-compatible serial controller */
#endif

class SerialDevice;

struct port_constraints {
	uint32 minsize;
	uint32 maxsize;
	uint32 split;		// range to split I/O ports for each device
	uint8 ignoremask;	// bitmask of BARs to ignore when probing
	uint8 maxports;		// max number of ports on the card if > 0
	uint32 subsystem_id_mask;	// if set mask with subsys id and shift to get maxports
};

#define PCI_INVAL 0xffff

struct serial_support_descriptor {
	bus_type bus;	// B_*_BUS
	const char *name;
	const uint32 *bauds;
	// not yet used
	SerialDevice *(*instanciator)(struct serial_support_descriptor *desc);
	// I/O port constrains (which ranges to use, how to split them)
	struct port_constraints constraints;
	// bus specific stuff here...
	struct {
		// for both ISA & PCI
		uchar	class_base;
		uchar	class_sub;
		uchar	class_api;	// or PCI_undefined
		// for PCI: if PCI_INVAL then match class
		ushort	vendor_id;
		ushort	device_id;
		ushort	subsystem_vendor_id;
		ushort	subsystem_device_id;
	} match;
};
typedef struct serial_support_descriptor serial_support_descriptor;


struct serial_config_descriptor {
	bus_type bus;	// B_*_BUS
	struct serial_support_descriptor *descriptor;
	union {
		struct pci_info pci;
	} d;
};


/* This one rounds the size to integral count of segs (segments) */
#define ROUNDUP(size, seg) (((size) + (seg) - 1) & ~((seg) - 1))
/* Default device buffer size */
#define DEF_BUFFER_SIZE	0x200

// XXX: sort up the mess in termios.h on B* !
#define BLAST B230400

/* line coding defines ... Come from CDC USB specs? */
#define LC_STOP_BIT_1			0
#define LC_STOP_BIT_2			2

#define LC_PARITY_NONE			0
#define LC_PARITY_ODD			1
#define LC_PARITY_EVEN			2

/* struct that represents line coding */
typedef struct pc_serial_line_coding_s {
  uint32 speed;
  uint8 stopbits;
  uint8 parity;
  uint8 databits;
} pc_serial_line_coding;

/* control line states */
#define CLS_LINE_DTR			0x0001
#define CLS_LINE_RTS			0x0002


extern config_manager_for_driver_module_info *gConfigManagerModule;
extern isa_module_info *gISAModule;
extern pci_module_info *gPCIModule;
extern tty_module_info *gTTYModule;
extern struct ddomain gSerialDomain;

extern "C" {

status_t	init_hardware();
void		uninit_driver();

bool		pc_serial_service(struct tty *tty, uint32 op, void *buffer,
				size_t length);

int32		pc_serial_interrupt(void *arg);

status_t	pc_serial_open(const char *name, uint32 flags, void **cookie);
status_t	pc_serial_read(void *cookie, off_t position, void *buffer, size_t *numBytes);
status_t	pc_serial_write(void *cookie, off_t position, const void *buffer, size_t *numBytes);
status_t	pc_serial_control(void *cookie, uint32 op, void *arg, size_t length);
status_t	pc_serial_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
status_t	pc_serial_deselect(void *coookie, uint8 event, selectsync *sync);
status_t	pc_serial_close(void *cookie);
status_t	pc_serial_free(void *cookie);

const char **publish_devices();
device_hooks *find_device(const char *name);

}



#endif //_PC_SERIAL_DRIVER_H_
