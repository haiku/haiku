/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FWCFG_H_
#define _FWCFG_H_


#include <stddef.h>
#include <stdint.h>


enum {
	fwCfgSelectSignature = 0x0000,
	fwCfgSelectId        = 0x0001,
	fwCfgSelectFileDir   = 0x0019,
	fwCfgSelectFileFirst = 0x0020,
};

enum {
	fwCfgSignature = 0x554D4551,

	fwCfgIdTraditional   = 0,
	fwCfgIdDma           = 1,
};

enum {
	fwCfgDmaFlagsError  = 0,
	fwCfgDmaFlagsRead   = 1,
	fwCfgDmaFlagsSkip   = 2,
	fwCfgDmaFlagsSelect = 3,
	fwCfgDmaFlagsWrite  = 4,
};


// integer values are big endian
struct __attribute__((packed)) FwCfgFile {
    uint32_t size;
    uint16_t select;
    uint16_t reserved;
    char name[56]; // '/0' terminated
};

struct __attribute__((packed)) FwCfgFiles {
    uint32_t count;
    FwCfgFile f[];
};

struct __attribute__((packed)) FwCfgDmaAccess {
    uint32_t control;
    uint32_t length;
    uint64_t address;
};


struct __attribute__((packed)) FwCfgRegs {
	uint64_t data;
	uint16_t selector;
	uint16_t unused1;
	uint32_t unused2;
	uint64_t dmaAdr;
};


// ramfb

enum {
	ramFbFormatXrgb8888 = ((uint32_t)('X') | ((uint32_t)('R') << 8)
		| ((uint32_t)('2') << 16) | ((uint32_t)('4') << 24)),
};

// all fields are big endian
struct __attribute__((packed)) RamFbCfg {
	uint64_t addr;
	uint32_t fourcc;
	uint32_t flags;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
};


extern FwCfgRegs *volatile gFwCfgRegs;


namespace FwCfg {

void Init();

};


#endif	// _FWCFG_H_
