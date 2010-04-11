/*
 * Copyright 2010, François Revol, <revol@free.fr>.
 * This file may be used under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_ITERATOR_H
#define ATTRIBUTE_ITERATOR_H


#include <SupportDefs.h>

#include "ext2.h"

class Inode;

class AttributeIterator {
public:
						AttributeIterator(Inode* inode);
						~AttributeIterator();

			status_t	InitCheck() const { return (fOffset
							>= sizeof(ext2_xattr_header)) ? B_OK : B_ENTRY_NOT_FOUND; }

			status_t	GetNext(char* name, size_t* _nameLength, ext2_xattr_entry *_entry=NULL);

			status_t	Rewind();

			status_t	Find(const char* name, ext2_xattr_entry *_entry);

private:
						AttributeIterator(const AttributeIterator&);
						AttributeIterator &operator=(const AttributeIterator&);
							// no implementation

			void		PrefixForNameIndex(uint8 index, char* prefix, size_t *length);
			status_t	NameBufferAppend(const char* str, char *&buffer, size_t *length);

private:
	Inode*				fInode;
	off_t				fOffset;
};

#endif	// ATTRIBUTE_ITERATOR_H
