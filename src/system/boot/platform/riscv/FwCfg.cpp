/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FwCfg.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <ByteOrder.h>
#include <Htif.h>
#include <KernelExport.h>

#include "graphics.h"


FwCfgRegs *volatile gFwCfgRegs = NULL;


namespace FwCfg {

void
Select(uint16_t selector)
{
	// GCC, why are you so crazy?
	// gFwCfgRegs->selector = B_BENDIAN_TO_HOST_INT16(selector);
	*(uint16*)0x10100008 = B_BENDIAN_TO_HOST_INT16(selector);
}


void
DmaOp(uint8_t *bytes, size_t count, uint32_t op)
{
	__attribute__ ((aligned (8))) FwCfgDmaAccess volatile dma;
	dma.control = B_HOST_TO_BENDIAN_INT32(1 << op);
	dma.length = B_HOST_TO_BENDIAN_INT32(count);
	dma.address = B_HOST_TO_BENDIAN_INT64((addr_t)bytes);
	// gFwCfgRegs->dmaAdr = B_HOST_TO_BENDIAN_INT64((addr_t)&dma);
	*(uint64*)0x10100010 = B_HOST_TO_BENDIAN_INT64((addr_t)&dma);
	while (uint32_t control = B_BENDIAN_TO_HOST_INT32(dma.control) != 0) {
		if (((1 << fwCfgDmaFlagsError) & control) != 0)
			abort();
	}
}


void
ReadBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsRead);
}


void
WriteBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsWrite);
}


uint8_t  Read8 () {uint8_t  val; ReadBytes(          &val, sizeof(val)); return val;}
uint16_t Read16() {uint16_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint32_t Read32() {uint32_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint64_t Read64() {uint64_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}


void
ListDir()
{
	uint32_t count = B_BENDIAN_TO_HOST_INT32(Read32());
	dprintf("count: %" B_PRIu32 "\n", count);
	for (uint32_t i = 0; i < count; i++) {
		FwCfgFile file;
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = B_BENDIAN_TO_HOST_INT32(file.size);
		file.select = B_BENDIAN_TO_HOST_INT16(file.select);
		file.reserved = B_BENDIAN_TO_HOST_INT16(file.reserved);
		dprintf("\n");
		dprintf("size: %" B_PRIu32 "\n", file.size);
		dprintf("select: %" B_PRIu32 "\n", file.select);
		dprintf("reserved: %" B_PRIu32 "\n", file.reserved);
		dprintf("name: %s\n", file.name);
	}
}


bool
ThisFile(FwCfgFile& file, uint16_t dir, const char *name)
{
	Select(dir);
	uint32_t count = B_BENDIAN_TO_HOST_INT32(Read32());
	for (uint32_t i = 0; i < count; i++) {
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = B_BENDIAN_TO_HOST_INT32(file.size);
		file.select = B_BENDIAN_TO_HOST_INT16(file.select);
		file.reserved = B_BENDIAN_TO_HOST_INT16(file.reserved);
		if (strcmp(file.name, name) == 0)
			return true;
	}
	return false;
}


void
InitFramebuffer()
{
	FwCfgFile file;
	if (!ThisFile(file, fwCfgSelectFileDir, "etc/ramfb")) {
		dprintf("[!] ramfb not found\n");
		return;
	}
	dprintf("file.select: %" B_PRIu16 "\n", file.select);

	RamFbCfg cfg;
	uint32_t width = 1024, height = 768;

	gFramebuf.colors = (uint32_t*)malloc(4*width*height);
	gFramebuf.stride = width;
	gFramebuf.width = width;
	gFramebuf.height = height;

	cfg.addr = B_HOST_TO_BENDIAN_INT64((size_t)gFramebuf.colors);
	cfg.fourcc = B_HOST_TO_BENDIAN_INT32(ramFbFormatXrgb8888);
	cfg.flags = B_HOST_TO_BENDIAN_INT32(0);
	cfg.width = B_HOST_TO_BENDIAN_INT32(width);
	cfg.height = B_HOST_TO_BENDIAN_INT32(height);
	cfg.stride = B_HOST_TO_BENDIAN_INT32(4 * width);
	Select(file.select);
	WriteBytes((uint8_t*)&cfg, sizeof(cfg));
}


void
Init()
{
	dprintf("gFwCfgRegs: 0x%08" B_PRIx64 "\n", (addr_t)gFwCfgRegs);
	if (gFwCfgRegs == NULL)
		return;
	Select(fwCfgSelectSignature);
	dprintf("fwCfgSelectSignature: 0x%08" B_PRIx32 "\n", Read32());
	Select(fwCfgSelectId);
	dprintf("fwCfgSelectId: : 0x%08" B_PRIx32 "\n", Read32());
	Select(fwCfgSelectFileDir);
	ListDir();
	InitFramebuffer();
}


};
