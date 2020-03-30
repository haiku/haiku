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
	const	char*			_PackageNameAtIndex(int32 index) const;
			int32			_InsertionIndex(
								const BString& packageName) const;
			int32			_InsertionIndexBinary(const BString& packageName,
								int32 startIndex, int32 endIndex) const;
			int32			_InsertionIndexLinear(const BString& packageName,
								int32 startIndex, int32 endIndex) const;

private:
			BGroupLayout*		fPackageListLayout;
			ScrollableGroupView* fContainerView;
};


#endif // FEATURED_PACKAGES_VIEW_H
