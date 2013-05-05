/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_CFE_CFE_H
#define KERNEL_BOOT_PLATFORM_CFE_CFE_H


#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CFE_EPTSEAL	0x43464531	/* 'CFE1' */
#define CFE_MAGIC	CFE_EPTSEAL

// cfe/include/cfe_timer.h
#define CFE_HZ		10

/* CFE sources declare this separately in cfe_api.h */

/* (let's hope it's always built-in,
   unlike u-boot's API which never is... */

#define CFE_FLG_COLDSTART		0x00000000
#define CFE_FLG_WARMSTART		0x00000001

#define CFE_STDHANDLE_CONSOLE	0

int cfe_init(uint64 handle, uint64 entry);

int cfe_exit(int32 warm, int32 status);
uint64 cfe_getticks(void);

int cfe_enumdev(int idx, char *name, int namelen);

int cfe_getstdhandle(int flag);
int cfe_open(const char *name);
int cfe_close(int handle);

int cfe_readblk(int handle, int64 offset, void *buffer, int length);
int cfe_writeblk(int handle, int64 offset, const void *buffer, int length);

#define CFE_OK			0
#define CFE_ERR			-1

status_t cfe_error(int32 err);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_PLATFORM_CFE_CFE_H */
