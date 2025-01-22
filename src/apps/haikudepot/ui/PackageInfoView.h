/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020-2025, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_VIEW_H
#define PACKAGE_INFO_VIEW_H

#include <GroupView.h>

#include "Model.h"
#include "PackageInfo.h"
#include "PackageInfoListener.h"
#include "ProcessCoordinator.h"


class BCardLayout;
class BLocker;
class OnePackageMessagePackageListener;
class PackageActionView;
class PagesView;
class TitleView;

enum {
	MSG_RATE_PACKAGE	= 'rate',
	MSG_SHOW_SCREENSHOT = 'shws',
};


class PackageInfoView : public BView {
public:
								PackageInfoView(Model* model,
									ProcessCoordinatorConsumer*
										processCoordinatorConsumer);
	virtual						~PackageInfoView();

	virtual void				AttachedToWindow();

			void				SetPackage(const PackageInfoRef& package);
			const PackageInfoRef& Package() const
									{ return fPackage; }
			void				Clear();

			void				HandleScreenshotCached(const ScreenshotCoordinate& coordinate);
			void				HandleIconsChanged();
			void				HandlePackagesChanged(const PackageInfoEvents& events);

private:
	static const ScreenshotCoordinate
								_ScreenshotThumbCoordinate(const PackageInfoRef& package);
			void				_SetPackageScreenshotThumb(const PackageInfoRef& package);
			void				_HandleScreenshotCached(const PackageInfoRef& package,
									const ScreenshotCoordinate& coordinate);

			void				_HandlePackageChanged(const PackageInfoEvent& event);

private:
			Model*				fModel;

			BCardLayout*		fCardLayout;
			TitleView*			fTitleView;
			PackageActionView*	fPackageActionView;
			PagesView*			fPagesView;

			PackageInfoRef		fPackage;
			OnePackageMessagePackageListener*
								fPackageListener;
			ProcessCoordinatorConsumer*
								fProcessCoordinatorConsumer;
};

#endif // PACKAGE_INFO_VIEW_H
