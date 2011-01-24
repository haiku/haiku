/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__TEMPFILE_MANAGER_H_
#define _HAIKU__PACKAGE__PRIVATE__TEMPFILE_MANAGER_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>
#include <SupportDefs.h>


namespace Haiku {

namespace Package {

namespace Private {


class TempfileManager {
public:
								TempfileManager();
								~TempfileManager();

			void				SetBaseDirectory(const BDirectory& baseDir);

			BEntry				Create(const BString& baseName = kDefaultName);

private:
	static	const BString		kDefaultName;

private:
			BDirectory			fBaseDirectory;
			vint32				fNextNumber;
};


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__TEMPFILE_MANAGER_H_
