/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCATABLE_DIRECTORY_H
#define LOCATABLE_DIRECTORY_H

#include "LocatableEntry.h"


class LocatableDirectory : public LocatableEntry {
public:
								LocatableDirectory(LocatableEntryOwner* owner,
									LocatableDirectory* parent,
									const BString& path);
								~LocatableDirectory();

	virtual	const char*			Name() const;
			const char*			Path() const;
			void				GetPath(BString& _path) const;

			// mutable (requires locking)
	virtual	bool				GetLocatedPath(BString& _path) const;
	virtual	void				SetLocatedPath(const BString& path,
									bool implicit);

			void				AddEntry(LocatableEntry* entry);
			void				RemoveEntry(LocatableEntry* entry);
			const LocatableEntryList& Entries() const	{ return fEntries; }

private:
			BString				fPath;
			BString				fLocatedPath;
			LocatableEntryList	fEntries;
};


#endif	// LOCATABLE_DIRECTORY_H
