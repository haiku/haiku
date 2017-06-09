/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef ATTRIBUTEITERATOR_H
#define ATTRIBUTEITERATOR_H


#include "BTree.h"
#include "Inode.h"


class AttributeIterator {
public:
								AttributeIterator(Inode* inode);
								~AttributeIterator();

			status_t			InitCheck();

			status_t			GetNext(char* name, size_t* _nameLength);
			status_t			Rewind();
private:
			uint64				fOffset;
			Inode* 				fInode;
			TreeIterator*		fIterator;
};


#endif	// ATTRIBUTEITERATOR_H
