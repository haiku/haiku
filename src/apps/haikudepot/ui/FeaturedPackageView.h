/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FEATURED_PACKAGE_VIEW_H
#define FEATURED_PACKAGE_VIEW_H


#include <View.h>

#include "PackageInfo.h"
#include "PackageInfoListener.h"


class BGroupLayout;


class FeaturedPackageView : public BView {
public:
								FeaturedPackageView();
	virtual						~FeaturedPackageView();

			void				AddPackage(const PackageInfoRef& package);
			void				Clear();

private:
			BGroupLayout*		fPackageListLayout;

};


#endif // FEATURED_PACKAGE_VIEW_H
