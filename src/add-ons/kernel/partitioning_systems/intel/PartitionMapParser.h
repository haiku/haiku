/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */

/*!	\file PartitionMapParser.h
	\brief Implementation of disk parser for "intel" style partitions.

	Parser reads primary and logical partitions from the disk (according to
	Master Boot and Extended Boot Records) and fills \c PartitionMap structure
	with partition representation.
*/
#ifndef PARTITION_MAP_PARSER_H
#define PARTITION_MAP_PARSER_H


#include <SupportDefs.h>


class Partition;
class PartitionMap;
class PrimaryPartition;
struct partition_table;

class PartitionMapParser {
public:
								PartitionMapParser(int deviceFD,
									off_t sessionOffset, off_t sessionSize,
									uint32 blockSize);
								~PartitionMapParser();

			status_t			Parse(const uint8* block, PartitionMap* map);

			int32				CountPartitions() const;
			const Partition*	PartitionAt(int32 index) const;

private:
			status_t			_ParsePrimary(const partition_table* table,
									bool& hadToReFitSize);
			status_t			_ParseExtended(PrimaryPartition* primary,
									off_t offset);
			status_t			_ReadPartitionTable(off_t offset,
									partition_table* table = NULL);

private:
			int					fDeviceFD;
			uint32				fBlockSize;
			off_t				fSessionOffset;
			off_t				fSessionSize;
			partition_table*	fPartitionTable;	// while parsing
			PartitionMap*		fMap;
};

#endif	// PARTITION_MAP_PARSER_H
