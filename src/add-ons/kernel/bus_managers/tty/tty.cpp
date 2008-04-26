/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include <KernelExport.h>
#include <device_manager.h>
#include <bus_manager.h>
#include <tty/ttylayer.h>

#include "tty.h"

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
tty_ddr_start(struct ddrover *)
{
	return NULL;
}


void
tty_ddr_done(struct ddrover *)
{
}

void
tty_ddr_acquire(struct ddrover *, struct ddomain *)
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


static struct tty_module_info sTTYModule = {
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
#if 0 /* Dano! */
	&tty_select,
	&tty_deselect,
#endif
	&tty_init,
	&tty_ilock,
	&tty_hwsignal,
	&tty_in,
	&tty_out,
	&tty_ddr_start,
	&tty_ddr_done,
	&tty_ddr_acquire
};

module_info *modules[] = {
	(module_info *)&sTTYModule,
	NULL
};
