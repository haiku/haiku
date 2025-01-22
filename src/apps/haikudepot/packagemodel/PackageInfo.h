/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <Referenceable.h>

#include "Language.h"
#include "List.h"
#include "PackageClassificationInfo.h"
#include "PackageCoreInfo.h"
#include "PackageLocalInfo.h"
#include "PackageLocalizedText.h"
#include "PackagePublisherInfo.h"
#include "PackageScreenshotInfo.h"
#include "PackageUserRatingInfo.h"
#include "ScreenshotInfo.h"


using BPackageKit::BPackageInfo;


class PackageInfoBuilder;


/*!	Instances of this class should not be created directly; instead use the
	PackageInfoBuilder class as a builder-constructor.
*/
class PackageInfo : public BReferenceable {
friend class PackageInfoBuilder;

public:
								PackageInfo();
								PackageInfo(const PackageInfo& other);

			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			uint32				ChangeMask(const PackageInfo& other) const;

			const BString&		Name() const;
			const PackageCoreInfoRef
								CoreInfo() const;
			const PackageLocalizedTextRef
								LocalizedText() const;
			const PackageClassificationInfoRef
								PackageClassificationInfo() const;
			const PackageUserRatingInfoRef
								UserRatingInfo() const;
			const PackageLocalInfoRef
								LocalInfo() const;
			const PackageScreenshotInfoRef
								ScreenshotInfo() const;

private:

			void				SetName(const BString& value);
			void				SetCoreInfo(PackageCoreInfoRef value);
			void				SetLocalizedText(PackageLocalizedTextRef value);
			void				SetPackageClassificationInfo(PackageClassificationInfoRef value);
			void				SetUserRatingInfo(PackageUserRatingInfoRef& userRatingInfo);
			void				SetLocalInfo(PackageLocalInfoRef& localInfo);
			void				SetScreenshotInfo(PackageScreenshotInfoRef value);

private:
			BString				fName;

			PackageCoreInfoRef	fCoreInfo;
			PackageLocalizedTextRef
								fLocalizedText;
			PackageClassificationInfoRef
								fClassificationInfo;
			PackageScreenshotInfoRef
								fScreenshotInfo;
			PackageUserRatingInfoRef
								fUserRatingInfo;
			PackageLocalInfoRef	fLocalInfo;
};


typedef BReference<PackageInfo> PackageInfoRef;


class PackageInfoBuilder
{
public:
								PackageInfoBuilder(const BString& name);
								PackageInfoBuilder(const PackageInfoRef& value);
								PackageInfoBuilder(const PackageInfo& value);
	virtual						~PackageInfoBuilder();

			PackageInfoRef		BuildRef();

			PackageInfoBuilder&
								WithCoreInfo(PackageCoreInfoRef value);
			PackageInfoBuilder&
								WithLocalizedText(PackageLocalizedTextRef value);
			PackageInfoBuilder&
								WithPackageClassificationInfo(PackageClassificationInfoRef value);
			PackageInfoBuilder&
								WithUserRatingInfo(PackageUserRatingInfoRef userRatingInfo);
			PackageInfoBuilder&
								WithLocalInfo(PackageLocalInfoRef localInfo);
			PackageInfoBuilder&
								WithScreenshotInfo(PackageScreenshotInfoRef value);

private:
			void				_Init(const PackageInfo* value);

private:
			BString				fName;

			PackageCoreInfoRef	fCoreInfo;
			PackageLocalizedTextRef
								fLocalizedText;
			PackageClassificationInfoRef
								fClassificationInfo;
			PackageScreenshotInfoRef
								fScreenshotInfo;
			PackageUserRatingInfoRef
								fUserRatingInfo;
			PackageLocalInfoRef	fLocalInfo;
};


#endif // PACKAGE_INFO_H
