// MediaRoutingDefs.h
// c.lenz 9oct99
//
// Constants used in MediaRoutingView and friends:
// - Colors
// - Cursors
//
// HISTORY
// 6oct99	c.lenz		Begun
// 

#ifndef __MediaRoutingDefs_H__
#define __MediaRoutingDefs_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** cursors
// -------------------------------------------------------- //

const unsigned char M_CABLE_CURSOR [] = {
// PREAMBLE
  16,   1,   0,   0,
// BITMAP
 224,   0, 144,   0, 168,   0,  68,   0,  34,   0,  17, 128,  11,  64,   7, 160,
   7, 208,   3, 232,   1, 244,   0, 250,   0, 125,   0,  63,   0,  30,   0,  12,
// MASK
 224,   0, 240,   0, 248,   0, 124,   0,  62,   0,  31, 128,  15, 192,   7, 224,
   7, 240,   3, 248,   1, 252,   0, 254,   0, 127,   0,  63,   0,  30,   0,  12
};

__END_CORTEX_NAMESPACE
#endif /* __MediaRoutingDefs_H__ */
