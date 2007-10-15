/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYM_LINK_H
#define _SYM_LINK_H


#include <Node.h>
#include <StorageDefs.h>

class BDirectory;
class BPath;


/*!
	\class BSymLink
	\brief A symbolic link in the filesystem
	
	Provides an interface for manipulating symbolic links.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/
class BSymLink : public BNode {
public:
	BSymLink();
	BSymLink(const BSymLink &link);
	BSymLink(const entry_ref *ref);
	BSymLink(const BEntry *entry);
	BSymLink(const char *path);
	BSymLink(const BDirectory *dir, const char *path);
	virtual ~BSymLink();

	ssize_t ReadLink(char *buf, size_t size);

	ssize_t MakeLinkedPath(const char *dirPath, BPath *path);
	ssize_t MakeLinkedPath(const BDirectory *dir, BPath *path);

	bool IsAbsolute();

private:
	virtual void _MissingSymLink1();
	virtual void _MissingSymLink2();
	virtual void _MissingSymLink3();
	virtual void _MissingSymLink4();
	virtual void _MissingSymLink5();
	virtual void _MissingSymLink6();

	uint32 _reservedData[4];
	BEntry *fSecretEntry;

private:
	int get_fd() const;
};

#endif	// _SYM_LINK_H
