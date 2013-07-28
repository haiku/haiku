/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_VIEW_H
#define PACKAGE_INFO_VIEW_H

#include <GroupView.h>

#include "PackageInfo.h"


class TitleView;
class PagesView;


class PackageInfoView : public BGroupView {
public:
								PackageInfoView();
	virtual						~PackageInfoView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetPackage(const PackageInfo& package);
			void				Clear();

private:
			TitleView*			fTitleView;
			PagesView*			fPagesView;
};

#endif // PACKAGE_INFO_VIEW_H
