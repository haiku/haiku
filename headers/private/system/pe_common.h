/*
 * Copyright 2013-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Alexander von Gluck IV, <kallisti5@unixzen.com>
 */
#ifndef _PE_COMMON_H
#define _PE_COMMON_H


#include <SupportDefs.h>
#include <ByteOrder.h>


// Magic strings
#define MZ_MAGIC "MZ"
#define PE_MAGIC "PE"
#define PE_OPTIONAL_MAGIC_PE32 0x010b
#define PE_OPTIONAL_MAGIC_PE32P 0x020b


typedef struct {
	uint16 magic; /* == MZ_MAGIC */
	uint16 bytesInLastBlock;
	uint16 blocksInFile;
	uint16 numRelocations;
	uint16 headerParagraphs;
	uint16 minExtraParagraphs;
	uint16 maxExtraParagraphs;
	uint16 ss;
	uint16 sp;
	uint16 checksum;
	uint16 ip;
	uint16 cs;
	uint16 relocationTableOffset;
	uint16 overlayNumber;
	uint16 reserved[4];
	uint16 oemID;
	uint16 oemInfo;
	uint16 reserved2[10];
	uint32 lfaNew;  // PE Header start addr
} MzHeader;

typedef struct {
	uint32 magic; // == PE_MAGIC */
	uint16 machine;
	uint16 numberOfSections;
	uint32 timeDateStamp;
	uint32 pointerToSymbolTable;
	uint32 numberOfSymbols;
	uint16 sizeOfOptionalHeader;
	uint16 characteristics;
} PeHeader;

typedef struct {
	uint16 magic; // == 0x010b - PE32, 0x020b - PE32+ (64 bit)
	uint8  majorLinkerVersion;
	uint8  minorLinkerVersion;
	uint32 sizeOfCode;
	uint32 sizeOfInitializedData;
	uint32 sizeOfUninitializedData;
	uint32 addressOfEntryPoint;
	uint32 baseOfCode;
	uint32 baseOfData;
	uint32 imageBase;
	uint32 sectionAlignment;
	uint32 fileAlignment;
	uint16 majorOperatingSystemVersion;
	uint16 minorOperatingSystemVersion;
	uint16 majorImageVersion;
	uint16 minorImageVersion;
	uint16 majorSubsystemVersion;
	uint16 minorSubsystemVersion;
	uint32 win32VersionValue;
	uint32 sizeOfImage;
	uint32 sizeOfHeaders;
	uint32 checksum;
	uint16 subsystem;
	uint16 llCharacteristics;
	uint32 sizeOfStackReserve;
	uint32 sizeOfStackCommit;
	uint32 sizeOfHeapReserve;
	uint32 sizeOfHeapCommit;
	uint32 loaderFlags;
	uint32 numberOfRvaAndSizes;
} Pe32OptionalHeader;

#endif /* _PE_COMMON_H */
