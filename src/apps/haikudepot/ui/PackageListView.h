/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LIST_VIEW_H
#define PACKAGE_LIST_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Locker.h>
#include <util/OpenHashTable.h>

#include "Model.h"
#include "PackageInfo.h"


class PackageRow;
class PackageListener;
class WorkStatusView;


class PackageListView : public BColumnListView {
public:
								PackageListView(Model* model);
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

	virtual	void				MessageReceived(BMessage* message);

	virtual void				SelectionChanged();

	virtual void				Clear();
			void				AddPackage(const PackageInfoRef& package);
			void				RemovePackage(const PackageInfoRef& package);

			void				SelectPackage(const PackageInfoRef& package);

			void				AttachWorkStatusView(WorkStatusView* view);

private:
			PackageRow*			_FindRow(const PackageInfoRef& package);
			PackageRow*			_FindRow(const BString& packageName);

private:
			class ItemCountView;
			struct RowByNameHashDefinition;
			typedef BOpenHashTable<RowByNameHashDefinition> RowByNameTable;

			Model*				fModel;
			ItemCountView*		fItemCountView;
			PackageListener*	fPackageListener;
			RowByNameTable*		fRowByNameTable;

			WorkStatusView*		fWorkStatusView;
};

#endif // PACKAGE_LIST_VIEW_H
