#ifndef _requant_h
#define _requant_h_

#include "mpc_dec.h"

/* C O N S T A N T S */
extern const unsigned int Res_bit [18];         // bits per sample for chosen quantizer
extern const float        __Cc    [1 + 18];     // coefficients for requantization
extern const int          __Dc    [1 + 18];     // offset for requantization

#define Cc      (__Cc + 1)
#define Dc      (__Dc + 1)

#endif
