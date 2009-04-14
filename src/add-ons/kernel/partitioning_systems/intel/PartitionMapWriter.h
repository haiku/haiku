/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
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
struct partition_table;

/*!
  \brief Writer for "Intel" style partitions.

  This class serves for writing \a primary and \a logical \a partitions to disk.
*/
class PartitionMapWriter {
public:
								PartitionMapWriter(int deviceFD,
									off_t sessionOffset, off_t sessionSize);
								~PartitionMapWriter();

			status_t			WriteMBR(const PartitionMap* map,
									bool clearSectors);
			status_t			WriteLogical(partition_table* pts,
									const LogicalPartition* partition);
			status_t			WriteExtendedHead(partition_table* pts,
									const LogicalPartition* firstPartition);

private:
			status_t			_WritePrimary(partition_table* pts);
			status_t			_WriteExtended(partition_table* pts,
									const LogicalPartition* partition,
									const LogicalPartition* next);

			status_t			_ReadSector(off_t offset,
									partition_table* pts);
			status_t			_WriteSector(off_t offset,
									const partition_table* pts);

private:
			int					fDeviceFD;
			off_t				fSessionOffset;
			off_t				fSessionSize;

			const PartitionMap* fMap; // while writing
};

#endif	// PARTITION_MAP_WRITER_H
