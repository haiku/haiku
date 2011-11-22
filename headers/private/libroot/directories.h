/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_DIRECTORIES_H
#define _LIBROOT_DIRECTORIES_H


#define kGlobalBinDirectory 			"/bin"
#define kGlobalEtcDirectory 			"/etc"
#define kGlobalPackageLinksDirectory	"/package-links"
#define kGlobalSystemDirectory 			"/system"
#define kGlobalTempDirectory 			"/tmp"
#define kGlobalVarDirectory 			"/var"

#define kAppsDirectory 					"/boot/apps"
#define kPreferencesDirectory 			"/boot/preferences"

#define kAppLocalAddonsDirectory 		"%A/add-ons"
#define kAppLocalLibDirectory 			"%A/lib"

#define kVolumeLocalSystemKernelAddonsDirectory	"system/add-ons/kernel"
#define kVolumeLocalCommonNonpackagedKernelAddonsDirectory	\
	"common/non-packaged/add-ons/kernel"
#define kVolumeLocalCommonKernelAddonsDirectory	"common/add-ons/kernel"
#define kVolumeLocalUserNonpackagedKernelAddonsDirectory	\
	"home/config/non-packaged/add-ons/kernel"
#define kVolumeLocalUserKernelAddonsDirectory	"home/config/add-ons/kernel"

#define kSystemDirectory 				"/boot/system"
#define kSystemAddonsDirectory 			"/boot/system/add-ons"
#define kSystemAppsDirectory 			"/boot/system/apps"
#define kSystemBinDirectory 			"/boot/system/bin"
#define kSystemDataDirectory 			"/boot/system/data"
#define kSystemDevelopDirectory 		"/boot/develop"
#define kSystemLibDirectory 			"/boot/system/lib"
#define kSystemPackagesDirectory 		"/boot/system/packages"
#define kSystemPreferencesDirectory 	"/boot/system/preferences"
#define kSystemServersDirectory 		"/boot/system/servers"

#define kCommonDirectory 				"/boot/common"
#define kCommonAddonsDirectory 			"/boot/common/add-ons"
#define kCommonBinDirectory 			"/boot/common/bin"
#define kCommonDevelopToolsBinDirectory "/boot/develop/tools/current/bin"
#define kCommonEtcDirectory 			"/boot/common/settings/etc"
#define kCommonLibDirectory 			"/boot/common/lib"
#define kCommonPackagesDirectory 		"/boot/common/packages"
#define kCommonSettingsDirectory 		"/boot/common/settings"
#define kCommonTempDirectory 			"/boot/common/cache/tmp"
#define kCommonVarDirectory 			"/boot/common/var"
#define kCommonLogDirectory 			"/boot/common/var/log"
#define kCommonNonpackagedAddonsDirectory	"/boot/common/non-packaged/add-ons"
#define kCommonNonpackagedBinDirectory 	"/boot/common/non-packaged/bin"
#define kCommonNonpackagedLibDirectory 	"/boot/common/non-packaged/lib"

#define kUserDirectory 					"/boot/home"
#define kUserConfigDirectory 			"/boot/home/config"
#define kUserAddonsDirectory 			"/boot/home/config/add-ons"
#define kUserBinDirectory 				"/boot/home/config/bin"
#define kUserLibDirectory 				"/boot/home/config/lib"
#define kUserPackagesDirectory	 		"/boot/home/config/packages"
#define kUserSettingsDirectory 			"/boot/home/config/settings"
#define kUserNonpackagedAddonsDirectory "/boot/home/config/non-packaged/add-ons"
#define kUserNonpackagedBinDirectory 	"/boot/home/config/non-packaged/bin"
#define kUserNonpackagedLibDirectory 	"/boot/home/config/non-packaged/lib"


#endif	// _LIBROOT_DIRECTORIES_H
