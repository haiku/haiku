/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H


#include "Inode.h"


struct attr_cookie {
	char	name[B_ATTR_NAME_LENGTH];
	uint32	type;
	int		open_mode;
	bool	create;
};


// This class will act as an interface between all types of attributes for xfs
class Attribute {
public:

			virtual				~Attribute()									=	0;

			virtual status_t	Open(const char* name, int openMode,
									attr_cookie** _cookie)						=	0;

			virtual status_t	Stat(attr_cookie* cookie, struct stat& stat)	=	0;

			virtual status_t	Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length)				=	0;

			virtual status_t	GetNext(char* name, size_t* nameLength)			=	0;

			virtual status_t	Lookup(const char* name, size_t* nameLength)	=	0;

			static Attribute*	Init(Inode* inode);
};

#endif