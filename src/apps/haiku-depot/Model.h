/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H


#include "PackageInfo.h"


class Model {
public:
								Model();

			// !Returns new PackageInfoList from current parameters
			PackageInfoList		CreatePackageList() const;

			bool				AddDepot(const DepotInfo& depot);

			// Access to global categories
			const CategoryRef&	CategoryAudio() const
									{ return fCategoryAudio; }
			const CategoryRef&	CategoryVideo() const
									{ return fCategoryVideo; }
			const CategoryRef&	CategoryGraphics() const
									{ return fCategoryGraphics; }
			const CategoryRef&	CategoryProductivity() const
									{ return fCategoryProductivity; }
			const CategoryRef&	CategoryDevelopment() const
									{ return fCategoryDevelopment; }
			const CategoryRef&	CategoryCommandLine() const
									{ return fCategoryCommandLine; }

private:
			BString				fSearchTerms;

			DepotInfoList		fDepots;

			CategoryRef			fCategoryAudio;
			CategoryRef			fCategoryVideo;
			CategoryRef			fCategoryGraphics;
			CategoryRef			fCategoryProductivity;
			CategoryRef			fCategoryDevelopment;
			CategoryRef			fCategoryCommandLine;
			// TODO: More categories
};


#endif // PACKAGE_INFO_H
