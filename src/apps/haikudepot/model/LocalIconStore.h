/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef LOCAL_ICON_STORE_H
#define LOCAL_ICON_STORE_H

#include <String.h>
#include <File.h>
#include <Path.h>

#include "PackageInfo.h"


class LocalIconStore {
public:
								LocalIconStore();
	virtual						~LocalIconStore();
			status_t			TryFindIconPath(const BString& pkgName,
									BPath& path) const;
			void				UpdateFromServerIfNecessary() const;

private:
			bool				_HasIconStoragePath() const;
			status_t			_EnsureIconStoragePath(BPath& path) const;
			status_t			_IdentifyBestIconFileAtDirectory(
									const BPath& directory,
									BPath& bestIconPath) const;

			BPath				fIconStoragePath;

};


#endif // LOCAL_ICON_STORE_H
