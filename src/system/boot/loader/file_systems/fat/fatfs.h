/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef FATFS_H
#define FATFS_H


#include <SupportDefs.h>
#include <ByteOrder.h>

namespace FATFS {

class Volume;

// mode bits
#define FAT_READ_ONLY   1
#define FAT_HIDDEN              2
#define FAT_SYSTEM              4
#define FAT_VOLUME              8
#define FAT_SUBDIR              16
#define FAT_ARCHIVE             32

#define read32(buffer,off) \
        B_LENDIAN_TO_HOST_INT32(*(uint32 *)&buffer[off])

#define read16(buffer,off) \
        B_LENDIAN_TO_HOST_INT16(*(uint16 *)&buffer[off])

#define write32(buffer, off, value) \
        *(uint32*)&buffer[off] = B_HOST_TO_LENDIAN_INT32(value)

#define write16(buffer, off, value) \
        *(uint16*)&buffer[off] = B_HOST_TO_LENDIAN_INT16(value)

enum name_lengths {
	FATFS_BASENAME_LENGTH	= 8,
	FATFS_EXTNAME_LENGTH	= 3,
	FATFS_NAME_LENGTH	= 12,
};

status_t get_root_block(int fDevice, char *buffer, int32 blockSize, off_t partitionSize);


}	// namespace FATFS

#endif	/* FATFS_H */

