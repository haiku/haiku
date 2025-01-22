/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2022-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_LISTENER_H
#define PACKAGE_INFO_LISTENER_H


#include <vector>

#include <Archivable.h>
#include <String.h>
#include <Referenceable.h>


extern const char* kPackageInfoChangesKey;
extern const char* kPackageInfoPackageNameKey;
extern const char* kPackageInfoEventsKey;


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


class PackageInfo;
typedef BReference<PackageInfo>	PackageInfoRef;


class PackageInfoEvent : public BArchivable {
public:
								PackageInfoEvent();
								PackageInfoEvent(const PackageInfoRef& package, uint32 changes);
								PackageInfoEvent(const PackageInfoEvent& other);
	virtual						~PackageInfoEvent();

			bool				operator==(const PackageInfoEvent& other);
			bool				operator!=(const PackageInfoEvent& other);
			PackageInfoEvent&	operator=(const PackageInfoEvent& other);

	inline	const PackageInfoRef&
								Package() const
									{ return fPackage; }

	inline	uint32				Changes() const
									{ return fChanges; }

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			PackageInfoRef		fPackage;
			uint32				fChanges;
};


class PackageInfoEvents : public BArchivable {
public:
								PackageInfoEvents();
								PackageInfoEvents(const PackageInfoEvent& event);
								PackageInfoEvents(const PackageInfoEvents& other);

			bool				IsEmpty() const;
			void				AddEvent(const PackageInfoEvent event);
			int32				CountEvents() const;
			const PackageInfoEvent&
								EventAtIndex(int32 index) const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			std::vector<PackageInfoEvent>
								fEvents;
};


class PackageInfoListener : public BReferenceable {
public:
								PackageInfoListener();
	virtual						~PackageInfoListener();

	virtual	void				PackagesChanged(const PackageInfoEvents& events) = 0;
};


typedef BReference<PackageInfoListener> PackageInfoListenerRef;


#endif // PACKAGE_INFO_LISTENER_H
