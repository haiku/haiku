/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_VIEW_H
#define PACKAGE_INFO_VIEW_H

#include <GroupView.h>

#include "PackageInfo.h"
#include "PackageInfoListener.h"


class BCardLayout;
class BLocker;
class MessagePackageListener;
class TitleView;
class PackageActionHandler;
class PackageActionView;
class PagesView;

enum {
	MSG_VOTE_UP			= 'vtup',
	MSG_VOTE_DOWN		= 'vtdn',
	MSG_RATE_PACKAGE	= 'rate',
};


class PackageInfoView : public BView {
public:
								PackageInfoView(BLocker* modelLock,
									PackageActionHandler* handler);
	virtual						~PackageInfoView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetPackage(const PackageInfoRef& package);
			const PackageInfoRef& Package() const
									{ return fPackage; }
			void				Clear();

private:
			BLocker*			fModelLock;

			BCardLayout*		fCardLayout;
			TitleView*			fTitleView;
			PackageActionView*	fPackageActionView;
			PagesView*			fPagesView;

			PackageInfoRef		fPackage;
			MessagePackageListener* fPackageListener;
};

#endif // PACKAGE_INFO_VIEW_H
