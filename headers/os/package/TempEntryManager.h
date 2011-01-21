/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__TEMP_ENTRY_MANAGER_H_
#define _HAIKU__PACKAGE__TEMP_ENTRY_MANAGER_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>
#include <SupportDefs.h>


namespace Haiku {

namespace Package {


class TempEntryManager {
public:
								TempEntryManager();
								~TempEntryManager();

			void				SetBaseDirectory(const BDirectory& baseDir);

			BEntry				Create(const BString& baseName = kDefaultName);

private:
	static	const BString		kDefaultName;

private:
			BDirectory			fBaseDirectory;
			vint32				fNextNumber;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__TEMP_ENTRY_MANAGER_H_
