//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_ICB_H
#define _UDF_ICB_H

/*! \file Icb.h
*/

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif
#ifdef COMPILE_FOR_R5
extern "C" {
#endif
	#include "fsproto.h"
#ifdef COMPILE_FOR_R5
}
#endif

#include "kernel_cpp.h"
#include "UdfDebug.h"

#include "CachedBlock.h"
#include "UdfStructures.h"
#include "SinglyLinkedList.h"

namespace Udf {

class DirectoryIterator;
class Volume;

/*! \brief Abstract interface to file entry structure members
	that are not commonly accessible through file_icb_entry().
	
	This is necessary, since we can't use virtual functions in
	the disk structure structs themselves, since we generally
	don't create disk structure objects by calling new, but
	rather just cast a chunk of memory read off disk to be
	a pointer to the struct of interest (which works fine
	for regular functions, but fails miserably for virtuals
	due to the vtable not being setup properly).
*/
class AbstractFileEntry {
public:
	virtual uint8* AllocationDescriptors() = 0;
	virtual uint32 AllocationDescriptorsLength() = 0;
};

template <class Descriptor>
class FileEntry : public AbstractFileEntry {
public:
	FileEntry(CachedBlock *descriptorBlock = NULL);
	void SetTo(CachedBlock *descriptorBlock);
	virtual uint8* AllocationDescriptors();
	virtual uint32 AllocationDescriptorsLength();
private:
	Descriptor* _Descriptor();
	
	CachedBlock *fDescriptorBlock;
};

class Icb {
public:
	Icb(Volume *volume, long_address address);
	status_t InitCheck();
	vnode_id Id() { return fId; }
	
	// categorization
	uint8 Type() { return IcbTag().file_type(); }
	bool IsFile() { return InitCheck() == B_OK && Type() == ICB_TYPE_REGULAR_FILE; }
	bool IsDirectory() { return InitCheck() == B_OK
	                     && (Type() == ICB_TYPE_DIRECTORY || Type() == ICB_TYPE_STREAM_DIRECTORY); }
	
	uint32 Uid() { return 0; }//FileEntry()->uid(); }
	uint32 Gid() { return 0; }
	uint16 FileLinkCount() { return FileEntry()->file_link_count(); }
	uint64 Length() { return FileEntry()->information_length(); }
	mode_t Mode() { return (IsDirectory() ? S_IFDIR : S_IFREG) | S_IRUSR | S_IRGRP | S_IROTH; }
	time_t AccessTime();
	time_t ModificationTime();
	
	uint8 *AllocationDescriptors() { return AbstractEntry()->AllocationDescriptors(); }
	uint32 AllocationDescriptorsSize() { return AbstractEntry()->AllocationDescriptorsLength(); }
	
	status_t Read(off_t pos, void *buffer, size_t *length, uint32 *block = NULL);
	
	// for directories only
	status_t GetDirectoryIterator(DirectoryIterator **iterator);
	status_t Find(const char *filename, vnode_id *id);
	
	Volume* GetVolume() const { return fVolume; }
	
private:
	Icb();	// unimplemented
		
	descriptor_tag & Tag() { return (reinterpret_cast<icb_header*>(fData.Block()))->tag(); }
	icb_entry_tag& IcbTag() { return (reinterpret_cast<icb_header*>(fData.Block()))->icb_tag(); }
	AbstractFileEntry* AbstractEntry() {
		DEBUG_INIT("Icb");
		return (Tag().id() == TAGID_EXTENDED_FILE_ENTRY)
//	             ? reinterpret_cast<extended_file_icb_entry*>(fData.Block())
//	             : reinterpret_cast<file_icb_entry*>(fData.Block()));
	             ? &fExtendedEntry
	             : &fFileEntry;
	}
	file_icb_entry* FileEntry() { return (reinterpret_cast<file_icb_entry*>(fData.Block())); }
	extended_file_icb_entry& ExtendedEntry() { return *(reinterpret_cast<extended_file_icb_entry*>(fData.Block())); }

	template <class DescriptorList>
	status_t _Read(DescriptorList &list, off_t pos, void *buffer, size_t *length, uint32 *block);

	
private:
	Volume *fVolume;
	CachedBlock fData;
	status_t fInitStatus;
	vnode_id fId;
	SinglyLinkedList<DirectoryIterator*> fIteratorList;
	/* [zooey]: gcc-2.95.3 requires the explicit namespace here, otherwise
	   it complains about a syntax error(!). This is most probably a bug. */
	Udf::FileEntry<file_icb_entry> fFileEntry;
	Udf::FileEntry<extended_file_icb_entry> fExtendedEntry;	
};

/*! \brief Does the dirty work of reading using the given DescriptorList object
	to access the allocation descriptors properly.
*/
template <class DescriptorList>
status_t
Icb::_Read(DescriptorList &list, off_t pos, void *_buffer, size_t *length, uint32 *block)
{
	DEBUG_INIT_ETC("Icb", ("list: %p, pos: %Ld, buffer: %p, length: (%p)->%ld",
	               &list, pos, _buffer, length, (length ? *length : 0))); 
	if (!_buffer || !length)
		RETURN(B_BAD_VALUE);
		
	uint64 bytesLeftInFile = uint64(pos) > Length() ? 0 : Length() - pos;
	size_t bytesLeft = (*length >= bytesLeftInFile) ? bytesLeftInFile : *length;
	size_t bytesRead = 0;
	
	Volume *volume = GetVolume();		
	status_t error = B_OK;		
	uint8 *buffer = reinterpret_cast<uint8*>(_buffer);
	bool isFirstBlock = true;
	
	while (bytesLeft > 0 && !error) {
		
		PRINT(("pos: %Ld\n", pos));
		PRINT(("bytesLeft: %ld\n", bytesLeft));
		
		long_address extent;
		bool isEmpty = false;
		error = list.FindExtent(pos, &extent, &isEmpty);
		if (!error) {
			PRINT(("found extent for offset %Ld: (block: %ld, partition: %d, length: %ld, type: %d)\n",
			       pos, extent.block(), extent.partition(), extent.length(), extent.type()));
			       
			switch (extent.type()) {
				case EXTENT_TYPE_RECORDED:
					isEmpty = false;
					break;
				
				case EXTENT_TYPE_ALLOCATED:
				case EXTENT_TYPE_UNALLOCATED:
					isEmpty = true;
					break;
				
				default:
					PRINT(("Invalid extent type found: %d\n", extent.type()));
					error = B_ERROR;
					break;			
			}
			
			if (!error) {
				// Note the unmapped first block of the total read in
				// the block output parameter if provided
				if (isFirstBlock) {
					isFirstBlock = false;
					if (block)
						*block = extent.block();
				}
							
				off_t blockOffset = pos - off_t((pos >> volume->BlockShift()) << volume->BlockShift());
				size_t fullBlocksLeft = bytesLeft >> volume->BlockShift();
				
				if (fullBlocksLeft > 0 && blockOffset == 0) {
					PRINT(("reading full block (or more)\n"));				
					// Block aligned and at least one full block left. Read in using
					// cached_read() calls.
					off_t diskBlock;
					error = volume->MapBlock(extent, &diskBlock);
					if (!error) {
						size_t fullBlockBytesLeft = fullBlocksLeft << volume->BlockShift();
						size_t readLength = fullBlockBytesLeft < extent.length()
						                      ? fullBlockBytesLeft
						                      : extent.length();					
					
						if (isEmpty) {
							PRINT(("reading %ld empty bytes as zeros\n", readLength));
							memset(buffer, 0, readLength);
						} else {
							off_t diskBlock;
							error = volume->MapBlock(extent, &diskBlock);
							if (!error) {							
								PRINT(("reading %ld bytes from disk block %Ld using cached_read()\n",
								       readLength, diskBlock));
								error = cached_read(volume->Device(), diskBlock, buffer,
								                  readLength >> volume->BlockShift(),
								                  volume->BlockSize());
							} 
						}
						
						if (!error) {
							bytesLeft -= readLength;
							bytesRead += readLength;
							pos += readLength;
							buffer += readLength;
						}					
					}
					
				} else {
					PRINT(("partial block\n"));
					off_t partialOffset;
					size_t partialLength;
					if (blockOffset == 0) {
						// Block aligned, but only a partial block's worth remaining. Read
						// in remaining bytes of file
						partialOffset = 0;
						partialLength = bytesLeft;
					} else {
						// Not block aligned, so just read up to the next block boundary.
						partialOffset = blockOffset;
						partialLength = volume->BlockSize() - blockOffset;
						if (bytesLeft < partialLength)
							partialLength = bytesLeft;
					}					
					
					PRINT(("partialOffset: %Ld\n", partialOffset));
					PRINT(("partialLength: %ld\n", partialLength));
					
					if (isEmpty) {
						PRINT(("reading %ld empty bytes as zeros\n", partialLength));
						memset(buffer, 0, partialLength);						
					} else {
						off_t diskBlock;
						error = volume->MapBlock(extent, &diskBlock);
						if (!error) {
							PRINT(("reading %ld bytes from disk block %Ld using get_block()\n",
							       partialLength, diskBlock));
							uint8 *data = (uint8*)get_block(volume->Device(), diskBlock, volume->BlockSize());
							error = data ? B_OK : B_BAD_DATA;
							if (!error) {
								memcpy(buffer, data+partialOffset, partialLength);
								release_block(volume->Device(), diskBlock);
							}
						}
					}
					
					if (!error) {
						bytesLeft -= partialLength;
						bytesRead += partialLength;
						pos += partialLength;
						buffer += partialLength;
					}					
				}				 
			}
		} else {
			PRINT(("error finding extent for offset %Ld: 0x%lx, `%s'", pos,
			       error, strerror(error)));
			break;
		}
	}
	
	*length = bytesRead;
	
	RETURN(error);
}

template <class Descriptor>
FileEntry<Descriptor>::FileEntry(CachedBlock *descriptorBlock)
	: fDescriptorBlock(descriptorBlock)
{
}

template <class Descriptor>
void
FileEntry<Descriptor>::SetTo(CachedBlock *descriptorBlock)
{
	fDescriptorBlock = descriptorBlock;
}

template <class Descriptor>
uint8*
FileEntry<Descriptor>::AllocationDescriptors()
{
	Descriptor* descriptor = _Descriptor();
	return descriptor ? descriptor->allocation_descriptors() : NULL;
}

template <class Descriptor>
uint32
FileEntry<Descriptor>::AllocationDescriptorsLength()
{
	Descriptor* descriptor = _Descriptor();
	return descriptor ? descriptor->allocation_descriptors_length() : 0;
}

template <class Descriptor>
Descriptor*
FileEntry<Descriptor>::_Descriptor()
{
	return fDescriptorBlock ? reinterpret_cast<Descriptor*>(fDescriptorBlock->Block()) : NULL;
}

};	// namespace Udf

#endif	// _UDF_ICB_H
