/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

#include "toscalls.h"


void *gXHDIEntryPoint = NULL;
uint32 gXHDIVersion = 0;

NatFeatCookie *gNatFeatCookie = NULL;
uint32 gDebugPrintfNatFeatID = 0;
uint32 gBootstrapNatFeatID = 0;


/*! Maps TOS error codes to native errors
 */
extern "C" status_t
toserror(int32 err)
{
	// generated from:
	// http://www.fortunecity.com/skyscraper/apple/308/html/appendd.htm
	// with:
	// while read N; do read V; read L; echo -e "\tcase $V: /* $N - $L */\n\t\treturn B_BAD_VALUE;"; done >> errs
	switch (err) {
	/* BIOS errors */
	case 0: /* E_OK - No error */
		return B_OK;
	case -1: /* ERROR - Generic error */
		return B_ERROR;
	case -2: /* EDRVNR - Drive not ready */
		return B_DEV_NOT_READY;
	case -3: /* EUNCMD - Unknown command */
		return B_BAD_VALUE;	//XXX
	case -4: /* E_CRC - CRC error */
		return B_DEV_CRC_ERROR;
	case -5: /* EBADRQ - Bad request */
		return B_BAD_VALUE;	//XXX
	case -6: /* E_SEEK - Seek error */
		return B_DEV_SEEK_ERROR;
	case -7: /* EMEDIA - Unknown media */
		return B_DEV_UNREADABLE;
	case -8: /* ESECNF - Sector not found */
		return B_DEV_FORMAT_ERROR;
	case -9: /* EPAPER - Out of paper */
		return B_DEVICE_NOT_FOUND;
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
		return B_BAD_VALUE;
	case -33: /* EFILNF - File not found */
		return B_FILE_NOT_FOUND;
	case -34: /* EPTHNF - Path not found */
		return B_ENTRY_NOT_FOUND;
	case -35: /* ENHNDL - No more handles */
		return B_NO_MORE_FDS;
	case -36: /* EACCDN - Access denied */
		return B_PERMISSION_DENIED;
	case -37: /* EIHNDL - Invalid handle */
		return B_FILE_ERROR;
	case -39: /* ENSMEM - Insufficient memory */
		return B_NO_MEMORY;
	case -40: /* EIMBA - Invalid memory block address */
		return B_BAD_ADDRESS;
	case -46: /* EDRIVE - Invalid drive specification */
		return B_DEV_BAD_DRIVE_NUM;
	case -48: /* ENSAME - Cross device rename */
		return B_CROSS_DEVICE_LINK;
	case -49: /* ENMFIL - No more files */
		return B_NO_MORE_FDS;
	case -58: /* ELOCKED - Record is already locked */
		return B_BAD_VALUE;	//XXX
	case -59: /* ENSLOCK - Invalid lock removal request */
		return B_BAD_VALUE;	//XXX
	case -64: /* ERANGE or ENAMETOOLONG - Range error */
		return B_NAME_TOO_LONG;
	case -65: /* EINTRN - Internal error */
		return B_ERROR;
	case -66: /* EPLFMT - Invalid program load format */
		return B_NOT_AN_EXECUTABLE;
	case -67: /* EGSBF - Memory block growth failure */
		return B_BAD_VALUE;
	case -80: /* ELOOP - Too many symbolic links */
		return B_LINK_LIMIT;
	case -200: /* EMOUNT - Mount point crossed (indicator) */
		return B_BAD_VALUE;	
	default:
		return B_ERROR;
	}
}


/*! Maps XHDI error codes to native errors
 * cf. http://toshyp.atari.org/010008.htm#XHDI_20error_20codes
 */
extern "C" status_t
xhdierror(int32 err)
{
	if (err <= -456) {
		int ide = -(err + 456);
		// ide status reg
		// guessed mapping
		if (ide & (1 << 1)) {	// track 0 not found
			return B_DEV_FORMAT_ERROR;
		} else if (ide & (1 << 0)) {	// DAM not found
			return B_DEV_FORMAT_ERROR;
		} else if (ide & (1 << 4)) {	// ID field not found
			return B_DEV_ID_ERROR;
		} else if (ide & (1 << 7)) {	// bad block mark
			return B_DEV_FORMAT_ERROR;
		} else if (ide & (1 << 6)) {	// uncorrectable error
			return B_DEV_UNREADABLE;
		} else if (ide & (1 << 2)) {	// command aborted
			return B_INTERRUPTED;
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
			return B_INTERRUPTED;
		case 0x28:
			return B_DEV_FORMAT_ERROR;
		case 0x5a:
			return B_DEV_FORMAT_ERROR;
		}
	}
	return toserror(err);
}


static void
dump_tos_cookie(const struct tos_cookie *c)
{
	if (c != NULL) {
		dprintf("%4.4s: 0x%08lx, %ld\n", (const char *)&c->cookie, c->ivalue,
			c->ivalue);
	}
}


extern "C" void
dump_tos_cookies(void)
{
	const tos_cookie *c = COOKIE_JAR;
	dprintf("Cookies:\n");
	while (c && (c->cookie)) {
		dump_tos_cookie(c++);
	}
}


extern "C" status_t
init_xhdi(void)
{
	const struct tos_cookie *c;

	if (gXHDIEntryPoint)
		return B_OK;
	
	c = tos_find_cookie(XHDI_COOKIE);
	if (!c)
		return B_ENTRY_NOT_FOUND;
	if (((uint32 *)c->pvalue)[-1] != XHDI_MAGIC)
		return B_BAD_VALUE;
	gXHDIEntryPoint = c->pvalue;
	gXHDIVersion = XHGetVersion();
	return B_OK;
}


extern "C" status_t
init_nat_features(void)
{
	gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_get_id = NULL;
	gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_call = NULL;
	gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_dprintf_id = 0;
	if (nat_features()) {
		// find debugprintf id
		gDebugPrintfNatFeatID = nat_feat_getid("DEBUGPRINTF");
		dprintf("DEBUGPRINTF natfeat id 0x%08lx\n", gDebugPrintfNatFeatID);
		// pass native features infos to the kernel
		gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_get_id =
			nat_features()->nfGetID;
		gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_call = 
			nat_features()->nfCall;
		gKernelArgs.arch_args.plat_args.atari.nat_feat.nf_dprintf_id =
			gDebugPrintfNatFeatID;
		// find other natfeat ids
		gBootstrapNatFeatID = nat_feat_getid("BOOTSTRAP");
		dprintf("BOOTSTRAP natfeat id 0x%08lx\n", gBootstrapNatFeatID);
	}
	return nat_features() ? B_OK : B_ENTRY_NOT_FOUND;
}


extern "C" void
nat_feat_debugprintf(const char *str)
{
	if (gDebugPrintfNatFeatID)
		nat_feat_call(gDebugPrintfNatFeatID, 0, str);
}

extern "C" int
nat_feat_get_bootdrive(void)
{
	if (gBootstrapNatFeatID == 0)
		return -1;
	return nat_feat_call(gBootstrapNatFeatID, 1);
}

extern "C" status_t
nat_feat_get_bootargs(char *str, long size)
{
	status_t err;
	if (gBootstrapNatFeatID == 0)
		return B_ERROR;
	return toserror(nat_feat_call(gBootstrapNatFeatID, 2, str, size));
}
