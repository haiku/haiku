/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_ATTRIBUTE_H
#define NODE_ATTRIBUTE_H


#include "LeafAttribute.h"
#include "Node.h"


class NodeAttribute : public Attribute {
public:
								NodeAttribute(Inode* inode);
								~NodeAttribute();

			status_t			Init();

			status_t			Stat(attr_cookie* cookie, struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

			status_t			Open(const char* name, int openMode, attr_cookie** _cookie);

			status_t			GetNext(char* name, size_t* nameLength);

			status_t			Lookup(const char* name, size_t* nameLength);
private:
			status_t			_FillMapEntry(xfs_extnum_t num);

			status_t			_FillBuffer(char* buffer, xfs_fsblock_t block);

			xfs_fsblock_t		_LogicalToFileSystemBlock(uint32 LogicalBlock);

			Inode*				fInode;
			const char*			fName;
			ExtentMapEntry*		fMap;
			char*				fLeafBuffer;
			char*				fNodeBuffer;
			uint16				fLastEntryOffset;
			uint16				fLastNodeOffset;
			uint8				fNodeFlag;
			AttrLeafNameLocal*	fLocalEntry;
			AttrLeafNameRemote*	fRemoteEntry;
};

#endif