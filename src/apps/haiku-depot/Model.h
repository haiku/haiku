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

private:
			BString				fSearchTerms;

			DepotInfoList		fDepots;
};


#endif // PACKAGE_INFO_H
