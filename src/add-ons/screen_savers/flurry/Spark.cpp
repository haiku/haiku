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
#include "Spark.h"
#include "Shared.h"

#define BIGMYSTERY 1800.0
#define MAXANGLES 16384

#define fieldCoherence 0
#define fieldSpeed 12.0f
#define fieldRange 1000.0f

void InitSpark(Spark *s)
{
	int i;
	for (i=0;i<3;i++)
	{
		s->position[i] = RandFlt(-100.0, 100.0);
	}
}

void DrawSpark(flurry_info_t *info, Spark *s)
{
	const float black[4] = {0.0f,0.0f,0.0f,1.0f};
	float width,sx,sy;
	float a;
	float c = 0.0625f;
	float screenx;
	float screeny;
	float w,z, scale;
	int k;
	width = 60000.0f * info->sys_glWidth / 1024.0f;

	z = s->position[2];
	sx = s->position[0] * info->sys_glWidth / z + info->sys_glWidth * 0.5f;
	sy = s->position[1] * info->sys_glWidth / z + info->sys_glHeight * 0.5f;
	w = width*4.0f / z;

	screenx = sx;
	screeny = sy;

	glPushMatrix();
	glTranslatef(screenx,screeny,0.0f);
	scale = w/50.0f;
	glScalef(scale,scale,0.0f);
	for (k=0;k<12;k++)
	{
		a = ((float) (random() % 3600)) / 10.0f;
		glRotatef(a,0.0f,0.0f,1.0f);
		glBegin(GL_QUAD_STRIP);
		glColor4fv(black);
		glVertex2f(-3.0f,0.0f);
		a = 2.0f + (float) (random() & 255) * c;
		glVertex2f(-3.0f,a);
		glColor4fv(s->color);
		glVertex2f(0.0f,0.0f);
		glColor4fv(black);
		glVertex2f(0.0f,a);
		glVertex2f(3.0f,0.0f);
		glVertex2f(3.0f,a);
		glEnd();
	}
	glPopMatrix();
}

void UpdateSparkColour(flurry_info_t *info, Spark *s)
{
	const float rotationsPerSecond = (float) (2.0*M_PI*fieldSpeed/MAXANGLES);
	double thisPointInRadians;
	double thisAngle = info->fTime*rotationsPerSecond;
	float cf;
	float cycleTime = 20.0f;
	float colorRot;
	float redPhaseShift;
	float greenPhaseShift;
	float bluePhaseShift;
	float baseRed;
	float baseGreen;
	float baseBlue;
	float colorTime;

	if (info->currentColorMode == rainbowColorMode)
	{
		cycleTime = 1.5f;
	}
	else if (info->currentColorMode == tiedyeColorMode)
	{
		cycleTime = 4.5f;
	}
	else if (info->currentColorMode == cyclicColorMode)
	{
		cycleTime = 20.0f;
	}
	else if (info->currentColorMode == slowCyclicColorMode)
	{
		cycleTime = 120.0f;
	}
	colorRot = (float) (2.0*M_PI/cycleTime);
	redPhaseShift = 0.0f; /* cycleTime * 0.0f / 3.0f */
	greenPhaseShift = cycleTime / 3.0f;
	bluePhaseShift = cycleTime * 2.0f / 3.0f ;
	colorTime = info->fTime;
	if (info->currentColorMode == whiteColorMode)
	{
		baseRed = 0.1875f;
		baseGreen = 0.1875f;
		baseBlue = 0.1875f;
	}
	else if (info->currentColorMode == multiColorMode)
	{
		baseRed = 0.0625f;
		baseGreen = 0.0625f;
		baseBlue = 0.0625f;
	}
	else if (info->currentColorMode == darkColorMode)
	{
		baseRed = 0.0f;
		baseGreen = 0.0f;
		baseBlue = 0.0f;
	}
	else
	{
		if (info->currentColorMode < slowCyclicColorMode)
		{
			colorTime = (info->currentColorMode / 6.0f) * cycleTime;
		}
		else
		{
			colorTime = info->fTime + info->randomSeed;
		}
		baseRed = 0.109375f * ((float) cos((colorTime+redPhaseShift)*colorRot)+1.0f);
		baseGreen = 0.109375f * ((float) cos((colorTime+greenPhaseShift)*colorRot)+1.0f);
		baseBlue = 0.109375f * ((float) cos((colorTime+bluePhaseShift)*colorRot)+1.0f);
	}

	cf = ((float) (cos(7.0*((info->fTime)*rotationsPerSecond)) +
		cos(3.0*((info->fTime)*rotationsPerSecond)) +
		cos(13.0*((info->fTime)*rotationsPerSecond))));
	cf /= 6.0f;
	cf += 2.0f;
	thisPointInRadians = 2.0 * M_PI * (double) s->mystery / (double) BIGMYSTERY;

	s->color[0] = baseRed + 0.0625f *
		(0.5f + (float) cos((15.0 * (thisPointInRadians + 3.0*thisAngle))) +
		(float) sin((7.0 * (thisPointInRadians + thisAngle))));
	s->color[1] = baseGreen + 0.0625f *
		(0.5f + (float) sin(((thisPointInRadians) + thisAngle)));
	s->color[2] = baseBlue + 0.0625f *
		(0.5f + (float) cos((37.0 * (thisPointInRadians + thisAngle))));
}

void UpdateSpark(flurry_info_t *info, Spark *s)
{
	const float rotationsPerSecond = (float) (2.0*M_PI*fieldSpeed/MAXANGLES);
	double thisPointInRadians;
	double thisAngle = info->fTime*rotationsPerSecond;
	float cf;
	int i;
	double tmpX1,tmpY1,tmpZ1;
	double tmpX2,tmpY2,tmpZ2;
	double tmpX3,tmpY3,tmpZ3;
	double tmpX4,tmpY4,tmpZ4;
	double rotation;
	double cr;
	double sr;
	float cycleTime = 20.0f;
	float colorRot;
	float redPhaseShift;
	float greenPhaseShift;
	float bluePhaseShift;
	float baseRed;
	float baseGreen;
	float baseBlue;
	float colorTime;

	float old[3];

	if (info->currentColorMode == rainbowColorMode) {
		cycleTime = 1.5f;
	} else if (info->currentColorMode == tiedyeColorMode) {
		cycleTime = 4.5f;
	} else if (info->currentColorMode == cyclicColorMode) {
		cycleTime = 20.0f;
	} else if (info->currentColorMode == slowCyclicColorMode) {
		cycleTime = 120.0f;
	}
	colorRot = (float) (2.0*M_PI/cycleTime);
	redPhaseShift = 0.0f; /* cycleTime * 0.0f / 3.0f */
	greenPhaseShift = cycleTime / 3.0f;
	bluePhaseShift = cycleTime * 2.0f / 3.0f ;
	colorTime = info->fTime;
	if (info->currentColorMode == whiteColorMode) {
		baseRed = 0.1875f;
		baseGreen = 0.1875f;
		baseBlue = 0.1875f;
	} else if (info->currentColorMode == multiColorMode) {
		baseRed = 0.0625f;
		baseGreen = 0.0625f;
		baseBlue = 0.0625f;
	} else if (info->currentColorMode == darkColorMode) {
		baseRed = 0.0f;
		baseGreen = 0.0f;
		baseBlue = 0.0f;
	} else {
		if(info->currentColorMode < slowCyclicColorMode) {
			colorTime = (info->currentColorMode / 6.0f) * cycleTime;
		} else {
			colorTime = info->fTime + info->randomSeed;
		}
		baseRed = 0.109375f * ((float) cos((colorTime+redPhaseShift)*colorRot)+1.0f);
		baseGreen = 0.109375f * ((float) cos((colorTime+greenPhaseShift)*colorRot)+1.0f);
		baseBlue = 0.109375f * ((float) cos((colorTime+bluePhaseShift)*colorRot)+1.0f);
	}

	for (i=0;i<3;i++) {
		old[i] = s->position[i];
	}

	cf = ((float) (cos(7.0*((info->fTime)*rotationsPerSecond)) +
		cos(3.0*((info->fTime)*rotationsPerSecond)) +
		cos(13.0*((info->fTime)*rotationsPerSecond))));
	cf /= 6.0f;
	cf += 2.0f;
	thisPointInRadians = 2.0 * M_PI * (double) s->mystery / (double) BIGMYSTERY;

	s->color[0] = baseRed + 0.0625f *
		(0.5f + (float) cos((15.0 * (thisPointInRadians + 3.0*thisAngle))) +
		 (float) sin((7.0 * (thisPointInRadians + thisAngle))));
	s->color[1] = baseGreen + 0.0625f *
		(0.5f + (float) sin(((thisPointInRadians) + thisAngle)));
	s->color[2] = baseBlue + 0.0625f *
		(0.5f + (float) cos((37.0 * (thisPointInRadians + thisAngle))));
	s->position[0] = fieldRange * cf *
		(float) cos(11.0 * (thisPointInRadians + (3.0*thisAngle)));
	s->position[1] = fieldRange * cf *
		(float) sin(12.0 * (thisPointInRadians + (4.0*thisAngle)));
	s->position[2] = fieldRange *
		(float) cos((23.0 * (thisPointInRadians + (12.0*thisAngle))));

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

	s->position[0] = (float) tmpX4 + RandBell(5.0f*fieldCoherence);
	s->position[1] = (float) tmpY4 + RandBell(5.0f*fieldCoherence);
	s->position[2] = (float) tmpZ4 + RandBell(5.0f*fieldCoherence);

	for (i=0;i<3;i++) {
		s->delta[i] = (s->position[i] - old[i])/info->fDeltaTime;
	}
}
