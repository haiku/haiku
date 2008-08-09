/*
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UDF_DIRECTORY_ITERATOR_H
#define _UDF_DIRECTORY_ITERATOR_H

/*! \file DirectoryIterator.h */

#include "UdfDebug.h"

#include <util/kernel_cpp.h>

class Icb;

class DirectoryIterator {
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

#endif	// _UDF_DIRECTORY_ITERATOR_H
