/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REPOSITORY_H_
#define _REPOSITORY_H_


#include <Archivable.h>


class BMessage;


class BRepository {
public:
								BRepository(const char* url);

public:
	static	BArchivable*		Instantiate(BMessage* archive);
};


#endif // _REPOSITORY_H_
