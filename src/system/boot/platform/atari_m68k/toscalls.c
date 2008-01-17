/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "toscalls.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

void *gXHDIEntryPoint = NULL;
NatFeatCookie *gNatFeatCookie = NULL;
int32 gDebugPrintfNatFeatID = 0;

/*! Maps TOS error codes to native errors
 */
status_t
toserror(int32 err)
{
	// generated from:
	// http://www.fortunecity.com/skyscraper/apple/308/html/appendd.htm
	// with:
	// while read N; do read V; read L; echo -e "\tcase $V: /* $N - $L */\n\t\treturn EINVAL;"; done >> errs
	switch (err) {
	/* BIOS errors */
	case 0: /* E_OK - No error */
		return B_OK;
	case -1: /* ERROR - Generic error */
		return B_ERROR;
	case -2: /* EDRVNR - Drive not ready */
		return B_DEV_NOT_READY;
	case -3: /* EUNCMD - Unknown command */
		return EINVAL;	//XXX
	case -4: /* E_CRC - CRC error */
		return B_DEV_CRC_ERROR;
	case -5: /* EBADRQ - Bad request */
		return EINVAL;	//XXX
	case -6: /* E_SEEK - Seek error */
		return B_DEV_SEEK_ERROR;
	case -7: /* EMEDIA - Unknown media */
		return B_DEV_UNREADABLE;
	case -8: /* ESECNF - Sector not found */
		return B_DEV_FORMAT_ERROR;
	case -9: /* EPAPER - Out of paper */
		return ENODEV;
	case -10: /* EWRITF - Write fault */
		return B_DEV_WRITE_ERROR;
	case -11: /* EREADF - Read fault */
		return B_DEV_READ_ERROR;
	case -12: /* EWRPRO - Device is write protected */
		return B_READ_ONLY_DEVICE;
	case -14: /* E_CHNG - Media change detected */
		return B_DEV_MEDIA_CHANGED;
	case -15: /* EUNDEV - Unknown device */
		return B_DEV_BAD_DRIVE_NUM;
	case -16: /* EBADSF - Bad sectors on format */
		return B_DEV_UNREADABLE;
	case -17: /* EOTHER - Insert other disk (request) */
		return B_DEV_MEDIA_CHANGE_REQUESTED;
	/* GEMDOS errors */
	case -32: /* EINVFN - Invalid function */
		return EINVAL;
	case -33: /* EFILNF - File not found */
		return B_FILE_NOT_FOUND;
	case -34: /* EPTHNF - Path not found */
		return B_ENTRY_NOT_FOUND;
	case -35: /* ENHNDL - No more handles */
		return B_NO_MORE_FDS;
	case -36: /* EACCDN - Access denied */
		return EACCES;
	case -37: /* EIHNDL - Invalid handle */
		return EBADF;
	case -39: /* ENSMEM - Insufficient memory */
		return ENOMEM;
	case -40: /* EIMBA - Invalid memory block address */
		return EFAULT;
	case -46: /* EDRIVE - Invalid drive specification */
		return B_DEV_BAD_DRIVE_NUM;
	case -48: /* ENSAME - Cross device rename */
		return EXDEV;
	case -49: /* ENMFIL - No more files */
		return EMFILE;
	case -58: /* ELOCKED - Record is already locked */
		return EINVAL;	//XXX
	case -59: /* ENSLOCK - Invalid lock removal request */
		return EINVAL;	//XXX
	case -64: /* ERANGE or ENAMETOOLONG - Range error */
		return ENAMETOOLONG;
	case -65: /* EINTRN - Internal error */
		return B_ERROR;
	case -66: /* EPLFMT - Invalid program load format */
		return ENOEXEC;
	case -67: /* EGSBF - Memory block growth failure */
		return EINVAL;
	case -80: /* ELOOP - Too many symbolic links */
		return ELOOP;
	case -200: /* EMOUNT - Mount point crossed (indicator) */
		return EINVAL;	
	default:
		return B_ERROR;
	}
}


/*! Maps XHDI error codes to native errors
 * cf. http://toshyp.atari.org/010008.htm#XHDI_20error_20codes
 */
status_t
xhdierror(int32 err)
{
	if (err <= -456) {
		int ide = -(err + 456);
		// ide status reg
		// guessed mapping
		if (ide & (1 << 1)) {	// track 0 not found
			return B_DEV_FORMAT_ERROR;
		} else if (ide & (1 << 0)) {	// DAM not found
			return B_DEV_FORMAT_ERROR,;
		} else if (ide & (1 << 4)) {	// ID field not found
			return B_DEV_ID_ERROR;
		} else if (ide & (1 << 7)) {	// bad block mark
			return B_DEV_FORMAT_ERROR;
		} else if (ide & (1 << 6)) {	// uncorrectable error
			return B_DEV_UNREADABLE;
		} else if (ide & (1 << 2)) {	// command aborted
			return EINTR;
		} else if (ide & (1 << 5)) {	// media change
			return B_DEV_MEDIA_CHANGED;
		} else if (ide & (1 << 3)) {	// media change requested
			return B_DEV_MEDIA_CHANGE_REQUESTED;
		}
		return B_ERROR;
	} else if (err <= -200) {
		/* SCSI errors */
		int scsi = -(err + 200);
		//XXX:
		switch (scsi) {
		case 0x06:
			return B_DEV_FORMAT_ERROR;
		case 0x10:
			return B_DEV_FORMAT_ERROR;
		case 0x11:
			return B_DEV_UNREADABLE;
		case 0x12:
			return B_DEV_ID_ERROR;
		case 0x13:
			return B_DEV_FORMAT_ERROR;
		case 0x20:
			return EINTR;
		case 0x28:
			return B_DEV_FORMAT_ERROR;
		case 0x5a:
			return B_DEV_FORMAT_ERROR;
		}
	}
	return toserror(err);
}

void
dump_tos_cookie(const struct tos_cookie *c)
{
	if (c)
		dprintf("%4.4s: 0x%08lx, %d\n", &c->cookie, c->ivalue, c->ivalue);
}


void
dump_tos_cookies(void)
{
	const tos_cookie *c = COOKIE_JAR;
	dprintf("Cookies:\n");
	while (c && (c->cookie)) {
		dump_tos_cookie(c++);
	}
}


status_t
init_xhdi(void)
{
	
}


status_t
init_nat_features(void)
{
	if (nat_features()) {
		// find debugprintf id
		gDebugPrintfNatFeatID = nat_feat_getid("DEBUGPRINTF");
		//
	}
	return nat_features() ? B_OK : ENOENT;
}

void
nat_feat_debugprintf(const char *str)
{
	if (gDebugPrintfNatFeatID)
		nat_feat_call(gDebugPrintfNatFeatID, 0, str);
}
