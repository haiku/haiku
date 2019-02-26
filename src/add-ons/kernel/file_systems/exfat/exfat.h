/*
 * Copyright 2011-2019, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2014 Haiku, Inc. All Rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		John Scipione, jscipione@gmail.com
 */
#ifndef EXFAT_H
#define EXFAT_H


#include <sys/stat.h>

#include <ByteOrder.h>
#include <fs_interface.h>
#include <KernelExport.h>


typedef uint64 fileblock_t;		// file block number
typedef uint64 fsblock_t;		// filesystem block number

typedef uint32 cluster_t;


#define EXFAT_SUPER_BLOCK_OFFSET	0x0
#define EXFAT_FIRST_DATA_CLUSTER	2


struct exfat_super_block {
	uint8	jump_boot[3];
	char	filesystem[8];
	uint8	reserved[53];
	uint64	first_block;
	uint64	num_blocks;
	uint32	first_fat_block;
	uint32	fat_length;
	uint32	first_data_block;
	uint32	cluster_count;
	uint32	root_dir_cluster;
	uint32	serial_number;
	uint8	version_minor;
	uint8	version_major;
	uint16	flags;
	uint8	block_shift;
	uint8	blocks_per_cluster_shift;
	uint8	fat_count;
	uint8	drive_select;
	uint8	used_percent;
	uint8	reserved2[7];
	uint8	boot_code[390];
	uint16	signature;

	bool IsValid();
		// implemented in Volume.cpp
	uint64 FirstBlock() const { return B_LENDIAN_TO_HOST_INT64(first_block); }
	uint64 NumBlocks() const { return B_LENDIAN_TO_HOST_INT64(num_blocks); }
	uint32 FirstFatBlock() const
		{ return B_LENDIAN_TO_HOST_INT32(first_fat_block); }
	uint32 FatLength() const
		{ return B_LENDIAN_TO_HOST_INT32(fat_length); }
	uint32 FirstDataBlock() const
		{ return B_LENDIAN_TO_HOST_INT32(first_data_block); }
	uint32 ClusterCount() const
		{ return B_LENDIAN_TO_HOST_INT32(cluster_count); }
	uint32 RootDirCluster() const
		{ return B_LENDIAN_TO_HOST_INT32(root_dir_cluster); }
	uint32 SerialNumber() const
		{ return B_LENDIAN_TO_HOST_INT32(serial_number); }
	uint8 VersionMinor() const { return version_minor; }
	uint8 VersionMajor() const { return version_major; }
	uint16 Flags() const { return B_LENDIAN_TO_HOST_INT16(flags); }
	uint8 BlockShift() const { return block_shift; }
	uint8 BlocksPerClusterShift() const { return blocks_per_cluster_shift; }
	uint8 FatCount() const { return fat_count; }
	uint8 DriveSelect() const { return drive_select; }
	uint8 UsedPercent() const { return used_percent; }
} _PACKED;


#define EXFAT_SUPER_BLOCK_MAGIC			"EXFAT   "

#define EXFAT_ENTRY_TYPE_NOT_IN_USE		0x03
#define EXFAT_ENTRY_TYPE_BITMAP			0x81
#define EXFAT_ENTRY_TYPE_UPPERCASE		0x82
#define EXFAT_ENTRY_TYPE_LABEL			0x83
#define EXFAT_ENTRY_TYPE_FILE			0x85
#define EXFAT_ENTRY_TYPE_GUID			0xa0
#define EXFAT_ENTRY_TYPE_FILEINFO		0xc0
#define EXFAT_ENTRY_TYPE_FILENAME		0xc1
#define EXFAT_CLUSTER_END				0xffffffff
#define EXFAT_ENTRY_ATTRIB_SUBDIR		0x10

#define EXFAT_ENTRY_FLAG_CONTIGUOUS		0x3

#define EXFAT_FILENAME_MAX_LENGTH		512


struct exfat_entry {
	uint8	type;
	union {
		struct {
			uint8 length;
			uint16 name[11];
			uint8 reserved[8];
		} _PACKED volume_label;
		struct {
			uint8 chunkCount;
			uint16 checksum;
			uint16 flags;
			uint8 guid[16];
			uint8 reserved[10];
		} _PACKED volume_guid;
		struct {
			uint8 reserved[3];
			uint32 checksum;
			uint8 reserved2[12];
			uint32 start_cluster;
			uint64 size;
		} _PACKED bitmap_uppercase;
		struct {
			uint8 chunkCount;
			uint16 checksum;
			uint16 attribs;
			uint16 reserved;
			uint16 creation_time;
			uint16 creation_date;
			uint16 modification_time;
			uint16 modification_date;
			uint16 access_time;
			uint16 access_date;
			uint8 creation_time_low;
			uint8 modification_time_low;
			uint8 reserved2[10];
			uint16 ModificationTime() const
				{ return B_LENDIAN_TO_HOST_INT16(modification_time); }
			uint16 ModificationDate() const
				{ return B_LENDIAN_TO_HOST_INT16(modification_date); }
			uint16 AccessTime() const
				{ return B_LENDIAN_TO_HOST_INT16(access_time); }
			uint16 AccessDate() const
				{ return B_LENDIAN_TO_HOST_INT16(access_date); }
			uint16 CreationTime() const
				{ return B_LENDIAN_TO_HOST_INT16(creation_time); }
			uint16 CreationDate() const
				{ return B_LENDIAN_TO_HOST_INT16(creation_date); }
			uint16 Attribs() const
				{ return B_LENDIAN_TO_HOST_INT16(attribs); }
			void SetAttribs(uint16 newAttribs)
				{ attribs = B_HOST_TO_LENDIAN_INT16(newAttribs); }
		} _PACKED file;
		struct {
			uint8 flag;
			uint8 reserved;
			uint8 name_length;
			uint16 name_hash;
			uint8 reserved2[2];
			uint64 size1;
			uint8 reserved3[4];
			uint32 start_cluster;
			uint64 size2;
			uint32 StartCluster() const
				{ return B_LENDIAN_TO_HOST_INT32(start_cluster); }
			void SetStartCluster(uint32 startCluster)
				{ start_cluster = B_HOST_TO_LENDIAN_INT32(startCluster); }
			bool IsContiguous() const
				{ return (flag & EXFAT_ENTRY_FLAG_CONTIGUOUS) != 0; }
			void SetFlag(uint8 newFlag)
				{ flag = newFlag; }
			uint64 Size() const
				{ return B_LENDIAN_TO_HOST_INT64(size2); }
		} _PACKED file_info;
		struct {
			uint8 flags;
			uint16 name[15];
		} _PACKED file_name;
	};
} _PACKED;


struct file_cookie {
	bigtime_t	last_notification;
	off_t		last_size;
	int			open_mode;
};

#define EXFAT_OPEN_MODE_USER_MASK		0x7fffffff

extern fs_volume_ops gExfatVolumeOps;
extern fs_vnode_ops gExfatVnodeOps;

#endif	// EXFAT_H
