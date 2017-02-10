//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_ALLOCATION_DESCRIPTOR_LIST_H
#define _UDF_ALLOCATION_DESCRIPTOR_LIST_H

/*! \file AllocationDescriptorList.h
*/

#include "kernel_cpp.h"
#include "UdfDebug.h"

#include "UdfStructures.h"
#include "Icb.h"
#include "Volume.h"

namespace Udf {

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
	AllocationDescriptorList(Icb *icb, Accessor accessor = Accessor())
		: fIcb(icb)
		, fVolume(icb->GetVolume())
		, fIcbDescriptors(reinterpret_cast<Descriptor*>(icb->AllocationDescriptors()))
		, fIcbDescriptorsSize(icb->AllocationDescriptorsSize())
		, fAdditionalDescriptors(icb->GetVolume())
		, fReadFromIcb(true)
		, fAccessor(accessor)
		, fDescriptorIndex(0)
		, fDescriptorNumber(0)
		, fBlockIndex(0)
	{
		DEBUG_INIT("AllocationDescriptorList<>");
		_WalkContinuationChain(_CurrentDescriptor());
	}
	
	/*! \brief Finds the extent for the given address in the stream,
		returning it in the address pointed to by \a blockRun.
		
		\param start The byte address of interest
		\param extent The extent containing the stream address given
		       by \c start.
		\param isEmpty If set to true, indicates that the given extent is unrecorded
		       and thus its contents should be interpreted as all zeros.
	*/
	status_t FindExtent(off_t start, long_address *extent, bool *isEmpty) {
		DEBUG_INIT_ETC("AllocationDescriptorList<>",
		               ("start: %Ld, extent: %p, isEmpty: %p", start, extent, isEmpty));
		off_t startBlock = start >> fVolume->BlockShift();
	
		// This should never have to happen, as FindExtent is only called by
		// Icb::_Read() sequentially as a file read is performed, but you
		// never know. :-)
		if (startBlock < _BlockIndex())
			_Rewind();
	
		status_t error = B_OK;
		while (true) {
			Descriptor *descriptor = _CurrentDescriptor();
			if (descriptor) {
				if (_BlockIndex() <= startBlock
				      && startBlock < _BlockIndex()+fAccessor.GetLength(*descriptor))
				{
					// The start block is somewhere in this extent, so return
					// the applicable tail end portion.
					off_t offset = startBlock - _BlockIndex();
					extent->set_block(fAccessor.GetBlock(*descriptor)+offset);
					extent->set_partition(fAccessor.GetPartition(*descriptor));
					extent->set_length(fAccessor.GetLength(*descriptor)-(offset*fVolume->BlockSize()));
					extent->set_type(fAccessor.GetType(*descriptor));
					break;					
				} else {
					_MoveToNextDescriptor();
				}			
			} else {
				PRINT(("Descriptor #%ld found NULL\n", _DescriptorNumber()));
				error = B_ERROR;
				break;
			}
		}
		RETURN(error);
	}

private:
	
	Descriptor* _CurrentDescriptor() const {
		DEBUG_INIT("AllocationDescriptorList<>");
		PRINT(("(_DescriptorIndex()+1)*sizeof(Descriptor) = %ld\n", (_DescriptorIndex()+1)*sizeof(Descriptor)));
		PRINT(("_DescriptorArraySize() = %ld\n", _DescriptorArraySize()));
		PRINT(("_DescriptorArray() = %p\n", _DescriptorArray()));
		return ((_DescriptorIndex()+1)*sizeof(Descriptor) <= _DescriptorArraySize())
					? &(_DescriptorArray()[_DescriptorIndex()])
					: NULL;
	}

	status_t _MoveToNextDescriptor() {
		DEBUG_INIT("AllocationDescriptorList<>"); 
	
		Descriptor* descriptor = _CurrentDescriptor();
		if (!descriptor) {
			RETURN(B_ENTRY_NOT_FOUND);
		} else {
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
		}	
		
	
 		RETURN(B_ERROR);
	}
	
	void _WalkContinuationChain(Descriptor *descriptor) {
		DEBUG_INIT_ETC("AllocationDescriptorList<>",
		               ("descriptor: %p", descriptor));
		if (descriptor && fAccessor.GetType(*descriptor) == EXTENT_TYPE_CONTINUATION) {
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
	
	void _Rewind() {
		fDescriptorIndex = 0;
		fDescriptorNumber = 0;
		fReadFromIcb = true;
	}

	Descriptor *_DescriptorArray() const {
		return fReadFromIcb
		         ? fIcbDescriptors
		         : reinterpret_cast<Descriptor*>(fAdditionalDescriptors.Block());
	}
	
	size_t _DescriptorArraySize() const {
		return fReadFromIcb ? fIcbDescriptorsSize : fAdditionalDescriptors.BlockSize();
	}
	
	int32 _DescriptorIndex() const {
		return fDescriptorIndex;
	}
	
	int32 _DescriptorNumber() const {
		return fDescriptorNumber;
	}
	
	off_t _BlockIndex() const {
		return fBlockIndex;
	}

	Icb *fIcb;
	Volume *fVolume;
	Descriptor *fIcbDescriptors;
	int32 fIcbDescriptorsSize;
	CachedBlock fAdditionalDescriptors;
	bool fReadFromIcb;
	
	Accessor fAccessor;
	int32 fDescriptorIndex;
	int32 fDescriptorNumber;
	off_t fBlockIndex;
	
};

// Accessors

class ShortDescriptorAccessor {
public:
	ShortDescriptorAccessor(uint16 partition)
		: fPartition(partition)
	{
	}
		
	typedef short_address DescriptorType;

	inline uint8 GetType(DescriptorType &descriptor) const {
		return descriptor.type();
	}

	inline uint32 GetBlock(DescriptorType &descriptor) const {
		return descriptor.block();
	}
		
	inline uint16 GetPartition(DescriptorType &descriptor) const {
		return fPartition;
	}
	
	inline uint32 GetLength(DescriptorType &descriptor) const {
		return descriptor.length();
	}
private:
	uint16 fPartition;
};

class LongDescriptorAccessor {
public:
	typedef long_address DescriptorType;

	inline uint8 GetType(DescriptorType &descriptor) const {
		return descriptor.type();
	}

	inline uint32 GetBlock(DescriptorType &descriptor) const {
		return descriptor.block();
	}
		
	inline uint16 GetPartition(DescriptorType &descriptor) const {
		return descriptor.partition();
	}
	
	inline uint32 GetLength(DescriptorType &descriptor) const {
		return descriptor.length();
	}
};


};	// namespace Udf

#endif	// _UDF_ALLOCATION_DESCRIPTOR_LIST_H
