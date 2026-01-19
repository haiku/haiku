/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2022-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_LISTENER_H
#define PACKAGE_INFO_LISTENER_H


#include <vector>

#include <Archivable.h>
#include <String.h>
#include <Referenceable.h>

#include "PackageInfo.h"


enum {
	PKG_CHANGED_LOCALIZED_TEXT				= 1 << 0,
		// ^ Covers title, summary, description and changelog.
	PKG_CHANGED_RATINGS						= 1 << 1,
	PKG_CHANGED_SCREENSHOTS					= 1 << 2,
	PKG_CHANGED_LOCAL_INFO					= 1 << 3,
		// ^ Covers state, download and size.
	PKG_CHANGED_CLASSIFICATION				= 1 << 4,
		// ^ This covers categories, prominence and is native desktop
	PKG_CHANGED_CORE_INFO					= 1 << 5
		// ^ covers the version change timestamp
};


//class PackageInfo;
//typedef BReference<PackageInfo>	PackageInfoRef;


/*!	PackageInfoChangeEvent is akin to the PackageChangeEvent but couples a
	change-mask together with the actual PackageInfo class rather than just the
	name.
*/
class PackageInfoChangeEvent {

public:
								PackageInfoChangeEvent(PackageInfoRef package, uint32 change);
	virtual						~PackageInfoChangeEvent();

	const	PackageInfoRef		Package() const
									{ return fPackage; }
			uint32				Changes() const
									{ return fChanges; }

private:
			PackageInfoRef		fPackage;
			uint32				fChanges;
};


/*!	Couples together the name of a package together with a mask which describes
	the change that has occurred against that package.
*/
class PackageChangeEvent : public BArchivable
{
public:
								PackageChangeEvent();
								PackageChangeEvent(const PackageInfoRef& package, uint32 changes);
								PackageChangeEvent(const BString& packageName, uint32 changes);
								PackageChangeEvent(const PackageChangeEvent& other);
								PackageChangeEvent(const BMessage* from);
	virtual						~PackageChangeEvent();

			bool				operator==(const PackageChangeEvent& other);
			bool				operator!=(const PackageChangeEvent& other);
			PackageChangeEvent&	operator=(const PackageChangeEvent& other);

	inline	const BString&		PackageName() const
									{ return fPackageName; }

	inline	uint32				Changes() const
									{ return fChanges; }

			bool				IsValid() const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			BString				fPackageName;
			uint32				fChanges;
};


/*!	This is a collection class designed to carry a number of `PackageChangeEvent`
	instances. It can conveniently serialize and deserialize to and from a
	`BMessage`.
*/
class PackageChangeEvents : public BArchivable
{
public:
								PackageChangeEvents();
								PackageChangeEvents(const PackageChangeEvent& event);
								PackageChangeEvents(const PackageChangeEvents& other);
								PackageChangeEvents(const BMessage* from);

			bool				IsEmpty() const;
			void				AddEvent(const PackageChangeEvent event);
			int32				CountEvents() const;
			const PackageChangeEvent&
								EventAtIndex(int32 index) const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			std::vector<PackageChangeEvent>
								fEvents;
};


class PackageInfoListener : public BReferenceable
{
public:
								PackageInfoListener();
	virtual						~PackageInfoListener();

	virtual	void				PackagesChanged(const PackageChangeEvents& events) = 0;
};


typedef BReference<PackageInfoListener> PackageInfoListenerRef;


#endif // PACKAGE_INFO_LISTENER_H
