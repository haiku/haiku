/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>
#include <stdarg.h>

#include <OS.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/heap.h>
#include <boot/platform/cfe/cfe.h>
#include <platform_arch.h>

typedef uint64	ptr64; // for clarity

status_t cfe_error(int32 err)
{
	// not an error
	if (err > 0)
		return err;

	switch (err) {
		case CFE_OK:
			return B_OK;
		case CFE_ERR:
			return B_ERROR;
		//TODO:add cases
		default:
			return B_ERROR;
	}
}

#define CFE_CMD_FW_GETINFO		0
#define CFE_CMD_FW_RESTART		1
#define CFE_CMD_FW_BOOT			2
#define CFE_CMD_FW_CPUCTL		3
#define CFE_CMD_FW_GETTIME		4
#define CFE_CMD_FW_MEMENUM		5
#define CFE_CMD_FW_FLUSHCACHE	6

#define CFE_CMD_DEV_GETHANDLE	9
#define CFE_CMD_DEV_ENUM		10
#define CFE_CMD_DEV_OPEN		11
#define CFE_CMD_DEV_READ		13
#define CFE_CMD_DEV_WRITE		14
#define CFE_CMD_DEV_CLOSE		16


struct cfe_xiocb_s {
	cfe_xiocb_s(uint64 fcode, int64 handle = 0, uint64 flags = 0);

	uint64	xiocb_fcode;
	int64	xiocb_status;
	int64	xiocb_handle;
	uint64	xiocb_flags;
	uint64	xiocb_psize;
	union {
		struct {
			uint64	buf_offset;
			ptr64	buf_ptr;
			uint64	buf_length;
			uint64	buf_retlen;
			uint64	buf_ioctlcmd;
		} xiocb_buffer;
/*
		struct {
		} xiocb_inpstat;
*/
		struct {
			int64	enum_idx;
			ptr64	name_ptr;
			int64	name_length;
			ptr64	val_ptr;
			int64	val_length;
		} xiocb_envbuf;
/*
		struct {
		} xiocb_cpuctl;
*/
		struct {
			int64	ticks;
		} xiocb_time;
/*
		struct {
		} xiocb_meminfo;
		struct {
		} xiocb_fwinfo;
*/
		struct {
			int64	status;
		} xiocb_exitstat;
	} plist;
};

typedef struct cfe_xiocb_s cfe_xiocb_t;

cfe_xiocb_s::cfe_xiocb_s(uint64 fcode, int64 handle, uint64 flags)
	: xiocb_fcode(fcode),
	xiocb_status(0),
	xiocb_handle(handle),
	xiocb_flags(flags),
	xiocb_psize(0)
{
	switch (fcode) {
		case CFE_CMD_FW_GETINFO:
		case CFE_CMD_DEV_READ:
		case CFE_CMD_DEV_WRITE:
		case CFE_CMD_DEV_OPEN:
			xiocb_psize = sizeof(plist.xiocb_buffer);
			break;
		case CFE_CMD_FW_RESTART:
			xiocb_psize = sizeof(plist.xiocb_exitstat);
			break;
		case CFE_CMD_FW_GETTIME:
			xiocb_psize = sizeof(plist.xiocb_time);
			break;
		case CFE_CMD_DEV_ENUM:
			xiocb_psize = sizeof(plist.xiocb_envbuf);
			break;
		//XXX: some more...
		default:
			break;
	}
	memset(&plist, 0, sizeof(plist));
};


// CFE handle
static uint64 sCFEHandle;
// CFE entry point
static uint64 sCFEEntry;

static int cfe_iocb_dispatch(cfe_xiocb_t *xiocb)
{
	static int (*dispfunc)(intptr_t handle, intptr_t xiocb);
	dispfunc = (int(*)(intptr_t, intptr_t))(void *)sCFEEntry;
	if (dispfunc == NULL)
		return CFE_ERR;
	return (*dispfunc)((intptr_t)sCFEHandle, (intptr_t)xiocb);
}

int
cfe_init(uint64 handle, uint64 entry)
{
	sCFEHandle = handle;
	sCFEEntry = entry;

	return CFE_OK;
}

int
cfe_exit(int32 warm, int32 status)
{
	cfe_xiocb_t xiocb(CFE_CMD_FW_RESTART, 0,
		warm ? CFE_FLG_WARMSTART : CFE_FLG_COLDSTART);
	xiocb.plist.xiocb_exitstat.status = status;

	cfe_iocb_dispatch(&xiocb);

	return xiocb.xiocb_status;
}


int cfe_enumdev(int idx, char *name, int namelen)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_ENUM);
	xiocb.plist.xiocb_envbuf.enum_idx = idx;
	xiocb.plist.xiocb_envbuf.name_ptr = (uint64)name;
	xiocb.plist.xiocb_envbuf.name_length = namelen;

	cfe_iocb_dispatch(&xiocb);

	return xiocb.xiocb_status;
}


int
cfe_getstdhandle(int flag)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_GETHANDLE, 0, flag);

	cfe_iocb_dispatch(&xiocb);

	if (xiocb.xiocb_status < 0);
		return xiocb.xiocb_status;
	return xiocb.xiocb_handle;
}


int
cfe_open(const char *name)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_OPEN);
	xiocb.plist.xiocb_buffer.buf_offset = 0;
	xiocb.plist.xiocb_buffer.buf_ptr = (uint64)name;
	xiocb.plist.xiocb_buffer.buf_length = strlen(name);

	cfe_iocb_dispatch(&xiocb);

	if (xiocb.xiocb_status < 0);
		return xiocb.xiocb_status;
	return xiocb.xiocb_handle;
}


int
cfe_close(int handle)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_CLOSE, handle);

	cfe_iocb_dispatch(&xiocb);

	return xiocb.xiocb_status;
}


uint64
cfe_getticks(void)
{
	cfe_xiocb_t xiocb(CFE_CMD_FW_GETTIME);

	cfe_iocb_dispatch(&xiocb);

	return xiocb.plist.xiocb_time.ticks;
}


int
cfe_readblk(int handle, int64 offset, void *buffer, int length)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_READ, handle);
	xiocb.plist.xiocb_buffer.buf_offset = offset;
	xiocb.plist.xiocb_buffer.buf_ptr = (uint64)buffer;
	xiocb.plist.xiocb_buffer.buf_length = length;

	cfe_iocb_dispatch(&xiocb);

	if (xiocb.xiocb_status < 0);
		return xiocb.xiocb_status;
	return xiocb.plist.xiocb_buffer.buf_retlen;
}



int
cfe_writeblk(int handle, int64 offset, const void *buffer, int length)
{
	cfe_xiocb_t xiocb(CFE_CMD_DEV_WRITE, handle);
	xiocb.plist.xiocb_buffer.buf_offset = offset;
	xiocb.plist.xiocb_buffer.buf_ptr = (uint64)buffer;
	xiocb.plist.xiocb_buffer.buf_length = length;

	cfe_iocb_dispatch(&xiocb);

	if (xiocb.xiocb_status < 0);
		return xiocb.xiocb_status;
	return xiocb.plist.xiocb_buffer.buf_retlen;
}


