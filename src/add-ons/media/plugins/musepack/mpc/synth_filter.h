#ifndef _synth_filter_h_
#define _synth_filter_h_

#include "mpc_dec.h"

/* D E F I N E S */
#define ftol(A,B)                             \
     { tmp = *(int*) & A - 0x4B7F8000;        \
       if ( tmp != (short)tmp )               \
           tmp = (tmp>>31) ^ 0x7FFF, clips++; \
       B   = (short) tmp;                     \
     }

#endif
