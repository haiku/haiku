/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef CHANGES_ITERATOR_H
#define CHANGES_ITERATOR_H

#include <HashMap.h>
#include <HashString.h>

#include "FileIterator.h"

class BEntry;
class BDirectory;
class Model;

class ChangesIterator : public FileIterator {
public:
								ChangesIterator(const Model* model);
	virtual						~ChangesIterator();

	virtual	bool				IsValid() const;
	virtual	bool				GetNextName(char* buffer);
	virtual	bool				NotifyNegatives() const;

public:
			void				EntryAdded(const char* path);
			void				EntryRemoved(const char* path);
			void				EntryChanged(const char* path);

			bool				IsEmpty() const;
			void				PrintToStream() const;

private:
	typedef HashMap<HashString, uint32> PathMap;
	enum {
		ENTRY_ADDED	= 0,
		ENTRY_REMOVED,
		ENTRY_CHANGED
	};

			PathMap				fPathMap;
			int32				fIteratorIndex;

			bool				fRecurseDirs : 1;
			bool				fRecurseLinks : 1;
			bool				fSkipDotDirs : 1;
			bool				fTextOnly : 1;
};

#endif // CHANGES_ITERATOR_H
