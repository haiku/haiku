// Pef.h

#ifndef _PEF_H
#define _PEF_H

#include <SupportDefs.h>

typedef char	PefOSType[4];

// container header
struct PEFContainerHeader {
	PefOSType	tag1;
	PefOSType	tag2;
	PefOSType	architecture;
	uint32		formatVersion;
	uint32		dateTimeStamp;
	uint32		oldDefVersion;
	uint32		oldImpVersion;
	uint32		currentVersion;
	uint16		sectionCount;
	uint16		instSectionCount;
	uint32		reservedA;
};

const char	kPEFFileMagic1[4]		= { 'J', 'o', 'y', '!' };
const char	kPEFFileMagic2[4]		= { 'p', 'e', 'f', 'f' };
const char	kPEFArchitecturePPC[4]	= { 'p', 'w', 'p', 'c' };
const char	kPEFContainerHeaderSize	= 40;

// section header
struct PEFSectionHeader {
	int32	nameOffset;
	uint32	defaultAddress;
	uint32	totalSize;
	uint32	unpackedSize;
	uint32	packedSize;
	uint32	containerOffset;
	uint8	sectionKind;
	uint8	shareKind;
	uint8	alignment;
	uint8	reservedA;
};

const uint32 kPEFSectionHeaderSize		= 28;

#endif	// _PEF_H


