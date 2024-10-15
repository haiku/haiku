/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GENERIC_VIDEO_SPLASH_H
#define GENERIC_VIDEO_SPLASH_H


#include <SupportDefs.h>


void
compute_splash_logo_placement(uint32 screenWidth, uint32 screenHeight,
	int& width, int& height, int& x, int& y)
{
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;

	width = min_c(kSplashLogoWidth, screenWidth);
	height = min_c(uint32(kSplashLogoHeight) + iconsHalfHeight,
		screenHeight);
	int placementX = max_c(0, min_c(100, kSplashLogoPlacementX));
	int placementY = max_c(0, min_c(100, kSplashLogoPlacementY));

	x = (screenWidth - width) * placementX / 100;
	y = (screenHeight - height) * placementY / 100;

	height = min_c(kSplashLogoHeight, screenHeight);
}


void
compute_splash_icons_placement(uint32 screenWidth, uint32 screenHeight,
	int& width, int& height, int& x, int& y)
{
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;

	width = min_c(kSplashIconsWidth, screenWidth);
	height = min_c(uint32(kSplashLogoHeight) + iconsHalfHeight,
		screenHeight);
	int placementX = max_c(0, min_c(100, kSplashIconsPlacementX));
	int placementY = max_c(0, min_c(100, kSplashIconsPlacementY));

	x = (screenWidth - width) * placementX / 100;
	y = kSplashLogoHeight + (screenHeight - height)
		* placementY / 100;

	height = min_c(iconsHalfHeight, screenHeight);
}


#endif	/* GENERIC_VIDEO_SPLASH_H */
