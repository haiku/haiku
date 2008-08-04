/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
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
