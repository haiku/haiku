/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <math.h>

#include "blending.h"

#if GAMMA_BLEND

// speed tests done on Pentium M, 1450 MHz
// blending two 800x600 bitmaps with "50" on each component (including alpha): 1500000 usecs
// -"-														  using gamma LUT:  651000 usecs
// -"-													   no use of floorf():  572000 usecs

// -"-												   uint16 integer version:   72000 usecs
// -"-																   inline:   60200 usecs

// for comparison:
// -"-								inline only, no LUTs, no gamma correction:   44000 usecs
// -"-							   + premultiplied alpha (less MULs, no DIVs):   16500 usecs


const float kGamma = 2.2;
const float kInverseGamma = 1.0 / kGamma;

uint16* kGammaTable = NULL;
uint8* kInverseGammaTable = NULL;


// convert_to_gamma
uint16
convert_to_gamma(uint8 value)
{
	return kGammaTable[value];
}

// init_gamma_blending
void
init_gamma_blending()
{
	// init LUT R'G'B' [0...255] -> RGB [0...25500]
	if (!kGammaTable)
		kGammaTable = new uint16[256];
	for (uint32 i = 0; i < 256; i++)
		kGammaTable[i] = (uint16)(powf((float)i / 255.0, kGamma) * 25500.0);

	// init LUT RGB [0...25500] -> R'G'B' [0...255] 
	if (!kInverseGammaTable)
		kInverseGammaTable = new uint8[25501];
	for (uint32 i = 0; i < 25501; i++)
		kInverseGammaTable[i] = (uint8)(powf((float)i / 25500.0, kInverseGamma) * 255.0);
}

// init_gamma_blending
void
uninit_gamma_blending()
{
	delete[] kGammaTable;
	kGammaTable = NULL;
	delete[] kInverseGammaTable;
	kInverseGammaTable = NULL;
}


#endif // GAMMA_BLEND
