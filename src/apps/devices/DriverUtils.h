/*
 * Copyright 2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Leo Rouleau
 */

#ifndef DRIVER_UTILS_H
#define DRIVER_UTILS_H

#include <Path.h>
#include <String.h>

class Device;

namespace DriverUtils {

bool IsDriverEnabled(const BString& driver);
bool IsPackagedDriver(const BString& driver, BString* outPackageName = NULL);
bool IsCriticalDriver(Device* device);
status_t GetDriverPackageSettings(const BString& driver, BPath& settingsPath,
	BString& relativePath);
status_t UpdatePackageBlockedEntry(const char* settingsPath, const char* packageName,
	const char* relativePath, bool block);

} // namespace DriverUtils

#endif // DRIVER_UTILS_H
