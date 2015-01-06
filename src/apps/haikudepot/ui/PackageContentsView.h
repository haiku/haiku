/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
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
								PackageContentsView(BRect frame, const char* name);
	virtual						~PackageContentsView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

			void				AddPackage(const PackageInfo& package);
			void 			MakeEmpty();
private:
			class PackageContentOutliner;
			BGroupLayout*		fLayout;
			BOutlineListView* fContentsList;
};

#endif // PACKAGE_CONTENTS_VIEW_H
