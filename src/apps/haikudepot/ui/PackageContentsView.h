/*
 * Copyright 2015, TigerKid001.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CONTENTS_VIEW_H
#define PACKAGE_CONTENTS_VIEW_H

#include <OutlineListView.h>
#include <View.h>
#include "PackageInfo.h"
#include "ScrollableGroupView.h"

class PackageContentsView : public BView {
public:
								PackageContentsView(const char* name);
	virtual						~PackageContentsView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

			void				SetPackage(const PackageInfo& package);
			void	 			Clear();

private:
			class PackageContentOutliner;

			BGroupLayout*		fLayout;
			BOutlineListView*	fContentListView;
};

#endif // PACKAGE_CONTENTS_VIEW_H
