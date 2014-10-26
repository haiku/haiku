/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FEATURED_PACKAGES_VIEW_H
#define FEATURED_PACKAGES_VIEW_H


#include <View.h>

#include "PackageInfo.h"
#include "PackageInfoListener.h"


class BGroupLayout;


class FeaturedPackagesView : public BView {
public:
								FeaturedPackagesView();
	virtual						~FeaturedPackagesView();

			void				AddPackage(const PackageInfoRef& package);
			void				Clear();

private:
			BGroupLayout*		fPackageListLayout;

};


#endif // FEATURED_PACKAGES_VIEW_H
