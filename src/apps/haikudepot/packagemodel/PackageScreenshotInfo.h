/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SCREENSHOT_INFO_H
#define PACKAGE_SCREENSHOT_INFO_H


#include <vector>

#include <Referenceable.h>
#include <String.h>

#include "ScreenshotInfo.h"


class PackageScreenshotInfoBuilder;


/*!	Instances of this class should not be created directly; instead use the
	PackageScreenshotInfoBuilder class as a builder-constructor.
*/
class PackageScreenshotInfo : public BReferenceable
{
friend class PackageScreenshotInfoBuilder;

public:
								PackageScreenshotInfo();
								PackageScreenshotInfo(const PackageScreenshotInfo& other);

			bool				operator==(const PackageScreenshotInfo& other) const;
			bool				operator!=(const PackageScreenshotInfo& other) const;

			int32				Count() const;
	const 	ScreenshotInfoRef	ScreenshotAtIndex(int32 index) const;

private:
			void				AddScreenshot(const ScreenshotInfoRef& info);

private:
			std::vector<ScreenshotInfoRef>
								fScreenshotInfos;
};


typedef BReference<PackageScreenshotInfo> PackageScreenshotInfoRef;


class PackageScreenshotInfoBuilder
{
public:
								PackageScreenshotInfoBuilder();
								PackageScreenshotInfoBuilder(const PackageScreenshotInfoRef& other);
	virtual						~PackageScreenshotInfoBuilder();

			PackageScreenshotInfoRef
								BuildRef();

			PackageScreenshotInfoBuilder&
								AddScreenshot(const ScreenshotInfoRef& value);

private:
			void				_InitFromSource();
			void				_Init(const PackageScreenshotInfo* value);

private:
			PackageScreenshotInfoRef
								fSource;
			std::vector<ScreenshotInfoRef>
								fScreenshotInfos;
};


#endif // PACKAGE_SCREENSHOT_INFO_H
