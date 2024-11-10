/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SCREENSHOT_INFO_H
#define PACKAGE_SCREENSHOT_INFO_H


#include <vector>

#include <Referenceable.h>
#include <String.h>

#include "ScreenshotInfo.h"


class PackageScreenshotInfo : public BReferenceable
{
public:
								PackageScreenshotInfo();
								PackageScreenshotInfo(const PackageScreenshotInfo& other);

			PackageScreenshotInfo&
								operator=(const PackageScreenshotInfo& other);
			bool				operator==(const PackageScreenshotInfo& other) const;
			bool				operator!=(const PackageScreenshotInfo& other) const;

			void				Clear();
			void				AddScreenshot(const ScreenshotInfoRef& info);
			int32				Count() const;
			ScreenshotInfoRef	ScreenshotAtIndex(int32 index) const;

private:
			std::vector<ScreenshotInfoRef>
								fScreenshotInfos;
};


typedef BReference<PackageScreenshotInfo> PackageScreenshotInfoRef;


#endif // PACKAGE_SCREENSHOT_INFO_H
