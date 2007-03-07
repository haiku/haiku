// DataContainer.h

#ifndef DATA_CONTAINER_H
#define DATA_CONTAINER_H

#include "List.h"

class AllocationInfo;
class BlockReference;
class Volume;

// Size of the DataContainer's small buffer. If it contains data up to this
// size, no blocks are allocated, but the small buffer is used instead.
// 16 bytes are for free, since they are shared with the block list.
// (actually even more, since the list has an initial size).
// I ran a test analyzing what sizes the attributes in my system have:
//     size   percentage   bytes used in average 
//   <=   0         0.00                   93.45
//   <=   4        25.46                   75.48
//   <=   8        30.54                   73.02
//   <=  16        52.98                   60.37
//   <=  32        80.19                   51.74
//   <=  64        94.38                   70.54
//   <= 126        96.90                  128.23
//
// For average memory usage it is assumed, that attributes larger than 126
// bytes have size 127, that the list has an initial capacity of 10 entries
// (40 bytes), that the block reference consumes 4 bytes and the block header
// 12 bytes. The optimal length is actually 35, with 51.05 bytes per
// attribute, but I conservatively rounded to 32.
static const size_t kSmallDataContainerSize = 32;

class DataContainer {
public:
	DataContainer(Volume *volume);
	virtual ~DataContainer();

	status_t InitCheck() const;

	Volume *GetVolume() const	{ return fVolume; }

	status_t Resize(off_t newSize);
	off_t GetSize() const { return fSize; }

	virtual status_t ReadAt(off_t offset, void *buffer, size_t size,
							size_t *bytesRead);
	virtual status_t WriteAt(off_t offset, const void *buffer, size_t size,
							 size_t *bytesWritten);

	void GetFirstDataBlock(const uint8 **data, size_t *length);

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	typedef List<BlockReference*>	BlockList;

private:
	static inline bool _RequiresBlockMode(size_t size);
	inline bool _IsBlockMode() const;

	inline BlockList *_GetBlockList();
	inline const BlockList *_GetBlockList() const;
	inline int32 _CountBlocks() const;
	inline void *_GetBlockDataAt(int32 index, size_t offset, size_t size);

	void _ClearArea(off_t offset, off_t size);

	status_t _Resize(off_t newSize);
	status_t _ResizeLastBlock(size_t newSize);

	status_t _SwitchToBlockMode(size_t newBlockSize);
	void _SwitchToSmallBufferMode(size_t newSize);

private:
	Volume					*fVolume;
	off_t					fSize;
	union {
		uint8				fBlocks[sizeof(BlockList)];
		uint8				fSmallBuffer[kSmallDataContainerSize];
	};
};

#endif	// DATA_CONTAINER_H
