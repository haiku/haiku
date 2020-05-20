/*
 * Copyright 2015, TigerKid001.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_CONTENTS_VIEW_H
#define PACKAGE_CONTENTS_VIEW_H

#include <Locker.h>
#include <View.h>

#include "PackageInfo.h"

class BOutlineListView;


class PackageContentsView : public BView {
public:
								PackageContentsView(const char* name);
	virtual						~PackageContentsView();

	virtual void				AttachedToWindow();
	virtual	void				AllAttached();

			void				SetPackage(const PackageInfoRef& package);
			void	 			Clear();

private:
			void				_InitContentPopulator();
	static	int32				_ContentPopulatorThread(void* arg);
			bool				_PopulatePackageContents(
									const PackageInfo& package);
			int32				_InstallLocation(
									const PackageInfo& package) const;

private:
			BOutlineListView*	fContentListView;

			thread_id			fContentPopulator;
			sem_id				fContentPopulatorSem;
			BLocker				fPackageLock;
			PackageInfoRef		fPackage;
			PackageState		fLastPackageState;
};

#endif // PACKAGE_CONTENTS_VIEW_H
