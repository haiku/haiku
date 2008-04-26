/*
 * Copyright 2008, François Revol, <revol@free.fr>.
 * Copyright 2007-2008, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <util/AutoLock.h>
#include <util/kernel_cpp.h>

#include <team.h>

#include <tty.h>

#include <KernelExport.h>
#include <device_manager.h>
#include <bus_manager.h>
#include <tty/ttylayer.h>

#include "tty_private.h"

//#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif




status_t
tty_open(struct ttyfile *, struct ddrover *, tty_service_func)
{
	return B_OK;
}


status_t
tty_close(struct ttyfile *, struct ddrover *)
{
	return B_OK;
}


status_t
tty_free(struct ttyfile *, struct ddrover *)
{
	return B_OK;
}


status_t
tty_read(struct ttyfile *, struct ddrover *, char *, size_t *)
{
	return B_OK;
}


status_t
tty_write(struct ttyfile *, struct ddrover *, const char *, size_t *)
{
	return B_OK;
}


status_t
tty_control(struct ttyfile *, struct ddrover *, ulong, void *, size_t)
{
	return B_OK;
}


status_t
tty_select(struct ttyfile *, struct ddrover *, uint8, uint32, selectsync *)
{
	return B_OK;
}


status_t
tty_deselect(struct ttyfile *, struct ddrover *, uint8, selectsync *)
{
	return B_OK;
}


void
tty_init(struct tty *, bool)
{
}


void
tty_ilock(struct tty *, struct ddrover *, bool )
{
}


void
tty_hwsignal(struct tty *, struct ddrover *, int, bool)
{
}


int
tty_in(struct tty *, struct ddrover *, int)
{
	return 0;
}


int
tty_out(struct tty *, struct ddrover *)
{
	return 0;
}


// #pragma mark ddrover handling


struct ddrover *
tty_dd_rstart(struct ddrover *)
{
	return NULL;
}


void
tty_dd_rdone(struct ddrover *)
{
}

void
tty_dd_acquire(struct ddrover *, struct ddomain *)
{
}


// #pragma mark bus manager exports


status_t
tty_module_init(void)
{
	return B_OK;
}


void
tty_module_uninit(void)
{
}


static int32
tty_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status;
		
			//TRACE(("TTY: tty_module_init\n"));
		
			status = tty_module_init();
			if (status < B_OK)
				return status;
		
			return B_OK;
		}

		case B_MODULE_UNINIT:
			//TRACE(("TTY: tty_module_uninit\n"));
			tty_module_uninit();
			return B_OK;
	}

	return B_BAD_VALUE;	
}


static struct tty_module_info_r5 sR5TTYModule = {
	// ! this is *NOT* a real bus manager (no rescan call!)
	//{
		{
			B_TTY_MODULE_NAME,
			0, //B_KEEP_LOADED,
			tty_module_std_ops
		},
		//NULL
	//},
	&tty_open,
	&tty_close,
	&tty_free,
	&tty_read,
	&tty_write,
	&tty_control,
	&tty_init,
	&tty_ilock,
	&tty_hwsignal,
	&tty_in,
	&tty_out,
	&tty_dd_rstart,
	&tty_dd_rdone,
	&tty_dd_acquire
};

static struct tty_module_info_dano sDanoTTYModule = {
	// ! this is *NOT* a real bus manager (no rescan call!)
	//{
		{
			B_TTY_MODULE_NAME_DANO,
			0, //B_KEEP_LOADED,
			tty_module_std_ops
		},
		//NULL
	//},
	&tty_open,
	&tty_close,
	&tty_free,
	&tty_read,
	&tty_write,
	&tty_control,
	&tty_select,
	&tty_deselect,
	&tty_init,
	&tty_ilock,
	&tty_hwsignal,
	&tty_in,
	&tty_out,
	&tty_dd_rstart,
	&tty_dd_rdone,
	&tty_dd_acquire
};

module_info *modules[] = {
	(module_info *)&sR5TTYModule,
	(module_info *)&sDanoTTYModule,
	NULL
};
