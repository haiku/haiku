/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FEATURED_PACKAGES_VIEW_H
#define FEATURED_PACKAGES_VIEW_H


#include <View.h>

#include "PackageInfo.h"
#include "PackageInfoListener.h"


class StackedFeaturedPackagesView;


class FeaturedPackagesView : public BView {
public:
								FeaturedPackagesView();
	virtual						~FeaturedPackagesView();

	virtual	void				FrameResized(float width, float height);
	virtual	void				DoLayout();

			void				AddPackage(const PackageInfoRef& package);
			void				RemovePackage(const PackageInfoRef& package);
			void				Clear();

			void				SelectPackage(const PackageInfoRef& package,
									bool scrollToEntry = false);

	static	void				CleanupIcons();

private:
			void			_AdjustViews();

private:

			BScrollView*		fScrollView;
			StackedFeaturedPackagesView*
								fPackagesView;
};


#endif // FEATURED_PACKAGES_VIEW_H
