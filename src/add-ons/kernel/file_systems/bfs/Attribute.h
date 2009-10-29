/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
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


class Attribute {
public:
							Attribute(Inode* inode);
							Attribute(Inode* inode, attr_cookie* cookie);
							~Attribute();

			status_t		InitCheck();
			status_t		CheckAccess(const char* name, int openMode);

			status_t		Get(const char* name);
			void			Put();

			status_t		Create(const char* name, type_code type,
								int openMode, attr_cookie** _cookie);
			status_t		Open(const char* name, int openMode,
								attr_cookie** _cookie);

			status_t		Stat(struct stat& stat);

			status_t		Read(attr_cookie* cookie, off_t pos, uint8* buffer,
								size_t* _length);
			status_t		Write(Transaction& transaction, attr_cookie* cookie,
								off_t pos, const uint8* buffer, size_t* _length,
								bool* _created);

private:
			status_t		_Truncate();

			NodeGetter		fNodeGetter;
			Inode*			fInode;
			small_data*		fSmall;
			Inode*			fAttribute;
			const char*		fName;
};

#endif	// ATTRIBUTE_H
