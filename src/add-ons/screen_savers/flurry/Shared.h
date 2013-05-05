/*

Copyright (c) 2002, Calum Robinson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the author nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef _SHARED_H_
#define _SHARED_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glu.h>

struct SmokeV;
struct Spark;
struct Star;

#define MAX_SPARKS 64
#define seraphDistance 2000.0f

/* used to compute the min and max of two expresions */
#define MIN_(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX_(a, b)  (((a) > (b)) ? (a) : (b))

#define random() rand()
#define RandFlt(min, max) (min + (max - min) * rand() / (float) RAND_MAX)
#define RandBell(scale) (scale * (1.0f - (rand() + rand() + rand()) / ((float) RAND_MAX * 1.5f)))

typedef enum _ColorModes
{
	redColorMode = 0,
	magentaColorMode,
	blueColorMode,
	cyanColorMode,
	greenColorMode,
	yellowColorMode,
	slowCyclicColorMode,
	cyclicColorMode,
	tiedyeColorMode,
	rainbowColorMode,
	whiteColorMode,
	multiColorMode,
	darkColorMode
} ColorModes;

typedef struct flurry_info_t {
	flurry_info_t *next;
	ColorModes currentColorMode;
	SmokeV *s;
	Star *star;
	Spark *spark[MAX_SPARKS];
	float streamExpansion;
	int numStreams;
	double randomSeed;
	double fTime;
	double fOldTime;
	double fDeltaTime;
	double briteFactor;
	float drag;
	int dframe;
	float sys_glWidth;
	float sys_glHeight;
} flurry_info_t;


#endif	// _SHARED_H_
