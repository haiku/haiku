/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_PACKAGE_LISTENER_H
#define MESSAGE_PACKAGE_LISTENER_H


#include "PackageInfoListener.h"


enum {
	MSG_UPDATE_PACKAGE		= 'updp'
};

class BView;


class MessagePackageListener : public PackageInfoListener {
public:
								MessagePackageListener(BView* view);
	virtual						~MessagePackageListener();

	virtual	void				PackageChanged(const PackageInfoEvent& event);

			void				SetPackage(const PackageInfoRef& package);
			const PackageInfoRef& Package() const;

private:
			BView*				fView;
			PackageInfoRef		fPackage;
};


#endif // MESSAGE_PACKAGE_LISTENER_H
