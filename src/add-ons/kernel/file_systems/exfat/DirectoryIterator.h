/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "CachedBlock.h"
#include "exfat.h"

class Inode;

class EntryVisitor {
public:
								EntryVisitor() {};
		virtual					~EntryVisitor() {};
		virtual bool			VisitBitmap(struct exfat_entry*)
									{ return false; }
		virtual bool			VisitUppercase(struct exfat_entry*)
									{ return false; }
		virtual bool			VisitLabel(struct exfat_entry*)
									{ return false; }
		virtual bool			VisitFilename(struct exfat_entry*)
									{ return false; }
		virtual bool			VisitFile(struct exfat_entry*)
									{ return false; }
		virtual bool			VisitFileInfo(struct exfat_entry*)
									{ return false; }
};


class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();

			status_t			GetNext(char* name, size_t* _nameLength,
									ino_t* _id, EntryVisitor* visitor = NULL);
			status_t			Lookup(const char* name, size_t nameLength,
									ino_t* _id);
			status_t			LookupEntry(EntryVisitor* visitor);
			status_t			Rewind();

			void				Iterate(EntryVisitor &visitor);
private:
			status_t			_GetNext(uchar* unicodename,
									size_t* _nameLength, ino_t* _id,
									EntryVisitor* visitor = NULL);
			status_t			_NextEntry();

			int64				fOffset;
			cluster_t			fCluster;
			Inode* 				fInode;
			CachedBlock			fBlock;
			struct exfat_entry*	fCurrent;
};


#endif	// DIRECTORYITERATOR_H
