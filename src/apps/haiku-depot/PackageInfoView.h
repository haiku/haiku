/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_VIEW_H
#define PACKAGE_INFO_VIEW_H

#include <GroupView.h>

#include "PackageInfo.h"


class TitleView;
class PackageActionView;
class PackageManager;
class PagesView;

enum {
	MSG_VOTE_UP			= 'vtup',
	MSG_VOTE_DOWN		= 'vtdn',
};


class PackageInfoView : public BGroupView {
public:
								PackageInfoView(PackageManager* packageManager);
	virtual						~PackageInfoView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetPackage(const PackageInfo& package);
			void				Clear();

private:
			TitleView*			fTitleView;
			PackageActionView*	fPackageActionView;
			PagesView*			fPagesView;
};

#endif // PACKAGE_INFO_VIEW_H
