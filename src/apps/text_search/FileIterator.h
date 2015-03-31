/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FILE_ITERATOR_H
#define FILE_ITERATOR_H

class BEntry;

// Provides an interface to retrieve the next file that should be grepped
// for the search string.
class FileIterator {
public:
								FileIterator();
	virtual						~FileIterator();

	virtual	bool				IsValid() const = 0;

	// Returns the full path name of the next file.
	virtual	bool				GetNextName(char* buffer) = 0;

	// Tells the Grepper whether the targets wants to know about negative hits.
	virtual	bool				NotifyNegatives() const = 0;

protected:
	// Determines whether we can grep a file.
			bool				_ExamineFile(BEntry& entry, char* buffer,
									bool textFilesOnly);
};

#endif // FILE_ITERATOR_H
