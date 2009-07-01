/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCATABLE_FILE_H
#define LOCATABLE_FILE_H

#include <ObjectList.h>

#include "LocatableEntry.h"


class LocatableFile : public LocatableEntry {
public:
	class Listener;

public:
								LocatableFile(LocatableEntryOwner* owner,
									LocatableDirectory* directory,
									const BString& name);
								~LocatableFile();

	virtual	const char*			Name() const;
			void				GetPath(BString& _path) const;

			// mutable (requires/does locking)
	virtual	bool				GetLocatedPath(BString& _path) const;
	virtual	void				SetLocatedPath(const BString& path,
									bool implicit);

			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef BObjectList<Listener> ListenerList;

private:
			void				_NotifyListeners();

private:
			BString				fName;
			BString				fLocatedPath;
			ListenerList		fListeners;
};


class LocatableFile::Listener {
public:
	virtual						~Listener();

	virtual	void				LocatableFileChanged(LocatableFile* file) = 0;
									// called with lock held
};


#endif	// LOCATABLE_FILE_H
