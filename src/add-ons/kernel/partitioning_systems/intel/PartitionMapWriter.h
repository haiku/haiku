/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */

/*!
	\file PartitionMapWriter.h
	\ingroup intel_module
	\brief Implementation of disk writer for "intel" style partitions.

	Writer can write \b Master \b Boot \b Record or \b Extended \b Boot
	\b Records to the disk according to partitions defined in \c PartitionMap
	structure.
*/


#ifndef PARTITION_MAP_WRITER_H
#define PARTITION_MAP_WRITER_H


#include <SupportDefs.h>


class PartitionMap;
class LogicalPartition;
struct partition_table_sector;

/*!
  \brief Writer for "Intel" style partitions.

  This class serves for writing \a primary and \a logical \a partitions to disk.
*/
class PartitionMapWriter {
public:
	PartitionMapWriter(int deviceFD, off_t sessionOffset, off_t sessionSize);
	~PartitionMapWriter();

	status_t WriteMBR(const PartitionMap *map, bool clearSectors);
	status_t WriteLogical(partition_table_sector *pts,
		const LogicalPartition *partition);
	status_t WriteExtendedHead(partition_table_sector *pts,
		const LogicalPartition *first_partition);

private:
	status_t _WritePrimary(partition_table_sector *pts);
	status_t _WriteExtended(partition_table_sector *pts,
		const LogicalPartition *partition, const LogicalPartition *next);
	status_t _ReadPTS(off_t offset, partition_table_sector *pts = NULL);
	status_t _WriteSector(off_t offset, const void* pts = NULL);

private:
	int						fDeviceFD;
	off_t					fSessionOffset;
	off_t					fSessionSize;
	partition_table_sector	*fPTS;	// while writing
	const PartitionMap		*fMap;
};

#endif	// PARTITION_MAP_WRITER_H
