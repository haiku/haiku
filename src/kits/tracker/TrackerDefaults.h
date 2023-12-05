/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _TRACKER_DEFAULTS_H
#define _TRACKER_DEFAULTS_H


static const bool kDefaultShowDisksIcon = false;
static const bool kDefaultMountVolumesOntoDesktop = true;
static const bool kDefaultMountSharedVolumesOntoDesktop = true;
static const bool kDefaultEjectWhenUnmounting = true;

static const bool kDefaultDesktopFilePanelRoot = true;
static const bool kDefaultShowSelectionWhenInactive = true;

static const bool kDefaultShowFullPathInTitleBar = false;
static const bool kDefaultSingleWindowBrowse = false;
static const bool kDefaultShowNavigator = false;
static const bool kDefaultTransparentSelection = true;
static const bool kDefaultSortFolderNamesFirst = true;
static const bool kDefaultHideDotFiles = false;
static const bool kDefaultTypeAheadFiltering = false;
static const bool kDefaultGenerateImageThumbnails = true;

static const int32 kDefaultRecentApplications = 10;
static const int32 kDefaultRecentDocuments = 10;
static const int32 kDefaultRecentFolders = 10;

static const bool kDefaultShowVolumeSpaceBar = true;
static const uint8 kDefaultSpaceBarAlpha = 192;
static const rgb_color kDefaultUsedSpaceColor = { 0, 203, 0, kDefaultSpaceBarAlpha };
static const rgb_color kDefaultFreeSpaceColor = { 255, 255, 255, kDefaultSpaceBarAlpha };
static const rgb_color kDefaultWarningSpaceColor = { 203, 0, 0, kDefaultSpaceBarAlpha };


#endif	// _TRACKER_DEFAULTS_H
