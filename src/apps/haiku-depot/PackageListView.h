/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LIST_VIEW_H
#define PACKAGE_LIST_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>

#include "PackageInfo.h"


class PackageRow;

class PackageListView : public BColumnListView {
public:
								PackageListView();
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				AddPackage(const PackageInfo& package);
			
private:
			PackageRow*			_FindRow(const PackageInfo& package,
									PackageRow* parent = NULL);
};

#endif // PACKAGE_LIST_VIEW_H
