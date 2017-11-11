/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PACKAGE_DATA_UPDATE_PROCESS_H
#define PACKAGE_DATA_UPDATE_PROCESS_H


#include "AbstractServerProcess.h"

#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class PkgDataUpdateProcess : public AbstractServerProcess {
public:
								PkgDataUpdateProcess(
									const BPath& localFilePath,
									BLocker& lock,
									BString naturalLanguageCode,
									BString repositorySourceCode,
									const PackageList& packages,
									const CategoryList& categories);
	virtual						~PkgDataUpdateProcess();

			status_t			Run();

protected:
			void				GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;
			const char*			LoggingName() const;

private:
			status_t			PopulateDataToDepots();

			BPath				fLocalFilePath;
			BString				fNaturalLanguageCode;
			BString				fRepositorySourceCode;
	const	PackageList&		fPackages;
	const 	CategoryList&		fCategories;
			BLocker&			fLock;

};

#endif // PACKAGE_DATA_UPDATE_PROCESS_H
