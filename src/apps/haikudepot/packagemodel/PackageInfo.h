/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <Referenceable.h>
#include <package/PackageInfo.h>

#include "Language.h"
#include "List.h"
#include "PackageClassificationInfo.h"
#include "PackageCoreInfo.h"
#include "PackageInfoListener.h"
#include "PackageLocalInfo.h"
#include "PackageLocalizedText.h"
#include "PackagePublisherInfo.h"
#include "PackageScreenshotInfo.h"
#include "ScreenshotInfo.h"
#include "UserRatingInfo.h"


using BPackageKit::BPackageInfo;


class PackageInfo : public BReferenceable {
public:
								PackageInfo();
								PackageInfo(const BPackageInfo& info);
								PackageInfo(const PackageInfo& other);

			PackageInfo&		operator=(const PackageInfo& other);
			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			uint32				DiffMask(const PackageInfo& other) const;

			const BString&		Name() const
									{ return fName; }

			PackageCoreInfoRef	CoreInfo() const
									{ return fCoreInfo; }
			void				SetCoreInfo(PackageCoreInfoRef value);

			PackageLocalizedTextRef
								LocalizedText() const;
			void				SetLocalizedText(PackageLocalizedTextRef value);

			void				SetPackageClassificationInfo(
									PackageClassificationInfoRef value);
			PackageClassificationInfoRef
								PackageClassificationInfo() const
									{ return fClassificationInfo; }

			UserRatingInfoRef	UserRatingInfo() const;
			void				SetUserRatingInfo(UserRatingInfoRef userRatingInfo);

			PackageLocalInfoRef	LocalInfo() const;
			void				SetLocalInfo(PackageLocalInfoRef& localInfo);

			PackageScreenshotInfoRef
								ScreenshotInfo() const
									{ return fScreenshotInfo; }
			void				SetScreenshotInfo(PackageScreenshotInfoRef value);

			bool				AddListener(
									const PackageInfoListenerRef& listener);
			void				RemoveListener(
									const PackageInfoListenerRef& listener);

			void				StartCollatingChanges();
			void				EndCollatingChanges();
			void				NotifyChangedIcon();

private:
			void				_NotifyListeners(uint32 changes);
			void				_NotifyListenersImmediate(uint32 changes);

private:
			BString				fName;

			PackageCoreInfoRef	fCoreInfo;
			PackageLocalizedTextRef
								fLocalizedText;
			PackageClassificationInfoRef
								fClassificationInfo;
			PackageScreenshotInfoRef
								fScreenshotInfo;
			UserRatingInfoRef	fUserRatingInfo;
			PackageLocalInfoRef	fLocalInfo;

			std::vector<PackageInfoListenerRef>
								fListeners;
			bool				fIsCollatingChanges;
			uint32				fCollatedChanges;


};


typedef BReference<PackageInfo> PackageInfoRef;


#endif // PACKAGE_INFO_H
