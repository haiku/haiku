/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_PACKAGE_LISTENER_H
#define MESSAGE_PACKAGE_LISTENER_H


#include "PackageInfoListener.h"


enum {
	MSG_UPDATE_PACKAGE		= 'updp'
};

class BHandler;


class MessagePackageListener : public PackageInfoListener {
public:
								MessagePackageListener(BHandler* target);
	virtual						~MessagePackageListener();

	virtual	void				PackageChanged(const PackageInfoEvent& event);

			void				SetChangesMask(uint32 mask);

private:
			BHandler*			fTarget;
			uint32				fChangesMask;
};


class OnePackageMessagePackageListener : public MessagePackageListener {
public:
								OnePackageMessagePackageListener(
									BHandler* target);
	virtual						~OnePackageMessagePackageListener();

			void				SetPackage(const PackageInfoRef& package);
			const PackageInfoRef& Package() const;

private:
			PackageInfoRef		fPackage;
};


#endif // MESSAGE_PACKAGE_LISTENER_H
