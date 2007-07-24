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

	Writer can write \b Master \b Boot \b Record or \b Extended \b Boot \b Records
	to the disk according to partitions defined in \c PartitionMap structure.
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
		/*!
		  \brief Creates the writer.

		  \param deviceFD File descriptor.
		  \param sessionOffset Disk offset of the partition with partitioning system.
		  \param sessionSize Size of the partition with partitioning system.
		  \param blockSize Size of the sector on given disk.
		*/
		PartitionMapWriter(int deviceFD, off_t sessionOffset, off_t sessionSize,
			int32 blockSize);
		~PartitionMapWriter();

		/*!
		  \brief Writes Master Boot Record to the first sector of the disk.

		  If a \a block is not specified, the sector is firstly read from the disk
		  and after changing relevant items it is written back to the disk.
		  This allows to keep code area in MBR intact.
		  \param block Pointer to \c partition_table_sector.
		  \param map Pointer to the PartitionMap structure describing disk partitions.
		*/
		status_t WriteMBR(uint8 *block, const PartitionMap *map);
		/*!
		  \brief Writes Partition Table Sector of the logical \a partition to the disk.

		  This function ensures that the connection of the following linked list of logical
		  partitions will be correct. It do nothing with the connection of previous logical
		  partitions (call this function on previous logical partition to ensure it).
		  \param block Pointer to \c partition_table_sector.
		  \param partition Pointer to the logical partition.
		*/
		status_t WriteLogical(uint8 *block, const LogicalPartition *partition);
		/*!
		  \brief Writes Extended Boot Record to the first sector of Extended Partition.

		  Writes the head of linked list describing logical partitions.

		  If the \a first_partition is not specified, it only initializes EBR and the linked
		  list contains no logical partitions.
		  \param block Pointer to \c partition_table_sector.
		  \param first_partition Pointer to the first logical partition.
		*/
		status_t WriteExtendedHead(uint8 *block, const LogicalPartition *first_partition);

	private:
		status_t _WritePrimary(partition_table_sector *pts);
		status_t _WriteExtended(partition_table_sector *pts,
								const LogicalPartition *partition,
								const LogicalPartition *next);
		status_t _ReadPTS(off_t offset, partition_table_sector *pts = NULL);
		status_t _WritePTS(off_t offset, const partition_table_sector *pts = NULL);

	private:
		int						fDeviceFD;
		off_t					fSessionOffset;
		off_t					fSessionSize;
		int32					fBlockSize;
		partition_table_sector	*fPTS;	// while writing
		const PartitionMap		*fMap;
};

#endif	// PARTITION_MAP_WRITER_H
