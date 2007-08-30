/* Kernel driver for firewire 
 *
 * Copyright (C) 2007 JiSheng Zhang <jszhang3@gmail.com>. All rights reserved 
 * Distributed under the terms of the MIT license.
 */
#ifndef _FW_MODULE_H
#define _FW_MODULE_H
#include <KernelExport.h>
#include <module.h>
#include <bus_manager.h>

#include "firewire.h"


#define	FIREWIRE_MODULE_NAME		"bus_managers/firewire/v1"

struct fw_module_info{

	bus_manager_info	binfo;

struct fw_device * (*fw_noderesolve_nodeid)(struct firewire_comm *fc, int dst);

struct fw_device * (*fw_noderesolve_eui64)(struct firewire_comm *fc, struct fw_eui64 *eui);

int (*fw_asyreq)(struct firewire_comm *fc, int sub, struct fw_xfer *xfer);

void (*fw_xferwake)(struct fw_xfer *xfer);
int (*fw_xferwait)(struct fw_xfer *xfer);

struct fw_bind * (*fw_bindlookup)(struct firewire_comm *fc, uint16_t dest_hi, uint32_t dest_lo);

int (*fw_bindadd)(struct firewire_comm *fc, struct fw_bind *fwb);

int (*fw_bindremove)(struct firewire_comm *fc, struct fw_bind *fwb);

int (*fw_xferlist_add)(struct fw_xferlist *q, 
		int slen, int rlen, int n,
		struct firewire_comm *fc, void *sc, void (*hand)(struct fw_xfer *));

void (*fw_xferlist_remove)(struct fw_xferlist *q);

struct fw_xfer * (*fw_xfer_alloc)();

struct fw_xfer * (*fw_xfer_alloc_buf)(int send_len, int recv_len);

void (*fw_xfer_done)(struct fw_xfer *xfer);

void (*fw_xfer_unload)(struct fw_xfer* xfer);

void (*fw_xfer_free_buf)( struct fw_xfer* xfer);

void (*fw_xfer_free)( struct fw_xfer* xfer);

void (*fw_asy_callback_free)(struct fw_xfer *xfer);

int (*fw_open_isodma)(struct firewire_comm *fc, int tx);

int (*get_handle)(int socket, struct firewire_softc **handle);

struct fwdma_alloc_multi * (*fwdma_malloc_multiseg)(int alignment,
		int esize, int n);

void (*fwdma_free_multiseg)(struct fwdma_alloc_multi *);
};
#endif
