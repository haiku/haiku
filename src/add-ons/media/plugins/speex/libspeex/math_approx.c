/* Copyright (C) 2002 Jean-Marc Valin 
   File: math_approx.c
   Various math approximation functions for Speex

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <math.h>
#include "math_approx.h"

#ifdef SLOW_TRIG

float cos_sin[102] = {
   1.00000000, 0.00000000,
   0.99804751, 0.06245932,
   0.99219767, 0.12467473,
   0.98247331, 0.18640330,
   0.96891242, 0.24740396,
   0.95156795, 0.30743851,
   0.93050762, 0.36627253,
   0.90581368, 0.42367626,
   0.87758256, 0.47942554,
   0.84592450, 0.53330267,
   0.81096312, 0.58509727,
   0.77283495, 0.63460708,
   0.73168887, 0.68163876,
   0.68768556, 0.72600866,
   0.64099686, 0.76754350,
   0.59180508, 0.80608111,
   0.54030231, 0.84147098,
   0.48668967, 0.87357494,
   0.43117652, 0.90226759,
   0.37397963, 0.92743692,
   0.31532236, 0.94898462,
   0.25543377, 0.96682656,
   0.19454771, 0.98089306,
   0.13290194, 0.99112919,
   0.07073720, 0.99749499,
   0.00829623, 0.99996559,
   -0.05417714, 0.99853134,
   -0.11643894, 0.99319785,
   -0.17824606, 0.98398595,
   -0.23935712, 0.97093160,
   -0.29953351, 0.95408578,
   -0.35854022, 0.93351428,
   -0.41614684, 0.90929743,
   -0.47212841, 0.88152979,
   -0.52626633, 0.85031979,
   -0.57834920, 0.81578931,
   -0.62817362, 0.77807320,
   -0.67554504, 0.73731872,
   -0.72027847, 0.69368503,
   -0.76219923, 0.64734252,
   -0.80114362, 0.59847214,
   -0.83695955, 0.54726475,
   -0.86950718, 0.49392030,
   -0.89865940, 0.43864710,
   -0.92430238, 0.38166099,
   -0.94633597, 0.32318451,
   -0.96467415, 0.26344599,
   -0.97924529, 0.20267873,
   -0.98999250, 0.14112001,
   -0.99687381, 0.07901022,
   -0.99986235, 0.01659189
};

float speex_cos(float x)
{
   int ind;
   float delta;
   ind = (int)floor(x*16+.5);
   delta = x-0.062500*ind;
   ind <<= 1;
   return cos_sin[ind] - delta*(cos_sin[ind+1] + 
                                  .5*delta*(cos_sin[ind] - 
                                            .3333333*delta*cos_sin[ind+1]));
}

#endif

