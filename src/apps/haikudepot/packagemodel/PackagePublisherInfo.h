/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_PUBLISHER_INFO_H
#define PACKAGE_PUBLISHER_INFO_H


#include <Referenceable.h>
#include <String.h>


class PackagePublisherInfo : public BReferenceable
{
public:
								PackagePublisherInfo();
								PackagePublisherInfo(const BString& name, const BString& website);
								PackagePublisherInfo(const PackagePublisherInfo& other);

			PackagePublisherInfo&
								operator=(const PackagePublisherInfo& other);
			bool				operator==(const PackagePublisherInfo& other) const;
			bool				operator!=(const PackagePublisherInfo& other) const;

			const BString&		Name() const
									{ return fName; }
			const BString&		Website() const
									{ return fWebsite; }

private:
			BString				fName;
			BString				fWebsite;
};


typedef BReference<PackagePublisherInfo> PackagePublisherInfoRef;


#endif // PACKAGE_PUBLISHER_INFO_H
