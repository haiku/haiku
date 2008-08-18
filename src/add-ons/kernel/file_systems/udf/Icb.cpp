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
	DEBUG_INIT_ETC("DirectoryIterator",
	               ("name: %p, length: %p, id: %p", name, length, id));

	if (!id || !name || !length)
		return B_BAD_VALUE;

	PRINT(("fPosition:          %Ld\n", fPosition));
	PRINT(("Parent()->Length(): %Ld\n", Parent()->Length()));

	status_t error = B_OK;
	if (fAtBeginning) {
		sprintf(name, ".");
		*length = 2;
		*id = Parent()->Id();
		fAtBeginning = false;
	} else {

		if (uint64(fPosition) >= Parent()->Length()) 
			RETURN(B_ENTRY_NOT_FOUND);

		uint8 data[kMaxFileIdSize];
		file_id_descriptor *entry = reinterpret_cast<file_id_descriptor*>(data);

		uint32 block = 0;
		off_t offset = fPosition;

		size_t entryLength = kMaxFileIdSize;
		// First read in the static portion of the file id descriptor,
		// then, based on the information therein, read in the variable
		// length tail portion as well.
		error = Parent()->Read(offset, entry, &entryLength, &block);
		if (!error && entryLength >= sizeof(file_id_descriptor)
			&& entry->tag().init_check(block) == B_OK) {
			PDUMP(entry);
			offset += entry->total_length();

			if (entry->is_parent()) {
				sprintf(name, "..");
				*length = 3;
			} else {
				UdfString string(entry->id(), entry->id_length());
				PRINT(("id == `%s'\n", string.Utf8()));
				DUMP(entry->icb());
				sprintf(name, "%s", string.Utf8());
				*length = string.Utf8Length();
			}
			*id = to_vnode_id(entry->icb());
		}

		if (!error)
			fPosition = offset;
	}

 	RETURN(error);
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
	DEBUG_INIT_ETC("Icb", ("volume: %p, address(block: %ld, "
	               "partition: %d, length: %ld)", volume, address.block(),
	               address.partition(), address.length()));  
	status_t error = volume ? B_OK : B_BAD_VALUE;
	if (!error) {
		off_t block;
		error = fVolume->MapBlock(address, &block);
		if (!error) {
			icb_header *header = (icb_header *)fData.SetTo(block);
			if (header->tag().id() == TAGID_FILE_ENTRY) {
				file_icb_entry *entry = reinterpret_cast<file_icb_entry*>(header);
				PDUMP(entry);
				(void)entry;	// warning death
			} else if (header->tag().id() == TAGID_EXTENDED_FILE_ENTRY) {
				extended_file_icb_entry *entry = reinterpret_cast<extended_file_icb_entry*>(header);
				PDUMP(entry);
				(void)entry;	// warning death
			} else {
				PDUMP(header);
			}
			error = header->tag().init_check(address.block());
		}
	}		
	fInitStatus = error;
	PRINT(("result: 0x%lx, `%s'\n", error, strerror(error)));
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
	DEBUG_INIT_ETC("Icb",
	               ("pos: %Ld, buffer: %p, length: (%p)->%ld", pos, buffer, length, (length ? *length : 0)));

	if (!buffer || !length || pos < 0)
		RETURN(B_BAD_VALUE);		
	
	if (uint64(pos) >= Length()) {
		*length = 0;
		return B_OK;
	}

	switch (_IcbTag().descriptor_flags()) {
		case ICB_DESCRIPTOR_TYPE_SHORT: {
			PRINT(("descriptor type: short\n"));
			AllocationDescriptorList<ShortDescriptorAccessor> list(this, ShortDescriptorAccessor(0));
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}
	
		case ICB_DESCRIPTOR_TYPE_LONG: {
			PRINT(("descriptor type: long\n"));
			AllocationDescriptorList<LongDescriptorAccessor> list(this);
			RETURN(_Read(list, pos, buffer, length, block));
			break;
		}

		case ICB_DESCRIPTOR_TYPE_EXTENDED: {
			PRINT(("descriptor type: extended\n"));
//			AllocationDescriptorList<ExtendedDescriptorAccessor> list(this, ExtendedDescriptorAccessor(0));
//			RETURN(_Read(list, pos, buffer, length, block));
			RETURN(B_ERROR);
			break;
		}
		
		case ICB_DESCRIPTOR_TYPE_EMBEDDED: {
			PRINT(("descriptor type: embedded\n"));
			RETURN(B_ERROR);
			break;
		}
		
		default:
			PRINT(("Invalid icb descriptor flags! (flags = %d)\n", _IcbTag().descriptor_flags()));
			RETURN(B_BAD_VALUE);
			break;		
	}
}


status_t
Icb::Find(const char *filename, ino_t *id)
{
	DEBUG_INIT_ETC("Icb",
	               ("filename: `%s', id: %p", filename, id));
	               
	if (!filename || !id)
		RETURN(B_BAD_VALUE);
		
	DirectoryIterator *i;
	status_t error = GetDirectoryIterator(&i);
	if (!error) {
		ino_t entryId;
		uint32 length = B_FILE_NAME_LENGTH;
		char name[B_FILE_NAME_LENGTH];
		
		bool foundIt = false;
		while (i->GetNextEntry(name, &length, &entryId) == B_OK)
		{
	    	if (strcmp(filename, name) == 0) {
	    		foundIt = true;
	    		break;
	    	}	
		}
		
		if (foundIt) {
			*id = entryId;
		} else {
			error = B_ENTRY_NOT_FOUND;
		}
	}
	
	RETURN(error);
}
