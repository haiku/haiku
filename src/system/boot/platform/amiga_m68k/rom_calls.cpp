/*
 * Copyright 2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

#include "rom_calls.h"


struct GfxBase *GRAPHICS_BASE_NAME = NULL;
struct Library *KEYMAP_BASE_NAME = NULL;
struct Library *LOWLEVEL_BASE_NAME = NULL;


/*! Maps Amiga error codes to native errors
 */
extern "C" status_t
exec_error(int32 err)
{
	switch (err) {
		case 0:
			return B_OK;
		case IOERR_OPENFAIL:
			return B_DEV_BAD_DRIVE_NUM;
		case IOERR_ABORTED:
			return B_INTERRUPTED;
		case IOERR_NOCMD:
			return B_BAD_VALUE;
		case IOERR_BADLENGTH:
			return B_BAD_VALUE;
		case IOERR_BADADDRESS:
			return B_BAD_ADDRESS;
		case IOERR_UNITBUSY:
			return B_DEV_NOT_READY;
		case IOERR_SELFTEST:
			return B_NOT_INITIALIZED;
		default:
			return B_ERROR;
	}
}


