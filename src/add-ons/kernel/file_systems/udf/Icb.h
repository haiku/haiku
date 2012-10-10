/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
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
								~Icb();

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
	void 						GetAccessTime(struct timespec &timespec) const;
	void 						GetModificationTime(struct timespec &timespec) const;

	uint8						*AllocationDescriptors()
									{ return _AbstractEntry()->AllocationDescriptors(); }
	uint32						AllocationDescriptorsSize()
									{ return _AbstractEntry()->AllocationDescriptorsLength(); }

	status_t					FindBlock(uint32 logicalBlock, off_t &block);
	status_t					Read(off_t pos, void *buffer, size_t *length,
									uint32 *block = NULL);

	void *						FileCache() { return fFileCache; }
	void *						FileMap() { return fFileMap; }

	status_t					GetFileMap(off_t offset, size_t size,
									struct file_io_vec *vecs, size_t *count);

	// for directories only
	status_t					GetDirectoryIterator(DirectoryIterator **iterator);
	status_t					Find(const char *filename, ino_t *id);

	Volume						*GetVolume() const { return fVolume; }

private:
	AbstractFileEntry			*_AbstractEntry() const { return (_Tag().id()
									== TAGID_EXTENDED_FILE_ENTRY)
									? (AbstractFileEntry *)&fExtendedEntry
									: (AbstractFileEntry *)&fFileEntry; }

	descriptor_tag				&_Tag() const { return ((icb_header *)fData.Block())->tag(); }
	icb_entry_tag				&_IcbTag() const { return ((icb_header *)fData.Block())->icb_tag(); }
	file_icb_entry				*_FileEntry() const
		{ return (file_icb_entry *)fData.Block(); }
	extended_file_icb_entry		*_ExtendedEntry() const
		{ return (extended_file_icb_entry *)fData.Block(); }

	template<class DescriptorList>
	status_t					_GetFileMap(DescriptorList &list, off_t offset,
									size_t size, struct file_io_vec *vecs,
									size_t *count);
	template<class DescriptorList>
	status_t					_Read(DescriptorList &list, off_t pos,
									void *buffer, size_t *length, uint32 *block);

	Volume						*fVolume;
	CachedBlock					fData;
	status_t					fInitStatus;
	ino_t						fId;
	SinglyLinkedList<DirectoryIterator>		fIteratorList;
	uint16						fPartition;
	FileEntry<file_icb_entry> 				fFileEntry;
	FileEntry<extended_file_icb_entry>		fExtendedEntry;
	void *						fFileCache;
	void *						fFileMap;
};


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
