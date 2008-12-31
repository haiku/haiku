/*
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! Global settings of the app server regarding antialiasing and font hinting

#include "GlobalSubpixelSettings.h"

// NOTE: all these are initialized in DesktopSettings.cpp
bool gSubpixelAntialiasing;
uint8 gDefaultHintingMode;
uint8 gSubpixelAverageWeight;
bool gSubpixelOrderingRGB;
