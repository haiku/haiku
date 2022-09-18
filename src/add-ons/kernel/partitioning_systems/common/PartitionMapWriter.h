/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 *		Bryce Groff, brycegroff@gmail.com
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
class PrimaryPartition;
struct partition_table;

bool check_logical_location(const LogicalPartition* logical,
	const PrimaryPartition* primary);

/*!
  \brief Writer for "Intel" style partitions.

  This class serves for writing \a primary and \a logical \a partitions to disk.
*/
class PartitionMapWriter {
public:
								PartitionMapWriter(int deviceFD,
									uint32 blockSize);
								~PartitionMapWriter();

			status_t			WriteMBR(const PartitionMap* map,
									bool writeBootCode);
			status_t			WriteLogical(const LogicalPartition* logical,
									const PrimaryPartition* primary,
									bool clearCode);
			status_t			WriteExtendedHead(
									const LogicalPartition* logical,
									const PrimaryPartition* primary,
									bool clearCode);

			status_t			ClearExtendedHead(
									const PrimaryPartition* primary);

private:
			status_t			_ReadBlock(off_t offset,
									partition_table& partitionTable);
			status_t			_WriteBlock(off_t offset,
									const partition_table& partitionTable);

private:
			int					fDeviceFD;
			int32				fBlockSize;

};

#endif	// PARTITION_MAP_WRITER_H
