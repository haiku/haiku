/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PATH_BLACKLIST_H
#define KERNEL_BOOT_PATH_BLACKLIST_H


#include <string.h>

#include <util/SinglyLinkedList.h>


class BlacklistedPath : public SinglyLinkedListLinkImpl<BlacklistedPath> {
public:
								BlacklistedPath();
								~BlacklistedPath();

			bool				SetTo(const char* path);

			bool				Append(const char* component);

			const char*			Path() const
									{ return fPath != NULL ? fPath : ""; }

			size_t				Length() const
									{ return fLength; }

			bool				operator==(const char* path) const
									{ return strcmp(Path(), path) == 0; }

private:
			bool				_Resize(size_t length, bool keepData);

private:
			char*				fPath;
			size_t				fLength;
			size_t				fCapacity;
};


class PathBlacklist {
public:
			typedef SinglyLinkedList<BlacklistedPath>::Iterator Iterator;

public:
								PathBlacklist();
								~PathBlacklist();

			bool				Add(const char* path);
			void				Remove(const char* path);
			bool				Contains(const char* path) const;
			void				MakeEmpty();

			bool				IsEmpty() const
									{ return fPaths.IsEmpty(); }

			Iterator			GetIterator() const
									{ return fPaths.GetIterator(); }

private:
			BlacklistedPath*	_FindPath(const char* path) const;

private:
			typedef SinglyLinkedList<BlacklistedPath> PathList;

private:
			PathList			fPaths;
};


#endif	// KERNEL_BOOT_PATH_BLACKLIST_H
