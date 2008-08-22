/*
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com.
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UDF_ICB_H
#define _UDF_ICB_H

/*! \file Icb.h */

#include "UdfStructures.h"

#include <util/kernel_cpp.h>
#include <util/SinglyLinkedList.h>

#include "CachedBlock.h"

class DirectoryIterator;
class Icb;
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
	virtual uint8* 		AllocationDescriptors() = 0;
	virtual uint32		AllocationDescriptorsLength() = 0;
};


template <class Descriptor>
class FileEntry : public AbstractFileEntry {
public:
								FileEntry(CachedBlock *descriptorBlock = NULL);
	void						SetTo(CachedBlock *descriptorBlock);
	virtual uint8*				AllocationDescriptors();
	virtual uint32				AllocationDescriptorsLength();

private:
	Descriptor					*_Descriptor();
	CachedBlock					*fDescriptorBlock;
};


class DirectoryIterator : public SinglyLinkedListLinkImpl<DirectoryIterator> {
public:

	status_t						GetNextEntry(char *name, uint32 *length,
										ino_t *id);

	Icb								*Parent() { return fParent; }
	const Icb						*Parent() const { return fParent; }

	void							Rewind();

private:
friend class 						Icb;

	/* The following is called by Icb::GetDirectoryIterator() */
									DirectoryIterator(Icb *parent);
	/* The following is called by Icb::~Icb() */
	void							_Invalidate() { fParent = NULL; }

	bool							fAtBeginning;
	Icb								*fParent;
	off_t							fPosition;
};


class Icb {
public:
								Icb(Volume *volume, long_address address);
	status_t					InitCheck();
	ino_t						Id() { return fId; }

	// categorization
	uint8						Type() { return _IcbTag().file_type(); }
	bool						IsFile() { return Type() == ICB_TYPE_REGULAR_FILE; }
	bool						IsDirectory() { return (Type() == ICB_TYPE_DIRECTORY
									|| Type() == ICB_TYPE_STREAM_DIRECTORY); }

	uint32						Uid() { return _FileEntry()->uid(); }
	uint32						Gid() { return 0; }
	uint16						FileLinkCount() { return _FileEntry()->file_link_count(); }
	uint64						Length() { return _FileEntry()->information_length(); }
	mode_t						Mode() { return (IsDirectory() ? S_IFDIR : S_IFREG)
									| S_IRUSR | S_IRGRP | S_IROTH; }
	time_t						AccessTime();
	time_t						ModificationTime();

	uint8						*AllocationDescriptors()
									{ return _AbstractEntry()->AllocationDescriptors(); }
	uint32						AllocationDescriptorsSize()
									{ return _AbstractEntry()->AllocationDescriptorsLength(); }

	status_t					Read(off_t pos, void *buffer, size_t *length,
									uint32 *block = NULL);

	// for directories only
	status_t					GetDirectoryIterator(DirectoryIterator **iterator);
	status_t					Find(const char *filename, ino_t *id);

	Volume						*GetVolume() const { return fVolume; }

private:
	AbstractFileEntry			*_AbstractEntry() { return (_Tag().id()
									== TAGID_EXTENDED_FILE_ENTRY)
									? &fExtendedEntry : &fFileEntry; }

	descriptor_tag				&_Tag() { return ((icb_header *)fData.Block())->tag(); }
	icb_entry_tag				&_IcbTag() { return ((icb_header *)fData.Block())->icb_tag(); }
	file_icb_entry				*_FileEntry() { return (file_icb_entry *)fData.Block(); }
	extended_file_icb_entry		&_ExtendedEntry() { return *(extended_file_icb_entry *)fData.Block(); }

	template <class DescriptorList>
	status_t					_Read(DescriptorList &list, off_t pos,
									void *buffer, size_t *length, uint32 *block);

private:
	Volume						*fVolume;
	CachedBlock					fData;
	status_t					fInitStatus;
	ino_t						fId;
	SinglyLinkedList<DirectoryIterator>		fIteratorList;
	FileEntry<file_icb_entry> 				fFileEntry;
	FileEntry<extended_file_icb_entry>		fExtendedEntry;	
};


/*! \brief Does the dirty work of reading using the given DescriptorList object
	to access the allocation descriptors properly.
*/
template <class DescriptorList>
status_t
Icb::_Read(DescriptorList &list, off_t pos, void *_buffer, size_t *length, uint32 *block)
{
	TRACE(("Icb::_Read(): list = %p, pos = %Ld, buffer = %p, length = %ld\n",
		&list, pos, _buffer, (length ? *length : 0)));

	uint64 bytesLeftInFile = uint64(pos) > Length() ? 0 : Length() - pos;
	size_t bytesLeft = (*length >= bytesLeftInFile) ? bytesLeftInFile : *length;
	size_t bytesRead = 0;

	Volume *volume = GetVolume();
	status_t status = B_OK;
	uint8 *buffer = (uint8 *)_buffer;
	bool isFirstBlock = true;

	while (bytesLeft > 0) {

		TRACE(("Icb::_Read(): pos: %Ld, bytesLeft: %ld\n", pos, bytesLeft));
		long_address extent;
		bool isEmpty = false;
		status = list.FindExtent(pos, &extent, &isEmpty);
		if (status != B_OK) {
			TRACE_ERROR(("Icb::_Read: error finding extent for offset %Ld. "
				"status = 0x%lx `%s'\n", pos, status, strerror(status)));
			break;
		}

		TRACE(("Icb::_Read(): found extent for offset %Ld: (block: %ld, "
			"partition: %d, length: %ld, type: %d)\n", pos, extent.block(),
			extent.partition(), extent.length(), extent.type()));

		switch (extent.type()) {
			case EXTENT_TYPE_RECORDED:
				isEmpty = false;
				break;

			case EXTENT_TYPE_ALLOCATED:
			case EXTENT_TYPE_UNALLOCATED:
				isEmpty = true;
				break;

			default:
				TRACE_ERROR(("Icb::_Read(): Invalid extent type found: %d\n",
					extent.type()));
				status = B_ERROR;
				break;
		}

		if (status != B_OK)
			break;

		// Note the unmapped first block of the total read in
		// the block output parameter if provided
		if (isFirstBlock) {
			isFirstBlock = false;
			if (block)
				*block = extent.block();
		}

		off_t blockOffset
			= pos - off_t((pos >> volume->BlockShift()) << volume->BlockShift());

		size_t fullBlocksLeft = bytesLeft >> volume->BlockShift();

		if (fullBlocksLeft > 0 && blockOffset == 0) {
			TRACE(("Icb::_Read(): reading full block (or more)\n"));
			// Block aligned and at least one full block left. Read
			// in using block_cache_get_etc() calls.
			off_t diskBlock;
			status = volume->MapBlock(extent, &diskBlock);
			if (status != B_OK) {
				TRACE_ERROR(("Icb::_Read: could not map extent\n"));
				break;
			}

			size_t fullBlockBytesLeft = fullBlocksLeft << volume->BlockShift();
			size_t readLength = fullBlockBytesLeft < extent.length()
				? fullBlockBytesLeft : extent.length();

			if (isEmpty) {
				TRACE(("Icb::_Read(): reading %ld empty bytes as zeros\n",
					readLength));
				memset(buffer, 0, readLength);
			} else {
				off_t diskBlock;
				status = volume->MapBlock(extent, &diskBlock);
				if (status != B_OK) {
					TRACE_ERROR(("Icb::_Read: could not map extent\n"));
					break;
				}
				TRACE(("Icb::_Read(): reading %ld bytes from disk block %Ld "
					"using block_cache_get_etc()\n", readLength, diskBlock));
				size_t length = readLength >> volume->BlockShift();
				uint8 *data = (uint8*)block_cache_get_etc(volume->BlockCache(),
					diskBlock, pos, length);
				if (data == NULL) {
					status = B_BAD_DATA;
					break;
				}
				memcpy(buffer, data, length);
				block_cache_put(volume->BlockCache(), diskBlock);
			}

			bytesLeft -= readLength;
			bytesRead += readLength;
			pos += readLength;
			buffer += readLength;
		} else {
			off_t partialOffset;
			size_t partialLength;
			if (blockOffset == 0) {
				// Block aligned, but only a partial block's worth remaining.
				// Read in remaining bytes of file
				partialOffset = 0;
				partialLength = bytesLeft;
			} else {
				// Not block aligned, so just read up to the next block boundary.
				partialOffset = blockOffset;
				partialLength = volume->BlockSize() - blockOffset;
				if (bytesLeft < partialLength)
					partialLength = bytesLeft;
			}
			TRACE(("Icb::_Read: reading partial block. partialOffset = %Ld, "
				"partialLength: %ld\n",partialOffset, partialLength));

			if (isEmpty) {
				TRACE(("Icb::_Read: reading %ld empty bytes as zeros\n",
					partialLength));
				memset(buffer, 0, partialLength);
			} else {
				off_t diskBlock;
				status = volume->MapBlock(extent, &diskBlock);
				if (status != B_OK) {
					TRACE_ERROR(("Icb::_Read: could not map extent\n"));
					break;
				}

				TRACE(("Icb::_Read: %ld bytes from disk block %Ld using "
					"block_cache_get_etc()\n", partialLength, diskBlock));
				uint8 *data = (uint8*)block_cache_get_etc(volume->BlockCache(),
					diskBlock, 0, partialLength);
				if (data == NULL)
					break;
				memcpy(buffer, data + partialOffset, partialLength);
				block_cache_put(volume->BlockCache(), diskBlock);
			}

			bytesLeft -= partialLength;
			bytesRead += partialLength;
			pos += partialLength;
			buffer += partialLength;
		}					
	}				 
	
	*length = bytesRead;
	
	return status;
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
	return fDescriptorBlock
		? (Descriptor *)fDescriptorBlock->Block() : NULL;
}

#endif	// _UDF_ICB_H
