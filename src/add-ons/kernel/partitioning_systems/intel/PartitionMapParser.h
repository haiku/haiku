/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */
#ifndef PARTITION_MAP_PARSER_H
#define PARTITION_MAP_PARSER_H


#include <SupportDefs.h>


class Partition;
class PartitionMap;
class PrimaryPartition;
struct partition_table_sector;

class PartitionMapParser {
	public:
		PartitionMapParser(int deviceFD, off_t sessionOffset, off_t sessionSize,
			int32 blockSize);
		~PartitionMapParser();

		status_t Parse(const uint8 *block, PartitionMap *map);

		int32 CountPartitions() const;
		const Partition *PartitionAt(int32 index) const;

	private:
		status_t _ParsePrimary(const partition_table_sector *pts);
		status_t _ParseExtended(PrimaryPartition *primary, off_t offset);
		status_t _ReadPTS(off_t offset, partition_table_sector *pts = NULL);

	private:
		int						fDeviceFD;
		off_t					fSessionOffset;
		off_t					fSessionSize;
		int32					fBlockSize;
		partition_table_sector	*fPTS;	// while parsing
		PartitionMap			*fMap;
};

#endif	// PARTITION_MAP_PARSER_H
