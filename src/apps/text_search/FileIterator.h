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
#ifndef FILE_ITERATOR_H
#define FILE_ITERATOR_H

#include <List.h>

class BEntry;
class BDirectory;
class Model;

// TODO: split into Folder and MessageFileIterator (_GetTopEntry)

// Provides an interface to retrieve the next file that should be grepped
// for the search string.
class FileIterator {
public:
								FileIterator(Model* model);
	virtual						~FileIterator();

	virtual	bool				IsValid() const;

	// Returns the full path name of the next file.
	virtual	bool				GetNextName(char* buffer);
	
private:
	// Looks for the next entry.
			bool				_GetNextEntry(BEntry& entry);

	// Looks for the next entry in the top-level dir.
			bool				_GetTopEntry(BEntry& entry);
	
	// Looks for the next entry in a subdir.
			bool				_GetSubEntry(BEntry& entry);
	
	// Determines whether we can add a subdir.
			void				_ExamineSubdir(BEntry& entry);
	
	// Determines whether we can grep a file.
			bool				_ExamineFile(BEntry& entry, char* buffer);

private:
	// Contains pointers to BDirectory objects.
			BList				fDirectories;
	
	// The directory we are currently looking at.
			BDirectory*			fCurrentDir;
	
	// The ref number we are currently looking at.
			int32				fCurrentRef;

	// The directory or files to grep on.
			Model*				fModel;
};

#endif // FILE_ITERATOR_H
