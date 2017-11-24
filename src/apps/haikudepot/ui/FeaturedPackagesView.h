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
class ScrollableGroupView;


class FeaturedPackagesView : public BView {
public:
								FeaturedPackagesView();
	virtual						~FeaturedPackagesView();

			void				AddPackage(const PackageInfoRef& package);
			void				RemovePackage(const PackageInfoRef& package);
			void				Clear();

			void				SelectPackage(const PackageInfoRef& package,
									bool scrollToEntry = false);

	static	void				CleanupIcons();

private:
			BGroupLayout*		fPackageListLayout;
			ScrollableGroupView* fContainerView;
};


#endif // FEATURED_PACKAGES_VIEW_H
