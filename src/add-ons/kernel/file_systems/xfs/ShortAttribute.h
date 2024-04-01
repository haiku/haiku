/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef SHORT_ATTRIBUTE_H
#define SHORT_ATTRIBUTE_H


#include "Attribute.h"
#include "Inode.h"


class ShortAttribute : public Attribute {
public:
			struct AShortFormEntry {
			public:
				uint8				namelen;
				uint8				valuelen;
				uint8				flags;
				uint8				nameval[];
			};

			struct AShortFormHeader {
				uint16				totsize;
				uint8				count;
				uint8				padding;
			};
								ShortAttribute(Inode* inode);
								~ShortAttribute();

			status_t			Stat(attr_cookie* cookie, struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

			status_t			Open(const char* name, int openMode, attr_cookie** _cookie);

			status_t			GetNext(char* name, size_t* nameLength);

			status_t			Lookup(const char* name, size_t* nameLength);
private:
			void				SwapEndian();
private:
			uint32				_DataLength(AShortFormEntry* entry);

			AShortFormEntry*	_FirstEntry();

			Inode*				fInode;
			const char*			fName;
			AShortFormHeader*	fHeader;
			AShortFormEntry*	fEntry;
			uint16				fLastEntryOffset;
};

#endif