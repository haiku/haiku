/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>
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
class WorkStatusView;


class PackageListView : public BColumnListView {
public:
								PackageListView(Model* model);
	virtual						~PackageListView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

	virtual void				SelectionChanged();

	virtual void				Clear();
			void				RetainPackages(const std::vector<PackageInfoRef>& packages);
			void				AddRemovePackages(const std::vector<PackageInfoRef>& addedPackages,
									const std::vector<PackageInfoRef>& removedPackages);

			void				SelectPackage(const PackageInfoRef& package);

			void				AttachWorkStatusView(WorkStatusView* view);

			void				HandleIconsChanged();
			void				HandlePackagesChanged(const PackageInfoEvents& events);

private:
			void				_HandlePackageChanged(const PackageInfoEvent& event);

			void				_AddPackage(const PackageInfoRef& package);
			void				_RemovePackage(const PackageInfoRef& package);

			PackageRow*			_FindRow(const PackageInfoRef& package);
			PackageRow*			_FindRow(const BString& packageName);

private:
			class ItemCountView;
			struct RowByNameHashDefinition;
			typedef BOpenHashTable<RowByNameHashDefinition> RowByNameTable;

			Model*				fModel;
			ItemCountView*		fItemCountView;
			RowByNameTable*		fRowByNameTable;

			WorkStatusView*		fWorkStatusView;

			bool				fIgnoreSelectionChanged;
};

#endif // PACKAGE_LIST_VIEW_H
