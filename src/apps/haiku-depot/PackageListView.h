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

enum {
	MSG_PACKAGE_SELECTED		= 'pkgs',
};


class PackageListView : public BColumnListView {
public:
								PackageListView();
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	virtual void				SelectionChanged();

			void				AddPackage(const PackageInfo& package);
			
private:
			PackageRow*			_FindRow(const PackageInfo& package,
									PackageRow* parent = NULL);
private:
			class ItemCountView;
			
			ItemCountView*		fItemCountView;
};

#endif // PACKAGE_LIST_VIEW_H
