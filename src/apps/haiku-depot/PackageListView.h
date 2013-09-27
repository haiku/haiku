/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LIST_VIEW_H
#define PACKAGE_LIST_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Locker.h>

#include "PackageInfo.h"


class PackageRow;
class PackageListener;

enum {
	MSG_PACKAGE_SELECTED		= 'pkgs',
};


class PackageListView : public BColumnListView {
public:
								PackageListView(BLocker* modelLock);
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

	virtual	void				MessageReceived(BMessage* message);

	virtual void				SelectionChanged();

			void				AddPackage(const PackageInfoRef& package);

private:
			PackageRow*			_FindRow(const PackageInfoRef& package,
									PackageRow* parent = NULL);
			PackageRow*			_FindRow(const BString& packageTitle,
									PackageRow* parent = NULL);

private:
			class ItemCountView;

			BLocker*			fModelLock;
			ItemCountView*		fItemCountView;
			PackageListener*	fPackageListener;
};

#endif // PACKAGE_LIST_VIEW_H
