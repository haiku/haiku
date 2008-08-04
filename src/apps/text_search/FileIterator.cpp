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

#include "FileIterator.h"

#include <string.h>

#include <Entry.h>
#include <NodeInfo.h>
#include <Path.h>


FileIterator::FileIterator() 
{
}


FileIterator::~FileIterator()
{
}


bool
FileIterator::_ExamineFile(BEntry& entry, char* buffer, bool textFilesOnly)
{
	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return false;

	strcpy(buffer, path.Path());

	if (!textFilesOnly)
		return true;

	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char mimeTypeString[B_MIME_TYPE_LENGTH];

	if (nodeInfo.GetType(mimeTypeString) != B_OK)
		return false;

	BMimeType mimeType(mimeTypeString);
	BMimeType superType;

	if (mimeType.GetSupertype(&superType) == B_OK) {
		if (strcmp("text", superType.Type()) == 0
			|| strcmp("message", superType.Type()) == 0) {
			return true;
		}
	}

	return false;
}

