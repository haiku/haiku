/*
 * Pegasus BeOS Driver
 * 
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef _DEV_PEGASUS_H_
#define _DEV_PEGASUS_H_

#include <Drivers.h>
#include <OS.h>
#include <SupportDefs.h>
#include <USB3.h>

#include "ether_driver.h"

#include "ByteOrder.h"

// compat
#define u_uint8_t 	uint8
#define u_uint16_t 	uint16
#define u_int8_t 	int8
#define u_int16_t	int16
#define DELAY 		snooze

#include "if_auereg.h"

#define VERSION "Version 0, Copyright (c) 2006 Jérôme Duval, compiled on " ## __DATE__ ## " " ## __TIME__ 
#define DRIVER_NAME "pegasus"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "

//#define DEBUG 1
#if DEBUG
	#define	DPRINTF_INFO(x...)	dprintf(MY_ID x)
	#define	DPRINTF_ERR(x...)	dprintf(MY_ID x)
#else
	#define DPRINTF_INFO(x...)  
	#define DPRINTF_ERR(x...)	dprintf(MY_ID x)
#endif

#define	ASSERT(x) \
	((x) ? 0 : dprintf (MY_ID "assertion failed at " __FILE__ ", line %d\n", __LINE__))

#define NUM_CARDS 3
#define DEVNAME 32

#define	DEFAULT_CONFIGURATION	0

#define ENET_HEADER_SIZE	14
#define ETHER_ADDRESS_LENGTH		6
#define MAX_FRAME_SIZE	1536
#define ETHER_TRANSMIT_TIMEOUT ((bigtime_t)5000000)
	/* five seconds */

/*
 * Various supported device vendors/products.
 */
struct aue_type {
	struct usb_devno {
		uint16 vendor;
		uint16 product;
	} aue_dev;
	uint16		aue_flags;
#define LSYS	0x0001		/* use Linksys reset */
#define PNA	0x0002		/* has Home PNA */
#define PII	0x0004		/* Pegasus II chip */
};


/*
 * Devices
 */

typedef struct _pegasus_dev {
	/* list structure */
	struct _pegasus_dev *next;
	
	uint32			cookieMagic;
	
	/* maintain device */
	sem_id 		sem_lock;

	char		name[DEVNAME];	/* used for resources */
	int 		type;
	usb_device 	dev;
	uint16 		aue_vendor;
	uint16 		aue_product;
	
	uint16 		ifno;
	
	int open;
	struct driver_cookie *open_fds;
	volatile bool aue_dying;
	int number;
	uint flags;

	int			aue_unit;
	uint16		aue_flags;
	
	volatile bool		nonblocking;

	uint32				maxframesize;	// 14 bytes header + MTU
	uint8				macaddr[6];
	
	usb_pipe pipe_in;			/**/
	usb_pipe pipe_out;			/**/
	usb_pipe pipe_intr;			/**/
	
	uint16			phy;
	bool			link;
		
	// receive data
	
	sem_id			rx_sem;
	sem_id			rx_sem_cb;
	char			rx_buffer[MAX_FRAME_SIZE];
	status_t		rx_status;
	size_t			rx_actual_length;
	
	// transmit data
	
	sem_id			tx_sem;
	sem_id			tx_sem_cb;
	char			tx_buffer[MAX_FRAME_SIZE];
	status_t		tx_status;
	size_t			tx_actual_length;
	
} pegasus_dev;

#define PEGASUS_COOKIE_MAGIC 'pega'


/* driver.c */

extern usb_module_info *usb;

pegasus_dev *create_device(const usb_device dev, const usb_interface_info *ii, uint16 ifno);
void remove_device(pegasus_dev *device);

/* devlist.c */

extern sem_id gDeviceListLock;
extern bool gDeviceListChanged;

void add_device_info(pegasus_dev *device);
void remove_device_info(pegasus_dev *device);
pegasus_dev *search_device_info(const char *name);

extern char **gDeviceNames;

void alloc_device_names(void);
void free_device_names(void);
void rebuild_device_names(void);

/* if_aue.c */
void aue_attach(pegasus_dev *sc);
void aue_init(pegasus_dev *sc);
void aue_uninit(pegasus_dev *sc);

#endif /* _DEV_PEGASUS_H_ */
