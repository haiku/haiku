/*******************************************************************************
/
/	File:			mime_table.c
/
/	Description:	Kernel module implementing kernel-space mime_table API
/
/	Copyright 2004, François Revol.
/
*******************************************************************************/

#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include <mime_table.h>
extern struct ext_mime mimes[];

#if DEBUG > 0
#define ddprintf(x) dprintf x
#else
#define ddprintf(x)
#endif

/* Module static data */
const char mime_table_module_name[] = B_MIME_TABLE_MODULE_NAME;

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return B_OK;
	case B_MODULE_UNINIT:
		return B_OK;
	default:
		/* do nothing */
		;
	}
	return -1;
}

status_t get_table(struct ext_mime **table)
{
	if (!table)
		return EINVAL;
	/* no need to malloc & copy yet */
	*table = mimes;
	return B_OK;
}

void free_table(struct ext_mime *table)
{
	/* do nothing yet */
}

const char *mime_for_ext(const char *ext)
{
	int i;
	/* should probably be optimized */
	for (i = 0; mimes[i].extension; i++) {
		if (!strcmp(ext, mimes[i].extension))
			return mimes[i].mime;
	}
	return NULL;
}

const char *ext_for_mime(const char *mime)
{
	int i;
	/* should probably be optimized */
	for (i = 0; mimes[i].mime; i++) {
		if (!strcmp(mime, mimes[i].mime))
			return mimes[i].extension;
	}
	return NULL;
}

static mime_table_module_info mime_table = {
	{
		mime_table_module_name,
		0,
		std_ops
	},
	get_table,
	free_table,
	mime_for_ext,
	ext_for_mime
};

_EXPORT mime_table_module_info *modules[] = {
        &mime_table,
        NULL
};

