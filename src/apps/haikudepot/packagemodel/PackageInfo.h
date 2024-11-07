/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <vector>

#include <Referenceable.h>
#include <package/PackageInfo.h>

#include "Language.h"
#include "List.h"
#include "PackageClassificationInfo.h"
#include "PackageInfoListener.h"
#include "PackageLocalInfo.h"
#include "PackageLocalizedText.h"
#include "PublisherInfo.h"
#include "ScreenshotInfo.h"
#include "UserRatingInfo.h"


using BPackageKit::BPackageInfo;
using BPackageKit::BPackageVersion;


class PackageInfo : public BReferenceable {
public:
								PackageInfo();
								PackageInfo(const BPackageInfo& info);
								PackageInfo(const PackageInfo& other);

			PackageInfo&		operator=(const PackageInfo& other);
			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			const BString&		Name() const
									{ return fName; }

			PackageLocalizedTextRef
								LocalizedText() const;
			void				SetLocalizedText(PackageLocalizedTextRef value);

			const BPackageVersion&
								Version() const
									{ return fVersion; }
			const PublisherInfo& Publisher() const
									{ return fPublisher; }

			const BString		Architecture() const
									{ return fArchitecture; }

			void				SetPackageClassificationInfo(
									PackageClassificationInfoRef value);
			PackageClassificationInfoRef
								PackageClassificationInfo() const
									{ return fPackageClassificationInfo; }

			UserRatingInfoRef	UserRatingInfo() const;
			void				SetUserRatingInfo(UserRatingInfoRef userRatingInfo);

			PackageLocalInfoRef	LocalInfo() const;
			void				SetLocalInfo(PackageLocalInfoRef& localInfo);

			void				ClearScreenshotInfos();
			void				AddScreenshotInfo(
									const ScreenshotInfoRef& info);
			int32				CountScreenshotInfos() const;
			ScreenshotInfoRef	ScreenshotInfoAtIndex(int32 index) const;

			void				SetVersionCreateTimestamp(uint64 value);
			uint64				VersionCreateTimestamp() const
									{ return fVersionCreateTimestamp; }

			void				SetDepotName(const BString& depotName);
			const BString&		DepotName() const
									{ return fDepotName; }

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
			BPackageVersion		fVersion;
			PublisherInfo		fPublisher;
			PackageLocalizedTextRef
								fLocalizedText;
			PackageClassificationInfoRef
								fPackageClassificationInfo;
			std::vector<ScreenshotInfoRef>
								fScreenshotInfos;
			UserRatingInfoRef	fUserRatingInfo;
			PackageLocalInfoRef	fLocalInfo;

			std::vector<PackageInfoListenerRef>
								fListeners;
			BString				fArchitecture;
			BString				fDepotName;

			bool				fIsCollatingChanges;
			uint32				fCollatedChanges;

			uint64				fVersionCreateTimestamp;
				// milliseconds since epoch
};


typedef BReference<PackageInfo> PackageInfoRef;


#endif // PACKAGE_INFO_H
