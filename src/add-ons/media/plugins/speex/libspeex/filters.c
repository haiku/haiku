/* Copyright (C) 2002 Jean-Marc Valin 
   File: filters.c
   Various analysis/synthesis filters

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

#include "filters.h"
#include "stack_alloc.h"
#include <math.h>


void bw_lpc(float gamma, float *lpc_in, float *lpc_out, int order)
{
   int i;
   float tmp=1;
   for (i=0;i<order+1;i++)
   {
      lpc_out[i] = tmp * lpc_in[i];
      tmp *= gamma;
   }
}

#ifdef _USE_SSE
#include "filters_sse.h"
#else
void filter_mem2(float *x, float *num, float *den, float *y, int N, int ord, float *mem)
{
   int i,j;
   float xi,yi;
   for (i=0;i<N;i++)
   {
      xi=x[i];
      y[i] = num[0]*xi + mem[0];
      yi=y[i];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] + num[j+1]*xi - den[j+1]*yi;
      }
      mem[ord-1] = num[ord]*xi - den[ord]*yi;
   }
}


void iir_mem2(float *x, float *den, float *y, int N, int ord, float *mem)
{
   int i,j;
   for (i=0;i<N;i++)
   {
      y[i] = x[i] + mem[0];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] - den[j+1]*y[i];
      }
      mem[ord-1] = - den[ord]*y[i];
   }
}
#endif

void fir_mem2(float *x, float *num, float *y, int N, int ord, float *mem)
{
   int i,j;
   float xi;
   for (i=0;i<N;i++)
   {
      xi=x[i];
      y[i] = num[0]*xi + mem[0];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] + num[j+1]*xi;
      }
      mem[ord-1] = num[ord]*xi;
   }
}

void syn_percep_zero(float *xx, float *ak, float *awk1, float *awk2, float *y, int N, int ord, char *stack)
{
   int i;
   float *mem = PUSH(stack,ord, float);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(xx, awk1, ak, y, N, ord, mem);
   for (i=0;i<ord;i++)
     mem[i]=0;
   iir_mem2(y, awk2, y, N, ord, mem);
}

void residue_percep_zero(float *xx, float *ak, float *awk1, float *awk2, float *y, int N, int ord, char *stack)
{
   int i;
   float *mem = PUSH(stack,ord, float);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(xx, ak, awk1, y, N, ord, mem);
   for (i=0;i<ord;i++)
     mem[i]=0;
   fir_mem2(y, awk2, y, N, ord, mem);
}


void qmf_decomp(float *xx, float *aa, float *y1, float *y2, int N, int M, float *mem, char *stack)
{
   int i,j,k,M2;
   float *a;
   float *x;
   float *x2;
   
   a = PUSH(stack, M, float);
   x = PUSH(stack, N+M-1, float);
   x2=x+M-1;
   M2=M>>1;
   for (i=0;i<M;i++)
      a[M-i-1]=aa[i];
   for (i=0;i<M-1;i++)
      x[i]=mem[M-i-2];
   for (i=0;i<N;i++)
      x[i+M-1]=xx[i];
   for (i=0,k=0;i<N;i+=2,k++)
   {
      y1[k]=0;
      y2[k]=0;
      for (j=0;j<M2;j++)
      {
         y1[k]+=a[j]*(x[i+j]+x2[i-j]);
         y2[k]-=a[j]*(x[i+j]-x2[i-j]);
         j++;
         y1[k]+=a[j]*(x[i+j]+x2[i-j]);
         y2[k]+=a[j]*(x[i+j]-x2[i-j]);
      }
   }
   for (i=0;i<M-1;i++)
     mem[i]=xx[N-i-1];
}

/* By segher */
void fir_mem_up(float *x, float *a, float *y, int N, int M, float *mem, char *stack)
   /* assumptions:
      all odd x[i] are zero -- well, actually they are left out of the array now
      N and M are multiples of 4 */
{
   int i, j;
   float *xx=PUSH(stack, M+N-1, float);

   for (i = 0; i < N/2; i++)
      xx[2*i] = x[N/2-1-i];
   for (i = 0; i < M - 1; i += 2)
      xx[N+i] = mem[i+1];

   for (i = 0; i < N; i += 4) {
      float y0, y1, y2, y3;
      float x0;

      y0 = y1 = y2 = y3 = 0.f;
      x0 = xx[N-4-i];

      for (j = 0; j < M; j += 4) {
         float x1;
         float a0, a1;

         a0 = a[j];
         a1 = a[j+1];
         x1 = xx[N-2+j-i];

         y0 += a0 * x1;
         y1 += a1 * x1;
         y2 += a0 * x0;
         y3 += a1 * x0;

         a0 = a[j+2];
         a1 = a[j+3];
         x0 = xx[N+j-i];

         y0 += a0 * x0;
         y1 += a1 * x0;
         y2 += a0 * x1;
         y3 += a1 * x1;
      }
      y[i] = y0;
      y[i+1] = y1;
      y[i+2] = y2;
      y[i+3] = y3;
   }

   for (i = 0; i < M - 1; i += 2)
      mem[i+1] = xx[i];
}


void comp_filter_mem_init (CombFilterMem *mem)
{
   mem->last_pitch=0;
   mem->last_pitch_gain[0]=mem->last_pitch_gain[1]=mem->last_pitch_gain[2]=0;
   mem->smooth_gain=1;
}

void comb_filter(
float *exc,          /*decoded excitation*/
float *new_exc,      /*enhanced excitation*/
float *ak,           /*LPC filter coefs*/
int p,               /*LPC order*/
int nsf,             /*sub-frame size*/
int pitch,           /*pitch period*/
float *pitch_gain,   /*pitch gain (3-tap)*/
float  comb_gain,    /*gain of comb filter*/
CombFilterMem *mem
)
{
   int i;
   float exc_energy=0, new_exc_energy=0;
   float gain;
   float step;
   float fact;
   /*Compute excitation energy prior to enhancement*/
   for (i=0;i<nsf;i++)
      exc_energy+=exc[i]*exc[i];

   /*Some gain adjustment is pitch is too high or if unvoiced*/
   {
      float g=0;
      g = .5*fabs(pitch_gain[0]+pitch_gain[1]+pitch_gain[2] +
      mem->last_pitch_gain[0] + mem->last_pitch_gain[1] + mem->last_pitch_gain[2]);
      if (g>1.3)
         comb_gain*=1.3/g;
      if (g<.5)
         comb_gain*=2*g;
   }
   step = 1.0/nsf;
   fact=0;
   /*Apply pitch comb-filter (filter out noise between pitch harmonics)*/
   for (i=0;i<nsf;i++)
   {
      fact += step;

      new_exc[i] = exc[i] + comb_gain * fact * (
                                         pitch_gain[0]*exc[i-pitch+1] +
                                         pitch_gain[1]*exc[i-pitch] +
                                         pitch_gain[2]*exc[i-pitch-1]
                                         )
      + comb_gain * (1-fact) * (
                                         mem->last_pitch_gain[0]*exc[i-mem->last_pitch+1] +
                                         mem->last_pitch_gain[1]*exc[i-mem->last_pitch] +
                                         mem->last_pitch_gain[2]*exc[i-mem->last_pitch-1]
                                         );
   }

   mem->last_pitch_gain[0] = pitch_gain[0];
   mem->last_pitch_gain[1] = pitch_gain[1];
   mem->last_pitch_gain[2] = pitch_gain[2];
   mem->last_pitch = pitch;

   /*Gain after enhancement*/
   for (i=0;i<nsf;i++)
      new_exc_energy+=new_exc[i]*new_exc[i];

   /*Compute scaling factor and normalize energy*/
   gain = sqrt(exc_energy)/sqrt(.1+new_exc_energy);
   if (gain < .5)
      gain=.5;
   if (gain>1)
      gain=1;

   for (i=0;i<nsf;i++)
   {
      mem->smooth_gain = .96*mem->smooth_gain + .04*gain;
      new_exc[i] *= mem->smooth_gain;
   }
}
