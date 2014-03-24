/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_LISTENER_H
#define PACKAGE_INFO_LISTENER_H


#include <Referenceable.h>


enum {
	PKG_CHANGED_DESCRIPTION		= 1 << 0,
	PKG_CHANGED_RATINGS			= 1 << 1,
	PKG_CHANGED_SCREENSHOTS		= 1 << 2,
	PKG_CHANGED_STATE			= 1 << 3,
	PKG_CHANGED_ICON			= 1 << 4,
	PKG_CHANGED_CHANGELOG		= 1 << 5,
	PKG_CHANGED_CATEGORIES		= 1 << 6
	// ...
};


class PackageInfo;
typedef BReference<PackageInfo>	PackageInfoRef;


class PackageInfoEvent {
public:
								PackageInfoEvent();
								PackageInfoEvent(const PackageInfoRef& package,
									uint32 changes);
								PackageInfoEvent(const PackageInfoEvent& other);
	virtual						~PackageInfoEvent();

			bool				operator==(const PackageInfoEvent& other);
			bool				operator!=(const PackageInfoEvent& other);
			PackageInfoEvent&	operator=(const PackageInfoEvent& other);

	inline	const PackageInfoRef& Package() const
									{ return fPackage; }

	inline	uint32				Changes() const
									{ return fChanges; }

private:
			PackageInfoRef		fPackage;
			uint32				fChanges;
};


class PackageInfoListener : public BReferenceable {
public:
								PackageInfoListener();
	virtual						~PackageInfoListener();

	virtual	void				PackageChanged(
									const PackageInfoEvent& event) = 0;
};


typedef BReference<PackageInfoListener> PackageInfoListenerRef;


#endif // PACKAGE_INFO_LISTENER_H
