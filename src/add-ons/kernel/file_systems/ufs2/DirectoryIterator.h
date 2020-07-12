/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "ufs2.h"


class Inode;

#define	MAXNAMELEN	255

struct dir {
	u_int32_t	next_ino;
	u_int16_t	reclen;
	u_int8_t	type;
	u_int8_t	namlen;
	char		name[MAXNAMELEN + 1];
};

struct dir_info {
	u_int32_t	dot_ino;
	int16_t		dot_reclen;
	u_int8_t	dot_type;
	u_int8_t	dot_namelen;
	char		dot_name[4];
	u_int32_t	dotdot_ino;
	int16_t		dotdot_reclen;
	u_int8_t	dotdot_type;
	u_int8_t	dotdot_namelen;
	char		dotdot_name[4];
};

class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();
			status_t			Lookup(const char* name, size_t length,
										ino_t* id);
			status_t			GetNext(char* name, size_t* _nameLength,
										ino_t* _id);
			status_t			_GetNext(const char* name, size_t* _nameLength,
										ino_t* _id, int64_t* offset);
			dir*				DirectContent() { return direct; }
			dir_info*			DirectInfo() { return direct_info; }

private:
			int 				fCountDir;
			int64				fOffset;
			cluster_t			fCluster;
			Inode* 				fInode;
			dir*				direct;
			dir_info*			direct_info;

};


#endif	// DIRECTORYITERATOR_H
