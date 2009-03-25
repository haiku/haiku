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
#ifndef _SMOKE_H_
#define _SMOKE_H_

struct flurry_info_t;

#define NUMSMOKEPARTICLES 3600

typedef union {
	float		f[4];
} floatToVector;

typedef union {
	unsigned int	i[4];
} intToVector;

typedef struct SmokeParticleV
{
	floatToVector color[4];
	floatToVector position[3];
	floatToVector oldposition[3];
	floatToVector delta[3];
	intToVector dead;
	floatToVector time;
	intToVector animFrame;
} SmokeParticleV;

typedef struct SmokeV
{
	SmokeParticleV p[NUMSMOKEPARTICLES/4];
	int nextParticle;
	int nextSubParticle;
	float lastParticleTime;
	int firstTime;
	long frame;
	float old[3];
	floatToVector seraphimVertices[NUMSMOKEPARTICLES*2+1];
	floatToVector seraphimColors[NUMSMOKEPARTICLES*4+1];
	float seraphimTextures[NUMSMOKEPARTICLES*2*4];
} SmokeV;

void InitSmoke(SmokeV *s);
void UpdateSmoke_ScalarBase(flurry_info_t *flurry, SmokeV *s);
void DrawSmoke_Scalar(flurry_info_t *flurry, SmokeV *s, float);

#endif	// _SMOKE_H_
