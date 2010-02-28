/*
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef _UDF_ALLOCATION_DESCRIPTOR_LIST_H
#define _UDF_ALLOCATION_DESCRIPTOR_LIST_H

/*! \file AllocationDescriptorList.h */

#include "UdfDebug.h"

#include "Icb.h"
#include "UdfStructures.h"
#include "Volume.h"

#include <util/kernel_cpp.h>

/*!	\brief Common interface for dealing with the three standard
	forms of allocation descriptors used in UDF icbs.

	The \c Accessor class is an allocation descriptor accessor class
	for the allocation scheme of interest. Instances of it should be
	passable by	value, and should define the following public members:
	- typedef DescriptorType;
	- inline uint8 GetType(DescriptorType &descriptor);
	- inline uint32 GetBlock(DescriptorType &descriptor);
	- inline uint16 GetPartition(DescriptorType &descriptor);
	- inline uint32 GetLength(DescriptorType &descriptor);
*/
template <class Accessor>
class AllocationDescriptorList {
private:
	typedef typename Accessor::DescriptorType Descriptor;

public:
							AllocationDescriptorList(Icb *icb,
								Accessor accessor = Accessor());

	status_t				FindExtent(off_t start, long_address *extent,
								bool *isEmpty);

private:

	off_t					_BlockIndex() const { return fBlockIndex; }
	Descriptor				*_CurrentDescriptor() const;
	Descriptor				*_DescriptorArray() const;
	size_t 					_DescriptorArraySize() const;
	int32					_DescriptorIndex() const { return fDescriptorIndex; }
	int32					_DescriptorNumber() const { return fDescriptorNumber; }
	status_t				_MoveToNextDescriptor();
	void					_Rewind();
	void					_WalkContinuationChain(Descriptor *descriptor);

	Accessor 				fAccessor;
	CachedBlock				fAdditionalDescriptors;
	off_t					fBlockIndex;
	int32					fDescriptorIndex;
	int32					fDescriptorNumber;
	Icb						*fIcb;
	Descriptor				*fIcbDescriptors;
	int32					fIcbDescriptorsSize;
	bool					fReadFromIcb;
	Volume					*fVolume;
};


template<class Accessor>
AllocationDescriptorList<Accessor>::AllocationDescriptorList(Icb *icb,
	Accessor accessor)
		:
		fAccessor(accessor),
		fAdditionalDescriptors(icb->GetVolume()),
		fBlockIndex(0),
		fDescriptorIndex(0),
		fDescriptorNumber(0),
		fIcb(icb),
		fIcbDescriptors((Descriptor *)icb->AllocationDescriptors()),
		fIcbDescriptorsSize(icb->AllocationDescriptorsSize()),
		fReadFromIcb(true),
		fVolume(icb->GetVolume())
{
	TRACE(("AllocationDescriptorList<>::AllocationDescriptorList\n"));
	_WalkContinuationChain(_CurrentDescriptor());
}


/*! \brief Finds the extent for the given address in the stream,
	returning it in the address pointed to by \a blockRun.

	\param start The byte address of interest
	\param extent The extent containing the stream address given
			by \c start.
	\param isEmpty If set to true, indicates that the given extent is
			unrecorded  and thus its contents should be interpreted
			as all zeros.
*/
template<class Accessor>
status_t
AllocationDescriptorList<Accessor>::FindExtent(off_t start,
	long_address *extent, bool *isEmpty)
{
	TRACE(("AllocationDescriptorList<>::FindExtent: start: %Ld, "
		"extent: %p, isEmpty: %p\n", start, extent, isEmpty));

	off_t startBlock = start >> fVolume->BlockShift();

	// This should never have to happen, as FindExtent is only called by
	// Icb::_Read() sequentially, as a file read is performed, but you
	// never know. :-)
	if (startBlock < _BlockIndex())
		_Rewind();

	status_t status = B_OK;
	while (true) {
		Descriptor *descriptor = _CurrentDescriptor();
		if (descriptor) {
			if (_BlockIndex() <= startBlock && startBlock
				< _BlockIndex() + fAccessor.GetLength(*descriptor)) {
				// The start block is somewhere in this extent, so return
				// the applicable tail end portion.
				off_t offset = startBlock - _BlockIndex();
				extent->set_block(fAccessor.GetBlock(*descriptor) + offset);
				extent->set_partition(fAccessor.GetPartition(*descriptor));
				extent->set_length(fAccessor.GetLength(*descriptor)
					- (offset*fVolume->BlockSize()));
				extent->set_type(fAccessor.GetType(*descriptor));
				break;
			} else {
				_MoveToNextDescriptor();
			}
		} else {
			TRACE_ERROR(("AllocationDescriptorList<>::FindExtent: "
				"Descriptor #%ld found NULL\n", _DescriptorNumber()));
			status = B_ERROR;
			break;
		}
	}

	return status;
}


//	#pragma - Private methods


template<class Accessor>
typename AllocationDescriptorList<Accessor>::Descriptor*
AllocationDescriptorList<Accessor>::_CurrentDescriptor() const
{
	TRACE(("AllocationDescriptorList<>::_CurrentDescriptor:\n"
		"\t_DescriptorIndex() + 1 * sizeof(Descriptor) = %ld\n"
		"\t_DescriptorArraySize() = %ld\n"
		"\t_DescriptorArray() = %p\n",
		(_DescriptorIndex() + 1) * sizeof(Descriptor),
		_DescriptorArraySize(), _DescriptorArray()));

	return ((_DescriptorIndex() + 1) * sizeof(Descriptor)
		<= _DescriptorArraySize())
		? &(_DescriptorArray()[_DescriptorIndex()])
		: NULL;
}


template<class Accessor>
status_t
AllocationDescriptorList<Accessor>::_MoveToNextDescriptor()
{
	Descriptor* descriptor = _CurrentDescriptor();
	if (!descriptor)
		return B_ENTRY_NOT_FOUND;

	// Increment our indices and get the next descriptor
	// from this extent.
	fBlockIndex += fAccessor.GetLength(*descriptor);
	fDescriptorIndex++;
	fDescriptorNumber++;
	descriptor = _CurrentDescriptor();

	// If no such descriptor exists, we've run out of
	// descriptors in this extent, and we're done. The
	// next time _CurrentDescriptor() is called, it will
	// return NULL, signifying this. Otherwise, we have to
	// see if the new descriptor identifies the next extent
	// of allocation descriptors, in which case we have to
	// load up the appropriate extent (guaranteed to be at
	// most one block in length by UDF-2.01 5.1 and UDF-2.01
	// 2.3.11).
	_WalkContinuationChain(descriptor);

	return B_ERROR;
}


template<class Accessor>
void
AllocationDescriptorList<Accessor>::_WalkContinuationChain(Descriptor *descriptor)
{
	TRACE(("AllocationDescriptorList<>::_WalkContinuationChain: descriptor = %p\n",
		descriptor));
	if (descriptor
		&& fAccessor.GetType(*descriptor) == EXTENT_TYPE_CONTINUATION) {
		// Load the new block, make sure we're not trying
		// to read from the icb descriptors anymore, and
		// reset the descriptor index.
		fAdditionalDescriptors.SetTo(fAccessor, *descriptor);
		fReadFromIcb = false;
		fDescriptorIndex = 0;

		// Make sure that the first descriptor in this extent isn't
		// another continuation. That would be stupid, but not
		// technically illegal.
		_WalkContinuationChain(_CurrentDescriptor());
	}
}


template<class Accessor>
void
AllocationDescriptorList<Accessor>::_Rewind()
{
	fDescriptorIndex = 0;
	fDescriptorNumber = 0;
	fReadFromIcb = true;
}


template<class Accessor>
typename AllocationDescriptorList<Accessor>::Descriptor*
AllocationDescriptorList<Accessor>::_DescriptorArray() const
{
	return fReadFromIcb ? fIcbDescriptors
		: (typename AllocationDescriptorList<Accessor>::Descriptor *)
			fAdditionalDescriptors.Block();
}


template<class Accessor>
size_t
AllocationDescriptorList<Accessor>::_DescriptorArraySize() const
{
	return fReadFromIcb ? fIcbDescriptorsSize
		: fAdditionalDescriptors.BlockSize();
}


//	pragma - Accessors


class ShortDescriptorAccessor {
public:
	ShortDescriptorAccessor(uint16 partition)
		:
		fPartition(partition)
	{
	}

	typedef short_address DescriptorType;

	inline uint32 GetBlock(DescriptorType &descriptor) const
	{
		return descriptor.block();
	}

	inline uint32 GetLength(DescriptorType &descriptor) const
	{
		return descriptor.length();
	}

	inline uint16 GetPartition(DescriptorType &descriptor) const
	{
		return fPartition;
	}

	inline uint8 GetType(DescriptorType &descriptor) const
	{
		return descriptor.type();
	}

private:
	uint16 			fPartition;
};


class LongDescriptorAccessor {
public:
	typedef long_address DescriptorType;

	inline uint32 GetBlock(DescriptorType &descriptor) const
	{
		return descriptor.block();
	}

	inline uint32 GetLength(DescriptorType &descriptor) const
	{
		return descriptor.length();
	}

	inline uint16 GetPartition(DescriptorType &descriptor) const
	{
		return descriptor.partition();
	}

	inline uint8 GetType(DescriptorType &descriptor) const
	{
		return descriptor.type();
	}
};

#endif	// _UDF_ALLOCATION_DESCRIPTOR_LIST_H
