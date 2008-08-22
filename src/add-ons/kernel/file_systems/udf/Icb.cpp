//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered 
//  by the MIT license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#include "Icb.h"

#include "time.h"

#include "AllocationDescriptorList.h"
#include "Utils.h"
#include "Volume.h"


status_t
DirectoryIterator::GetNextEntry(char *name, uint32 *length, ino_t *id)
{
	TRACE(("DirectoryIterator::GetNextEntry: name = %p, length = %p, id = %p\n",
		name, length, id));

	if (!id || !name || !length)
		return B_BAD_VALUE;

	TRACE(("\tfPosition:          %Ld\n", fPosition));
	TRACE(("\tParent()->Length(): %Ld\n", Parent()->Length()));

	status_t status = B_OK;
	if (fAtBeginning) {
		sprintf(name, ".");
		*length = 2;
		*id = Parent()->Id();
		fAtBeginning = false;
	} else {
		if (uint64(fPosition) >= Parent()->Length()) 
			return B_ENTRY_NOT_FOUND;

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
				sprintf(name, "..");
				*length = 3;
			} else {
				UdfString string(entry->id(), entry->id_length());
				TRACE(("\tid == `%s'\n", string.Utf8()));
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


//	#pragma - Private methods


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
	fFileEntry(&fData),
	fExtendedEntry(&fData)
{
	TRACE(("Icb::Icb: volume = %p, address(block = %ld, partition = %d, "
		"length = %ld)\n", volume, address.block(), address.partition(),
		address.length()));

	if (volume == NULL)
		fInitStatus = B_BAD_VALUE;

	off_t block;
	status_t status = fVolume->MapBlock(address, &block);
	if (!status) {
		icb_header *header = (icb_header *)fData.SetTo(block);
		if (header->tag().id() == TAGID_FILE_ENTRY) {
			file_icb_entry *entry = (file_icb_entry *)header;
			PDUMP(entry);
			(void)entry;	// warning death
		} else if (header->tag().id() == TAGID_EXTENDED_FILE_ENTRY) {
			extended_file_icb_entry *entry = (extended_file_icb_entry *)header;
			PDUMP(entry);
			(void)entry;	// warning death
		} else {
			PDUMP(header);
		}
		status = header->tag().init_check(address.block());
	}

	fInitStatus = status;
	TRACE(("Icb::Icb: status = 0x%lx, `%s'\n", status, strerror(status)));
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


time_t
Icb::AccessTime()
{
	return make_time(_FileEntry()->access_date_and_time());
}


time_t
Icb::ModificationTime()
{
	return make_time(_FileEntry()->modification_date_and_time());
}


status_t
Icb::Read(off_t pos, void *buffer, size_t *length, uint32 *block)
{
	TRACE(("Icb::Read: pos = %Ld, buffer = %p, length = (%p)->%ld",
		pos, buffer, length, (length ? *length : 0)));

	if (!buffer || !length || pos < 0)
		return B_BAD_VALUE;

	if (uint64(pos) >= Length()) {
		*length = 0;
		return B_OK;
	}

	switch (_IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT: {
			TRACE(("Icb::Read: descriptor type -> short\n"));
			AllocationDescriptorList<ShortDescriptorAccessor> list(this, ShortDescriptorAccessor(0));
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}
	
		case ICB_DESCRIPTOR_TYPE_LONG: {
			TRACE(("Icb::Read: descriptor type -> long\n"));
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EXTENDED: {
			TRACE(("Icb::Read: descriptor type -> extended\n"));
//			AllocationDescriptorList<ExtendedDescriptorAccessor> list(this, ExtendedDescriptorAccessor(0));
//			RETURN(_Read(list, pos, buffer, length, block));
			RETURN(B_ERROR);
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EMBEDDED: {
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


status_t
Icb::Find(const char *filename, ino_t *id)
{
	TRACE(("Icb::Find: filename = `%s', id = %p\n", filename, id));

	if (!filename || !id)
		RETURN(B_BAD_VALUE);

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
	}

	if (foundIt)
		*id = entryId;
	else
		status = B_ENTRY_NOT_FOUND;

	return status;
}
