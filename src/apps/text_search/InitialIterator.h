/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef INITIAL_ITERATOR_H
#define INITIAL_ITERATOR_H

#include <List.h>
#include <Message.h>

#include "FileIterator.h"

class BEntry;
class BDirectory;
class Model;

// TODO: split into Folder and MessageFileIterator (_GetTopEntry)
// This code either has an original folder to start iterating from,
// or it uses the refs in the message that contains the selected poses
// from the Tracker window that invoked us. But I think if folders are
// among those poses, it will then use the same code for examining sub
// folders.

// Provides an interface to retrieve the next file that should be grepped
// for the search string.
class InitialIterator : public FileIterator {
public:
								InitialIterator(const Model* model);
	virtual						~InitialIterator();

	virtual	bool				IsValid() const;
	virtual	bool				GetNextName(char* buffer);
	virtual	bool				NotifyNegatives() const;

	// Looks for the next entry in the top-level dir.
			bool				GetTopEntry(BEntry& entry);

	// Should this subfolder be followed?
			bool				FollowSubdir(BEntry& entry) const;

private:
	// Looks for the next entry.
			bool				_GetNextEntry(BEntry& entry);

	// Looks for the next entry in a subdir.
			bool				_GetSubEntry(BEntry& entry);

	// Determines whether we can add a subdir.
			void				_ExamineSubdir(BEntry& entry);

private:
	// Contains pointers to BDirectory objects.
			BList				fDirectories;

	// The directory we are currently looking at.
			BDirectory*			fCurrentDir;

	// The ref number we are currently looking at.
			int32				fCurrentRef;

	// The current settings
			BMessage			fSelectedFiles;

			bool				fRecurseDirs : 1;
			bool				fRecurseLinks : 1;
			bool				fSkipDotDirs : 1;
			bool				fTextOnly : 1;
};

#endif // INITIAL_ITERATOR_H
