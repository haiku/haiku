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
#include "Star.h"
#include "Shared.h"

#define BIGMYSTERY 1800.0
#define MAXANGLES 16384

/* Construction/Destruction */
void InitStar(Star *s)
{
	int i;
	for (i=0;i<3;i++) {
		s->position[i] = RandFlt(-10000.0, 10000.0);
	}
	s->rotSpeed = RandFlt(0.4, 0.9);
	s->mystery = RandFlt(0.0, 10.0);
}

void UpdateStar(flurry_info_t *info, Star *s)
{
	 /* speed control */
	float rotationsPerSecond = (float) (2.0*M_PI*12.0/MAXANGLES) * s->rotSpeed;
	double thisPointInRadians;
	double thisAngle = info->fTime*rotationsPerSecond;
	float cf;
	double tmpX1,tmpY1,tmpZ1;
	double tmpX2,tmpY2,tmpZ2;
	double tmpX3,tmpY3,tmpZ3;
	double tmpX4,tmpY4,tmpZ4;
	double rotation;
	double cr;
	double sr;

	s->ate = 0;

	cf = ((float) (cos(7.0*((info->fTime)*rotationsPerSecond)) +
		cos(3.0*((info->fTime)*rotationsPerSecond)) +
		cos(13.0*((info->fTime)*rotationsPerSecond))));
	cf /= 6.0f;
	cf += 0.75f;
	thisPointInRadians = 2.0 * M_PI * (double) s->mystery / (double) BIGMYSTERY;

	s->position[0] = 250.0f * cf * (float) cos(11.0 * (thisPointInRadians + (3.0*thisAngle)));
	s->position[1] = 250.0f * cf * (float) sin(12.0 * (thisPointInRadians + (4.0*thisAngle)));
	s->position[2] = 250.0f * (float) cos((23.0 * (thisPointInRadians + (12.0*thisAngle))));

	rotation = thisAngle*0.501 + 5.01 * (double) s->mystery / (double) BIGMYSTERY;
	cr = cos(rotation);
	sr = sin(rotation);
	tmpX1 = s->position[0] * cr - s->position[1] * sr;
	tmpY1 = s->position[1] * cr + s->position[0] * sr;
	tmpZ1 = s->position[2];

	tmpX2 = tmpX1 * cr - tmpZ1 * sr;
	tmpY2 = tmpY1;
	tmpZ2 = tmpZ1 * cr + tmpX1 * sr;

	tmpX3 = tmpX2;
	tmpY3 = tmpY2 * cr - tmpZ2 * sr;
	tmpZ3 = tmpZ2 * cr + tmpY2 * sr + seraphDistance;

	rotation = thisAngle*2.501 + 85.01 * (double) s->mystery / (double) BIGMYSTERY;
	cr = cos(rotation);
	sr = sin(rotation);
	tmpX4 = tmpX3 * cr - tmpY3 * sr;
	tmpY4 = tmpY3 * cr + tmpX3 * sr;
	tmpZ4 = tmpZ3;

	s->position[0] = (float) tmpX4;
	s->position[1] = (float) tmpY4;
	s->position[2] = (float) tmpZ4;
}
