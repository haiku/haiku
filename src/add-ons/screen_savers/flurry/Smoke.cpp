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
#include "Smoke.h"

#include "Shared.h"
#include "Star.h"
#include "Spark.h"

#define MAXANGLES 16384
#define NOT_QUITE_DEAD 3

#define streamBias 7.0f
#define incohesion 0.07f
#define streamSpeed 450.0
#define gravity 1500000.0f
#define intensity 75000.0f;
#define streamSize 25000.0f
#define colorIncoherence 0.15f

static float FastDistance2D(float x, float y)
{
	/* this function computes the distance from 0,0 to x,y with ~3.5% error */
	float mn;

	/* first compute the absolute value of x,y */
	x = (x < 0.0f) ? -x : x;
	y = (y < 0.0f) ? -y : y;

	/* compute the minimum of x,y */
	mn = x<y?x:y;

	/* return the distance */
	return(x+y-(mn*0.5f)-(mn*0.25f)+(mn*0.0625f));
}

void InitSmoke(SmokeV *s)
{
	int i;
	s->nextParticle = 0;
	s->nextSubParticle = 0;
	s->lastParticleTime = 0.25f;
	s->firstTime = 1;
	s->frame = 0;
	for (i=0;i<3;i++) {
		s->old[i] = RandFlt(-100.0, 100.0);
	}
}

void UpdateSmoke_ScalarBase(flurry_info_t *info, SmokeV *s)
{
	int i,j,k;
	float sx = info->star->position[0];
	float sy = info->star->position[1];
	float sz = info->star->position[2];
	double frameRate;
	double frameRateModifier;


	s->frame++;

	if(!s->firstTime) {
		/* release 12 puffs every frame */
		if(info->fTime - s->lastParticleTime >= 1.0f / 121.0f) {
			float dx,dy,dz,deltax,deltay,deltaz;
			float f;
			float rsquared;
			float mag;

			dx = s->old[0] - sx;
			dy = s->old[1] - sy;
			dz = s->old[2] - sz;
			mag = 5.0f;
			deltax = (dx * mag);
			deltay = (dy * mag);
			deltaz = (dz * mag);
			for(i=0;i<info->numStreams;i++) {
				float streamSpeedCoherenceFactor;

				s->p[s->nextParticle].delta[0].f[s->nextSubParticle] = deltax;
				s->p[s->nextParticle].delta[1].f[s->nextSubParticle] = deltay;
				s->p[s->nextParticle].delta[2].f[s->nextSubParticle] = deltaz;
				s->p[s->nextParticle].position[0].f[s->nextSubParticle] = sx;
				s->p[s->nextParticle].position[1].f[s->nextSubParticle] = sy;
				s->p[s->nextParticle].position[2].f[s->nextSubParticle] = sz;
				s->p[s->nextParticle].oldposition[0].f[s->nextSubParticle] = sx;
				s->p[s->nextParticle].oldposition[1].f[s->nextSubParticle] = sy;
				s->p[s->nextParticle].oldposition[2].f[s->nextSubParticle] = sz;
				streamSpeedCoherenceFactor = MAX_(0.0f,1.0f + RandBell(0.25f*incohesion));
				dx = s->p[s->nextParticle].position[0].f[s->nextSubParticle] - info->spark[i]->position[0];
				dy = s->p[s->nextParticle].position[1].f[s->nextSubParticle] - info->spark[i]->position[1];
				dz = s->p[s->nextParticle].position[2].f[s->nextSubParticle] - info->spark[i]->position[2];
				rsquared = (dx*dx+dy*dy+dz*dz);
				f = streamSpeed * streamSpeedCoherenceFactor;

				mag = f / (float) sqrt(rsquared);

				s->p[s->nextParticle].delta[0].f[s->nextSubParticle] -= (dx * mag);
				s->p[s->nextParticle].delta[1].f[s->nextSubParticle] -= (dy * mag);
				s->p[s->nextParticle].delta[2].f[s->nextSubParticle] -= (dz * mag);
				s->p[s->nextParticle].color[0].f[s->nextSubParticle] = info->spark[i]->color[0] * (1.0f + RandBell(colorIncoherence));
				s->p[s->nextParticle].color[1].f[s->nextSubParticle] = info->spark[i]->color[1] * (1.0f + RandBell(colorIncoherence));
				s->p[s->nextParticle].color[2].f[s->nextSubParticle] = info->spark[i]->color[2] * (1.0f + RandBell(colorIncoherence));
				s->p[s->nextParticle].color[3].f[s->nextSubParticle] = 0.85f * (1.0f + RandBell(0.5f*colorIncoherence));
				s->p[s->nextParticle].time.f[s->nextSubParticle] = info->fTime;
				s->p[s->nextParticle].dead.i[s->nextSubParticle] = 0;
				s->p[s->nextParticle].animFrame.i[s->nextSubParticle] = random()&63;
				s->nextSubParticle++;
				if (s->nextSubParticle==4) {
					s->nextParticle++;
					s->nextSubParticle=0;
				}
				if (s->nextParticle >= NUMSMOKEPARTICLES/4) {
					s->nextParticle = 0;
					s->nextSubParticle = 0;
				}
			}

			s->lastParticleTime = info->fTime;
		}
	} else {
		s->lastParticleTime = info->fTime;
		s->firstTime = 0;
	}

	for(i=0;i<3;i++) {
		s->old[i] = info->star->position[i];
	}

	frameRate = ((double) info->dframe)/(info->fTime);
	frameRateModifier = 42.5f / frameRate;

	for(i=0;i<NUMSMOKEPARTICLES/4;i++) {
		for(k=0; k<4; k++) {
			float dx,dy,dz;
			float f;
			float rsquared;
			float mag;
			float deltax;
			float deltay;
			float deltaz;

			if (s->p[i].dead.i[k]) {
				continue;
			}

			deltax = s->p[i].delta[0].f[k];
			deltay = s->p[i].delta[1].f[k];
			deltaz = s->p[i].delta[2].f[k];

			for(j=0;j<info->numStreams;j++) {
				dx = s->p[i].position[0].f[k] - info->spark[j]->position[0];
				dy = s->p[i].position[1].f[k] - info->spark[j]->position[1];
				dz = s->p[i].position[2].f[k] - info->spark[j]->position[2];
				rsquared = (dx*dx+dy*dy+dz*dz);

				f = (gravity/rsquared) * frameRateModifier;

				if ((((i*4)+k) % info->numStreams) == j) {
					f *= 1.0f + streamBias;
				}

				mag = f / (float) sqrt(rsquared);

				deltax -= (dx * mag);
				deltay -= (dy * mag);
				deltaz -= (dz * mag);
			}

			/* slow this particle down by info->drag */
			deltax *= info->drag;
			deltay *= info->drag;
			deltaz *= info->drag;

			if((deltax*deltax+deltay*deltay+deltaz*deltaz) >= 25000000.0f) {
				s->p[i].dead.i[k] = 1;
				continue;
			}

			/* update the position */
			s->p[i].delta[0].f[k] = deltax;
			s->p[i].delta[1].f[k] = deltay;
			s->p[i].delta[2].f[k] = deltaz;
			for(j=0;j<3;j++) {
				s->p[i].oldposition[j].f[k] = s->p[i].position[j].f[k];
				s->p[i].position[j].f[k] += (s->p[i].delta[j].f[k])*info->fDeltaTime;
			}
		}
	}
}

void DrawSmoke_Scalar(flurry_info_t *info, SmokeV *s, float brightness)
{
	int svi = 0;
	int sci = 0;
	int sti = 0;
	int si = 0;
	float width;
	float sx,sy;
	float u0,v0,u1,v1;
	float w,z;
	float screenRatio = info->sys_glWidth / 1024.0f;
	float hslash2 = info->sys_glHeight * 0.5f;
	float wslash2 = info->sys_glWidth * 0.5f;
	int i,k;

	width = (streamSize+2.5f*info->streamExpansion) * screenRatio;

	for (i=0;i<NUMSMOKEPARTICLES/4;i++)
	{
		for (k=0; k<4; k++) {
			float thisWidth;
			float oldz;

			if (s->p[i].dead.i[k]) {
				continue;
			}
			thisWidth = (streamSize + (info->fTime - s->p[i].time.f[k])*info->streamExpansion) * screenRatio;
			if (thisWidth >= width)
			{
				s->p[i].dead.i[k] = 1;
				continue;
			}
			z = s->p[i].position[2].f[k];
			sx = s->p[i].position[0].f[k] * info->sys_glWidth / z + wslash2;
			sy = s->p[i].position[1].f[k] * info->sys_glWidth / z + hslash2;
			oldz = s->p[i].oldposition[2].f[k];
			if (sx > info->sys_glWidth+50.0f || sx < -50.0f || sy > info->sys_glHeight+50.0f || sy < -50.0f || z < 25.0f || oldz < 25.0f)
			{
				continue;
			}

			w = MAX_(1.0f,thisWidth/z);
			{
				float oldx = s->p[i].oldposition[0].f[k];
				float oldy = s->p[i].oldposition[1].f[k];
				float oldscreenx = (oldx * info->sys_glWidth / oldz) + wslash2;
				float oldscreeny = (oldy * info->sys_glWidth / oldz) + hslash2;
				float dx = (sx-oldscreenx);
				float dy = (sy-oldscreeny);

				float d = FastDistance2D(dx, dy);

				float sm, os, ow;
				if (d)
				{
					sm = w/d;
				}
				else
				{
					sm = 0.0f;
				}
				ow = MAX_(1.0f,thisWidth/oldz);
				if (d)
				{
					os = ow/d;
				}
				else
				{
					os = 0.0f;
				}

				{
					floatToVector cmv;
					float cm;
					float m = 1.0f + sm;

					float dxs = dx*sm;
					float dys = dy*sm;
					float dxos = dx*os;
					float dyos = dy*os;
					float dxm = dx*m;
					float dym = dy*m;

					s->p[i].animFrame.i[k]++;
					if (s->p[i].animFrame.i[k] >= 64)
					{
						s->p[i].animFrame.i[k] = 0;
					}

					u0 = (s->p[i].animFrame.i[k]&&7) * 0.125f;
					v0 = (s->p[i].animFrame.i[k]>>3) * 0.125f;
					u1 = u0 + 0.125f;
					v1 = v0 + 0.125f;
					u1 = u0 + 0.125f;
					v1 = v0 + 0.125f;
					cm = (1.375f - thisWidth/width);
					if (s->p[i].dead.i[k] == 3)
					{
						cm *= 0.125f;
						s->p[i].dead.i[k] = 1;
					}
					si++;
					cm *= brightness;
					cmv.f[0] = s->p[i].color[0].f[k]*cm;
					cmv.f[1] = s->p[i].color[1].f[k]*cm;
					cmv.f[2] = s->p[i].color[2].f[k]*cm;
					cmv.f[3] = s->p[i].color[3].f[k]*cm;

#if 0
					/* MDT we can't use vectors in the Scalar routine */
					s->seraphimColors[sci++].v = cmv.v;
					s->seraphimColors[sci++].v = cmv.v;
					s->seraphimColors[sci++].v = cmv.v;
					s->seraphimColors[sci++].v = cmv.v;
#else
					{
						int ii, jj;
						for (jj = 0; jj < 4; jj++) {
							for (ii = 0; ii < 4; ii++) {
								s->seraphimColors[sci].f[ii] = cmv.f[ii];
							}
							sci += 1;
						}
					}
#endif

					s->seraphimTextures[sti++] = u0;
					s->seraphimTextures[sti++] = v0;
					s->seraphimTextures[sti++] = u0;
					s->seraphimTextures[sti++] = v1;

					s->seraphimTextures[sti++] = u1;
					s->seraphimTextures[sti++] = v1;
					s->seraphimTextures[sti++] = u1;
					s->seraphimTextures[sti++] = v0;

					s->seraphimVertices[svi].f[0] = sx+dxm-dys;
					s->seraphimVertices[svi].f[1] = sy+dym+dxs;
					s->seraphimVertices[svi].f[2] = sx+dxm+dys;
					s->seraphimVertices[svi].f[3] = sy+dym-dxs;
					svi++;

					s->seraphimVertices[svi].f[0] = oldscreenx-dxm+dyos;
					s->seraphimVertices[svi].f[1] = oldscreeny-dym-dxos;
					s->seraphimVertices[svi].f[2] = oldscreenx-dxm-dyos;
					s->seraphimVertices[svi].f[3] = oldscreeny-dym+dxos;
					svi++;
				}
			}
		}
	}
	glColorPointer(4,GL_FLOAT,0,s->seraphimColors);
	glVertexPointer(2,GL_FLOAT,0,s->seraphimVertices);
	glTexCoordPointer(2,GL_FLOAT,0,s->seraphimTextures);
	glDrawArrays(GL_QUADS,0,si*4);
}
