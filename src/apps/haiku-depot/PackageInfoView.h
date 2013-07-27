/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_VIEW_H
#define PACKAGE_INFO_VIEW_H

#include <TabView.h>


class PackageInfoView : public BTabView {
public:
								PackageInfoView();
	virtual						~PackageInfoView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			BView*				fDescriptionView;
			BView*				fRatingAndCommentsView;
			BView*				fChangeLogView;
};

#endif // PACKAGE_INFO_VIEW_H
