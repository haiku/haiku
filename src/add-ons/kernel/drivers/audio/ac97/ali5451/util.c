/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * Original code for memory management is
 * Copyright (c) 2003, Jerome Duval (jerome.duval@free.fr).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <hmulti_audio.h>
#include <string.h>

#include "ac97.h"
#include "queue.h"
#include "driver.h"
#include "util.h"
#include "debug.h"


/* memory management */


static uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


static area_id
alloc_mem(void **phy, void **log, size_t size, const char *name)
{
// TODO: phy should be phys_addr_t*!
	physical_entry pe;
	void *logadr;
	area_id areaid;
	status_t rv;

	size = round_to_pagesize(size);
	areaid = create_area(name, &logadr, B_ANY_KERNEL_ADDRESS, size,
		B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (areaid < B_OK) {
		TRACE("alloc_mem: couldn't allocate area %s\n", name);
		return B_ERROR;
	}
	rv = get_memory_map(logadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		TRACE("alloc_mem: couldn't map %s\n",name);
		return B_ERROR;
	}
	memset(logadr, 0, size);
	if (log)
		*log = logadr;
	if (phy)
		*phy = (void*)(addr_t)pe.address;

	return areaid;
}


static ali_mem
*ali_mem_new(ali_dev *card, size_t size)
{
	ali_mem *mem;

	if (!(mem = malloc(sizeof(*mem))))
		return NULL;

	mem->area = alloc_mem(&mem->phy_base, &mem->log_base, size, "ali buffer");
	mem->size = size;

	if (mem->area < B_OK) {
		free(mem);
		return NULL;
	}

	return mem;
}


static void
ali_mem_delete(ali_mem *mem)
{
	if (B_OK < mem->area)
		delete_area(mem->area);

	free(mem);
}


void *
ali_mem_alloc(ali_dev *card, size_t size)
{
	ali_mem *mem;

	mem = ali_mem_new(card, size);
	if (!mem)
		return NULL;

	LIST_INSERT_HEAD(&(card->mems), mem, next);

	return mem;
}


void
ali_mem_free(ali_dev *card, void *ptr)
{
	ali_mem *mem;

	LIST_FOREACH(mem, &card->mems, next) {
		if (mem->log_base != ptr)
			continue;

		LIST_REMOVE(mem, next);

		ali_mem_delete(mem);
		break;
	}
}


/* misc utils */


uint8
util_format_to_sample_size(uint32 format)
{
	switch (format) {
		case B_FMT_8BIT_U:
		case B_FMT_8BIT_S: return 1;

		case B_FMT_16BIT: return 2;

		case B_FMT_18BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
		case B_FMT_FLOAT: return 4;

		default: return 0;
	}
}


bool
util_format_is_signed(uint32 format)
{
	return (format == B_FMT_8BIT_S || format == B_FMT_16BIT);
}


uint32
util_get_buffer_length_for_rate(uint32 rate)
{
	// values are based on Haiku's hda driver

	switch (rate) {
		case 8000:
		case 11025: return 512;

		case 16000:
		case 22050: return 1024;

		case 32000:
		case 44100:
		case 48000: return 2048;
	}

	return 2048;
}


uint32
util_sample_rate_in_bits(uint32 rate)
{
	switch (rate) {
		case B_SR_8000: return 8000;
		case B_SR_11025: return 11025;
		case B_SR_12000: return 12000;
		case B_SR_16000: return 16000;
		case B_SR_22050: return 22050;
		case B_SR_24000: return 24000;
		case B_SR_32000: return 32000;
		case B_SR_44100: return 44100;
		case B_SR_48000: return 48000;
	}

	return 0;
}
