/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Model.h"

#include <stdio.h>


Model::Model()
	:
	fSearchTerms(),
	fDepots()
{
}


PackageInfoList
Model::CreatePackageList() const
{
	// TODO: Allow to restrict depot, filter by search terms, ...

	// Return all packages from all depots.
	PackageInfoList resultList;

	for (int32 i = 0; i < fDepots.CountItems(); i++) {
		const PackageInfoList& packageList
			= fDepots.ItemAtFast(i).PackageList();

		for (int32 j = 0; j < packageList.CountItems(); j++) {
			resultList.Add(packageList.ItemAtFast(j));
		}
	}

	return resultList;
}


bool
Model::AddDepot(const DepotInfo& depot)
{
	return fDepots.Add(depot);
}

