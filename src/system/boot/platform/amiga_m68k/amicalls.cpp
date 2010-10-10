/*
 * Copyright 2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

#include "amicalls.h"



/*! Maps Amiga error codes to native errors
 */
extern "C" status_t
exec_error(int32 err)
{
	switch (err) {
		case IOERR_OPENFAIL:
			return B_DEV_BAD_DRIVE_NUM;
		case IOERR_ABORTED:
			return EINTR;
		case IOERR_NOCMD:
			return EINVAL;
		case IOERR_BADLENGTH:
			return EINVAL;
		case IOERR_BADADDRESS:
			return EFAULT;
		case IOERR_UNITBUSY:
			return B_DEV_NOT_READY;
		case IOERR_SELFTEST:
			return B_ERROR;
		default:
			return B_ERROR;
	}
}


