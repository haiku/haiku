/*
 * Copyright 2010, François Revol, <revol@free.fr>.
 * This file may be used under the terms of the MIT License.
 */


#include "AttributeIterator.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


AttributeIterator::AttributeIterator(Inode* inode)
	:
	fInode(inode),
	fOffset(0)
{
	TRACE("AttributeIterator::%s()\n", __FUNCTION__);
	ext2_xattr_header header;
	size_t length = sizeof(header);
	if (fInode->AttributeBlockReadAt(fOffset, (uint8*)&header, &length) == B_OK
		&& length == sizeof(header)) {
		//header.Dump();
		if (header.IsValid()) {
			fOffset = sizeof(header);
			TRACE("AttributeIterator::%s: header valid\n", __FUNCTION__);
		}
	}
}


AttributeIterator::~AttributeIterator()
{
}


status_t
AttributeIterator::GetNext(char* name, size_t* _nameLength, ext2_xattr_entry *_entry)
{
	TRACE("AttributeIterator::%s()\n", __FUNCTION__);
	ext2_xattr_entry internalEntry;
	if (_entry == NULL)
		_entry = &internalEntry;

	TRACE("AttributeIterator::%s: fOffset %Ld e.MinSize %d BlockSize %d\n",
		__FUNCTION__, fOffset, _entry->MinimumSize(), fInode->GetVolume()->BlockSize());

	if (fOffset + _entry->MinimumSize() >= fInode->GetVolume()->BlockSize())
		return B_ENTRY_NOT_FOUND;

	size_t length = ext2_xattr_entry::MinimumSize();
	status_t status = fInode->AttributeBlockReadAt(fOffset, (uint8*)_entry, &length);
	TRACE("AttributeIterator::%s:AttributeBlockReadAt(%Ld): 0x%08lx len %d\n",
		__FUNCTION__, fOffset, status, length);
	//_entry->Dump();

	if (status != B_OK)
		return status;
	if (length < ext2_xattr_entry::MinimumSize())
		return B_ENTRY_NOT_FOUND;
	TRACE("AttributeIterator::%s:AttributeBlockReadAt(%Ld): namelen %d %cvalid\n",
		__FUNCTION__, fOffset, _entry->NameLength(), _entry->IsValid() ? ' ' : '!');
	if (!_entry->IsValid())
		return B_BAD_DATA;

	TRACE("xattr block offset %Ld: entry ino %Lu, length %u, name length %u,"
		" index %d\n",
		fOffset, fInode->ID(), _entry->Length(), _entry->NameLength(), 
		_entry->NameIndex());

	// read name

	length = _entry->NameLength();
	status = fInode->AttributeBlockReadAt(fOffset + ext2_xattr_entry::MinimumSize(),
		(uint8*)_entry->name, &length);
	TRACE("AttributeIterator::%s:AttributeBlockReadAt(%Ld): 0x%08lx len %d\n",
		__FUNCTION__, fOffset + ext2_xattr_entry::MinimumSize(),
		status, length);

	if (length < 1)
		status = ENOENT;

	if (status == B_OK) {
		const char *indexNames[] = { "0", "user" };

		size_t l;

		TRACE("AttributeIterator::%s: len %d nlen %d\n",
			__FUNCTION__, length, *_nameLength);
		if (_entry->NameIndex() < ((sizeof(indexNames) / sizeof(indexNames[0]))))
			l = snprintf(name, *_nameLength, "%s.%s.%.*s",
				"linux", indexNames[_entry->NameIndex()], length, _entry->name);
		else
			l = snprintf(name, *_nameLength, "%s.%d.%.*s",
				"linux", _entry->NameIndex(), length, _entry->name);
		if (l < 1 || l > *_nameLength - 1)
			status = ENOBUFS;
		else {
			*_nameLength = l + 1;
			name[l] = '\0';
		}

		TRACE("AttributeIterator::%s: name %.*s len %d nlen %d\n",
			__FUNCTION__, *_nameLength, name, length, *_nameLength);

		fOffset += _entry->Length();
	}

	return status;
}


status_t
AttributeIterator::Rewind()
{
	TRACE("AttributeIterator::%s()\n", __FUNCTION__);
	fOffset = sizeof(ext2_xattr_header);
	return B_OK;
}


status_t
AttributeIterator::Find(const char* name, ext2_xattr_entry *_entry)
{
	char buffer[EXT2_XATTR_NAME_LENGTH];
	size_t length = sizeof(buffer);
	status_t err;
	while ((err = GetNext(buffer, &length, _entry)) == B_OK) {
		TRACE("AttributeIterator::%s: next is '%.*s'\n", __FUNCTION__, length > 0 ? length : 0, buffer);
		_entry->Dump();
		if (strncmp(name, buffer, length) == 0)
			return B_OK;
		length = sizeof(buffer);
	}
	return ENOENT;
}


void
AttributeIterator::PrefixForNameIndex(uint8 index, char* prefix, size_t *length)
{
	const char *indexNames[] = { "user" };
	if (index < (sizeof(indexNames) / sizeof(indexNames[0])))
		*length = strlcpy(prefix, indexNames[index], *length);
	else {
		*length = strlcpy(prefix, "unknown", *length);
	}
}


status_t
AttributeIterator::NameBufferAppend(const char* str, char *&buffer, size_t *length)
{
	size_t done;
	done = strlcpy(buffer, str, *length);
	*length = done > 0 ? done - 1 : 0;
	buffer += done;
	return B_OK;
}

