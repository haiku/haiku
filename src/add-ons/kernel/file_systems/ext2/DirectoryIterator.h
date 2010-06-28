/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORY_ITERATOR_H
#define DIRECTORY_ITERATOR_H


#include <SupportDefs.h>


class Inode;

class DirectoryIterator {
public:
						DirectoryIterator(Inode* inode);
	virtual				~DirectoryIterator();

	virtual	status_t	GetNext(char* name, size_t* _nameLength, ino_t* id);

	virtual	status_t	Rewind();

private:
						DirectoryIterator(const DirectoryIterator&);
						DirectoryIterator &operator=(const DirectoryIterator&);
							// no implementation

protected:
	Inode*				fInode;
	off_t				fOffset;
};

#endif	// DIRECTORY_ITERATOR_H
