/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef UI_DEFINES_H
#define UI_DEFINES_H

const rgb_color kBlack			= {   0,   0,   0, 255 };
const rgb_color kWhite			= { 255, 255, 255, 255 };

const rgb_color kRed			= { 255, 0, 0, 255 };
const rgb_color kGreen			= { 0, 255, 0, 255 };
const rgb_color kBlue			= { 0, 0, 255, 255 };

const rgb_color kOrange			= { 255, 217, 121, 255 };
const rgb_color kLightOrange	= { 255, 217, 138, 255 };
const rgb_color kDarkOrange		= { 255, 145,  71, 255 };

const rgb_color kAlphaLow		= { 0xbb, 0xbb, 0xbb, 0xff };
const rgb_color kAlphaHigh		= { 0xe0, 0xe0, 0xe0, 0xff };

	// tweak view settings
const rgb_color kStripesHigh	= { 112, 112, 112, 255 };
const rgb_color kStripesLow		= { 104, 104, 104, 255 };


const pattern kStripes			= { { 0xc7, 0x8f, 0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3 } };
const pattern kDotted			= { { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa } };
const pattern kDottedBigger		= { { 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc } };
const pattern kDottedBig		= { { 0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0 } };


#endif // UI_DEFINES_H
