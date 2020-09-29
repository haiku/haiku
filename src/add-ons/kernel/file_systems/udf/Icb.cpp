/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2010, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */

#include "Icb.h"

#include "time.h"

#include "AllocationDescriptorList.h"
#include "Utils.h"
#include "Volume.h"

#include <file_cache.h>


status_t
DirectoryIterator::GetNextEntry(char *name, uint32 *length, ino_t *id)
{
	if (!id || !name || !length)
		return B_BAD_VALUE;

	TRACE(("DirectoryIterator::GetNextEntry: name = %p, length = %" B_PRIu32
		", id = %p, position = %" B_PRIdOFF ", parent length = %" B_PRIu64
		"\n", name, *length, id, fPosition, Parent()->Length()));

	status_t status = B_OK;
	if (fAtBeginning) {
		TRACE(("DirectoryIterator::GetNextEntry: .\n"));
		sprintf(name, ".");
		*length = 2;
		*id = Parent()->Id();
		fAtBeginning = false;
	} else {
		if (uint64(fPosition) >= Parent()->Length()) {
			TRACE(("DirectoryIterator::GetNextEntry: end of dir\n"));
			return B_ENTRY_NOT_FOUND;
		}

		uint8 data[kMaxFileIdSize];
		file_id_descriptor *entry = (file_id_descriptor *)data;

		uint32 block = 0;
		off_t offset = fPosition;

		size_t entryLength = kMaxFileIdSize;
		// First read in the static portion of the file id descriptor,
		// then, based on the information therein, read in the variable
		// length tail portion as well.
		status = Parent()->Read(offset, entry, &entryLength, &block);
		if (!status && entryLength >= sizeof(file_id_descriptor)
			&& entry->tag().init_check(block) == B_OK) {
			PDUMP(entry);
			offset += entry->total_length();

			if (entry->is_parent()) {
				TRACE(("DirectoryIterator::GetNextEntry: ..\n"));
				sprintf(name, "..");
				*length = 3;
			} else {
				UdfString string(entry->id(), entry->id_length());
				TRACE(("DirectoryIterator::GetNextEntry: UfdString id == `%s', "
					"length = %" B_PRIu32 "\n", string.Utf8(),
					string.Utf8Length()));
				DUMP(entry->icb());
				sprintf(name, "%s", string.Utf8());
				*length = string.Utf8Length();
			}
			*id = to_vnode_id(entry->icb());
		}

		if (!status)
			fPosition = offset;
	}

 	return status;
}


/*	\brief Rewinds the iterator to point to the first entry in the directory. */
void
DirectoryIterator::Rewind()
{
	fAtBeginning = true;
	fPosition = 0;
}


//	#pragma mark - Private methods


DirectoryIterator::DirectoryIterator(Icb *parent)
	:
	fAtBeginning(true),
	fParent(parent),
	fPosition(0)
{
}


Icb::Icb(Volume *volume, long_address address)
	:
	fVolume(volume),
	fData(volume),
	fInitStatus(B_NO_INIT),
	fId(to_vnode_id(address)),
	fPartition(address.partition()),
	fFileEntry(&fData),
	fExtendedEntry(&fData),
	fFileCache(NULL),
	fFileMap(NULL)
{
	TRACE(("Icb::Icb: volume = %p, address(block = %" B_PRIu32 ", partition = "
		"%d, length = %" B_PRIu32 ")\n", volume, address.block(),
		address.partition(), address.length()));

	if (volume == NULL) {
		fInitStatus = B_BAD_VALUE;
		return;
	}

	off_t block;
	status_t status = fVolume->MapBlock(address, &block);
	if (status == B_OK) {
		status = fData.SetTo(block);
		if (status == B_OK) {
			icb_header *header = (icb_header *)fData.Block();
			if (header->tag().id() == TAGID_FILE_ENTRY) {
				file_icb_entry *entry = (file_icb_entry *)header;
				PDUMP(entry);
				(void)entry;	// warning death
			} else if (header->tag().id() == TAGID_EXTENDED_FILE_ENTRY) {
				extended_file_icb_entry *entry
					= (extended_file_icb_entry *)header;
				PDUMP(entry);
				(void)entry;	// warning death
			} else {
				PDUMP(header);
			}
			status = header->tag().init_check(address.block());
		}
	}

	if (IsFile()) {
		fFileCache = file_cache_create(fVolume->ID(), fId, Length());
		fFileMap = file_map_create(fVolume->ID(), fId, Length());
	}

	fInitStatus = status;
	TRACE(("Icb::Icb: status = 0x%" B_PRIx32 ", `%s'\n", status,
		strerror(status)));
}


Icb::~Icb()
{
	if (fFileCache != NULL) {
		file_cache_delete(fFileCache);
		file_map_delete(fFileMap);
	}
}


status_t
Icb::GetDirectoryIterator(DirectoryIterator **iterator)
{
	status_t error = iterator ? B_OK : B_BAD_VALUE;

	if (!error) {
		*iterator = new(std::nothrow) DirectoryIterator(this);
		if (*iterator)
		 	fIteratorList.Add(*iterator);
		else
			error = B_NO_MEMORY;
	}

	return error;
}


status_t
Icb::InitCheck()
{
	return fInitStatus;
}


void
Icb::GetAccessTime(struct timespec &timespec) const
{
	timestamp ts;
	if (_Tag().id() == TAGID_EXTENDED_FILE_ENTRY)
		ts = _ExtendedEntry()->access_date_and_time();
	else
		ts = _FileEntry()->access_date_and_time();

	if (decode_time(ts, timespec) != B_OK) {
		decode_time(
			fVolume->PrimaryVolumeDescriptor()->recording_date_and_time(),
			timespec);
	}
}


void
Icb::GetModificationTime(struct timespec &timespec) const
{
	timestamp ts;
	if (_Tag().id() == TAGID_EXTENDED_FILE_ENTRY)
		ts = _ExtendedEntry()->modification_date_and_time();
	else
		ts = _FileEntry()->modification_date_and_time();

	if (decode_time(ts, timespec) != B_OK) {
		decode_time(
			fVolume->PrimaryVolumeDescriptor()->recording_date_and_time(),
			timespec);
	}
}


status_t
Icb::FindBlock(uint32 logicalBlock, off_t &block, bool &recorded)
{
	off_t pos = logicalBlock << fVolume->BlockShift();
	if (uint64(pos) >= Length()) {
		block = -1;
		return B_ERROR;
	}

	DEBUG_INIT_ETC("Icb", ("pos: %" B_PRIdOFF, pos));

	status_t status = B_OK;
	long_address extent;
	bool isEmpty = false;
	recorded = false;

	switch (_IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT:
		{
			TRACE(("Icb::FindBlock: descriptor type -> short\n"));
			AllocationDescriptorList<ShortDescriptorAccessor> list(this,
				ShortDescriptorAccessor(fPartition));
			status = list.FindExtent(pos, &extent, &isEmpty);
			if (status != B_OK) {
				TRACE_ERROR(("Icb::FindBlock: error finding extent for offset "
					"%" B_PRIdOFF ". status = 0x%" B_PRIx32 " `%s'\n", pos, status,
					strerror(status)));
			}
			break;
		}

		case ICB_DESCRIPTOR_TYPE_LONG:
		{
			TRACE(("Icb::FindBlock: descriptor type -> long\n"));
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			status = list.FindExtent(pos, &extent, &isEmpty);
			if (status != B_OK) {
				TRACE_ERROR(("Icb::FindBlock: error finding extent for offset "
					"%" B_PRIdOFF ". status = 0x%" B_PRIx32 " `%s'\n", pos,
					status, strerror(status)));
			}
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EXTENDED:
		{
			TRACE(("Icb::FindBlock: descriptor type -> extended\n"));
//			AllocationDescriptorList<ExtendedDescriptorAccessor> list(this, ExtendedDescriptorAccessor(0));
//			RETURN(_Read(list, pos, buffer, length, block));
			RETURN(B_ERROR);
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EMBEDDED:
		{
			TRACE(("Icb::FindBlock: descriptor type: embedded\n"));
			RETURN(B_ERROR);
			break;
		}

		default:
			TRACE(("Icb::FindBlock: invalid icb descriptor flags! (flags = %d)\n",
				_IcbTag().descriptor_flags()));
			RETURN(B_BAD_VALUE);
			break;
	}

	if (status == B_OK) {
		block = extent.block();
		recorded = extent.type() == EXTENT_TYPE_RECORDED;
		TRACE(("Icb::FindBlock: block %" B_PRIdOFF "\n", block));
	}
	return status;
}


status_t
Icb::Read(off_t pos, void *buffer, size_t *length, uint32 *block)
{
	TRACE(("Icb::Read: pos = %" B_PRIdOFF ", buffer = %p, length = (%p)->%ld\n",
		pos, buffer, length, (length ? *length : 0)));

	DEBUG_INIT_ETC("Icb", ("pos: %" B_PRIdOFF " , length: %ld", pos, *length));

	if (fFileCache != NULL)
		return file_cache_read(fFileCache, NULL, pos, buffer, length);

	if (!buffer || !length || pos < 0)
		return B_BAD_VALUE;

	if (uint64(pos) >= Length()) {
		*length = 0;
		return B_OK;
	}

	switch (_IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT:
		{
			TRACE(("Icb::Read: descriptor type -> short\n"));
			AllocationDescriptorList<ShortDescriptorAccessor> list(this,
				ShortDescriptorAccessor(fPartition));
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}

		case ICB_DESCRIPTOR_TYPE_LONG:
		{
			TRACE(("Icb::Read: descriptor type -> long\n"));
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EXTENDED:
		{
			TRACE(("Icb::Read: descriptor type -> extended\n"));
//			AllocationDescriptorList<ExtendedDescriptorAccessor> list(this, ExtendedDescriptorAccessor(0));
//			RETURN(_Read(list, pos, buffer, length, block));
			RETURN(B_ERROR);
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EMBEDDED:
		{
			TRACE(("Icb::Read: descriptor type: embedded\n"));
			RETURN(B_ERROR);
			break;
		}

		default:
			TRACE(("Icb::Read: invalid icb descriptor flags! (flags = %d)\n",
				_IcbTag().descriptor_flags()));
			RETURN(B_BAD_VALUE);
			break;
	}
}


/*! \brief Does the dirty work of reading using the given DescriptorList object
	to access the allocation descriptors properly.
*/
template <class DescriptorList>
status_t
Icb::_Read(DescriptorList &list, off_t pos, void *_buffer, size_t *length, uint32 *block)
{
	TRACE(("Icb::_Read(): list = %p, pos = %" B_PRIdOFF ", buffer = %p, "
		"length = %ld\n", &list, pos, _buffer, (length ? *length : 0)));

	uint64 bytesLeftInFile = uint64(pos) > Length() ? 0 : Length() - pos;
	size_t bytesLeft = (*length >= bytesLeftInFile) ? bytesLeftInFile : *length;
	size_t bytesRead = 0;

	Volume *volume = GetVolume();
	status_t status = B_OK;
	uint8 *buffer = (uint8 *)_buffer;
	bool isFirstBlock = true;

	while (bytesLeft > 0) {

		TRACE(("Icb::_Read(): pos: %" B_PRIdOFF ", bytesLeft: %ld\n", pos,
			bytesLeft));
		long_address extent;
		bool isEmpty = false;
		status = list.FindExtent(pos, &extent, &isEmpty);
		if (status != B_OK) {
			TRACE_ERROR(("Icb::_Read: error finding extent for offset %"
				B_PRIdOFF ". status = 0x%" B_PRIx32 " `%s'\n", pos, status,
				strerror(status)));
			break;
		}

		TRACE(("Icb::_Read(): found extent for offset %" B_PRIdOFF ": (block: "
			"%" B_PRIu32 ", partition: %d, length: %" B_PRIu32 ", type: %d)\n",
			pos, extent.block(), extent.partition(), extent.length(),
			extent.type()));

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

		size_t readLength = volume->BlockSize() - blockOffset;
		if (bytesLeft < readLength)
			readLength = bytesLeft;
		if (extent.length() < readLength)
			readLength = extent.length();

		TRACE(("Icb::_Read: reading block. offset = %" B_PRIdOFF
			", length: %ld\n", blockOffset, readLength));

		if (isEmpty) {
			TRACE(("Icb::_Read: reading %ld empty bytes as zeros\n",
				readLength));
			memset(buffer, 0, readLength);
		} else {
			off_t diskBlock;
			status = volume->MapBlock(extent, &diskBlock);
			if (status != B_OK) {
				TRACE_ERROR(("Icb::_Read: could not map extent\n"));
				break;
			}

			TRACE(("Icb::_Read: %ld bytes from disk block %" B_PRIdOFF " using"
				" block_cache_get_etc()\n", readLength, diskBlock));
			const uint8 *data;
			status = block_cache_get_etc(volume->BlockCache(),
				diskBlock, 0, readLength, (const void**)&data);
			if (status != B_OK)
				break;
			memcpy(buffer, data + blockOffset, readLength);
			block_cache_put(volume->BlockCache(), diskBlock);
		}

		bytesLeft -= readLength;
		bytesRead += readLength;
		pos += readLength;
		buffer += readLength;
	}

	*length = bytesRead;

	return status;
}


status_t
Icb::GetFileMap(off_t offset, size_t size, file_io_vec *vecs, size_t *count)
{
	switch (_IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT:
		{
			AllocationDescriptorList<ShortDescriptorAccessor> list(this,
				ShortDescriptorAccessor(0));
			return _GetFileMap(list, offset, size, vecs, count);
		}

		case ICB_DESCRIPTOR_TYPE_LONG:
		{
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			return _GetFileMap(list, offset, size, vecs, count);
		}

		case ICB_DESCRIPTOR_TYPE_EXTENDED:
		case ICB_DESCRIPTOR_TYPE_EMBEDDED:
		default:
		{
			// TODO: implement?
			return B_UNSUPPORTED;
		}
	}
}


template<class DescriptorList>
status_t
Icb::_GetFileMap(DescriptorList &list, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *count)
{
	size_t index = 0;
	size_t max = *count;

	while (true) {
		long_address extent;
		bool isEmpty = false;
		status_t status = list.FindExtent(offset, &extent, &isEmpty);
		if (status != B_OK)
			return status;

		switch (extent.type()) {
			case EXTENT_TYPE_RECORDED:
				isEmpty = false;
				break;

			case EXTENT_TYPE_ALLOCATED:
			case EXTENT_TYPE_UNALLOCATED:
				isEmpty = true;
				break;

			default:
				return B_ERROR;
		}

		if (isEmpty)
			vecs[index].offset = -1;
		else {
			off_t diskBlock;
			fVolume->MapBlock(extent, &diskBlock);
			vecs[index].offset = diskBlock << fVolume->BlockShift();
		}

		off_t length = extent.length();
		vecs[index].length = length;

		offset += length;
		size -= length;
		index++;

		if (index >= max || (off_t)size <= vecs[index - 1].length
			|| offset >= (off_t)Length()) {
			*count = index;
			return index >= max ? B_BUFFER_OVERFLOW : B_OK;
		}
	}

	// can never get here
	return B_ERROR;
}


status_t
Icb::Find(const char *filename, ino_t *id)
{
	TRACE(("Icb::Find: filename = `%s', id = %p\n", filename, id));

	if (!filename || !id)
		return B_BAD_VALUE;

	DirectoryIterator *i;
	status_t status = GetDirectoryIterator(&i);
	if (status != B_OK)
		return status;

	ino_t entryId;
	uint32 length = B_FILE_NAME_LENGTH;
	char name[B_FILE_NAME_LENGTH];

	bool foundIt = false;
	while (i->GetNextEntry(name, &length, &entryId) == B_OK) {
		if (strcmp(filename, name) == 0) {
			foundIt = true;
			break;
		}

		// reset overwritten length
		length = B_FILE_NAME_LENGTH;
	}

	if (foundIt)
		*id = entryId;
	else
		status = B_ENTRY_NOT_FOUND;

	return status;
}
