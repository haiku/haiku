/* Copyright (C) 2002 Jean-Marc Valin 
   File: ltp.c
   Long-Term Prediction functions

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
#include "ltp.h"
#include "stack_alloc.h"
#include "filters.h"
#include "speex_bits.h"

#ifdef _USE_SSE
#include "ltp_sse.h"
#else
static float inner_prod(float *x, float *y, int len)
{
   int i;
   float sum1=0,sum2=0,sum3=0,sum4=0;
   for (i=0;i<len;)
   {
      sum1 += x[i]*y[i];
      sum2 += x[i+1]*y[i+1];
      sum3 += x[i+2]*y[i+2];
      sum4 += x[i+3]*y[i+3];
      i+=4;
   }
   return sum1+sum2+sum3+sum4;
}
#endif

/*Original, non-optimized version*/
/*static float inner_prod(float *x, float *y, int len)
{
   int i;
   float sum=0;
   for (i=0;i<len;i++)
      sum += x[i]*y[i];
   return sum;
}
*/


void open_loop_nbest_pitch(float *sw, int start, int end, int len, int *pitch, float *gain, int N, char *stack)
{
   int i,j,k;
   /*float corr=0;*/
   /*float energy;*/
   float *best_score;
   float e0;
   float *corr, *energy, *score;

   best_score = PUSH(stack,N, float);
   corr = PUSH(stack,end-start+1, float);
   energy = PUSH(stack,end-start+2, float);
   score = PUSH(stack,end-start+1, float);
   for (i=0;i<N;i++)
   {
        best_score[i]=-1;
        gain[i]=0;
   }
   energy[0]=inner_prod(sw-start, sw-start, len);
   e0=inner_prod(sw, sw, len);
   for (i=start;i<=end;i++)
   {
      /* Update energy for next pitch*/
      energy[i-start+1] = energy[i-start] + sw[-i-1]*sw[-i-1] - sw[-i+len-1]*sw[-i+len-1];
   }
   for (i=start;i<=end;i++)
   {
      corr[i-start]=0;
      score[i-start]=0;
   }

   for (i=start;i<=end;i++)
   {
      /* Compute correlation*/
      corr[i-start]=inner_prod(sw, sw-i, len);
      score[i-start]=corr[i-start]*corr[i-start]/(energy[i-start]+1);
   }
   for (i=start;i<=end;i++)
   {
      if (score[i-start]>best_score[N-1])
      {
         float g1, g;
         g1 = corr[i-start]/(energy[i-start]+10);
         g = sqrt(g1*corr[i-start]/(e0+10));
         if (g>g1)
            g=g1;
         if (g<0)
            g=0;
         for (j=0;j<N;j++)
         {
            if (score[i-start] > best_score[j])
            {
               for (k=N-1;k>j;k--)
               {
                  best_score[k]=best_score[k-1];
                  pitch[k]=pitch[k-1];
                  gain[k] = gain[k-1];
               }
               best_score[j]=score[i-start];
               pitch[j]=i;
               gain[j]=g;
               break;
            }
         }
      }
   }

}




/** Finds the best quantized 3-tap pitch predictor by analysis by synthesis */
float pitch_gain_search_3tap(
float target[],                 /* Target vector */
float ak[],                     /* LPCs for this subframe */
float awk1[],                   /* Weighted LPCs #1 for this subframe */
float awk2[],                   /* Weighted LPCs #2 for this subframe */
float exc[],                    /* Excitation */
void *par,
int   pitch,                    /* Pitch value */
int   p,                        /* Number of LPC coeffs */
int   nsf,                      /* Number of samples in subframe */
SpeexBits *bits,
char *stack,
float *exc2,
float *r,
int  *cdbk_index
)
{
   int i,j;
   float *tmp, *tmp2;
   float *x[3];
   float *e[3];
   float corr[3];
   float A[3][3];
   float gain[3];
   int   gain_cdbk_size;
   signed char *gain_cdbk;
   float err1,err2;

   ltp_params *params;
   params = (ltp_params*) par;
   gain_cdbk=params->gain_cdbk;
   gain_cdbk_size=1<<params->gain_bits;
   tmp = PUSH(stack, 3*nsf, float);
   tmp2 = PUSH(stack, 3*nsf, float);

   x[0]=tmp;
   x[1]=tmp+nsf;
   x[2]=tmp+2*nsf;

   e[0]=tmp2;
   e[1]=tmp2+nsf;
   e[2]=tmp2+2*nsf;
   
   for (i=2;i>=0;i--)
   {
      int pp=pitch+1-i;
      for (j=0;j<nsf;j++)
      {
         if (j-pp<0)
            e[i][j]=exc2[j-pp];
         else if (j-pp-pitch<0)
            e[i][j]=exc2[j-pp-pitch];
         else
            e[i][j]=0;
      }

      if (i==2)
         syn_percep_zero(e[i], ak, awk1, awk2, x[i], nsf, p, stack);
      else {
         for (j=0;j<nsf-1;j++)
            x[i][j+1]=x[i+1][j];
         x[i][0]=0;
         for (j=0;j<nsf;j++)
            x[i][j]+=e[i][0]*r[j];
      }
   }

   for (i=0;i<3;i++)
      corr[i]=inner_prod(x[i],target,nsf);
   
   for (i=0;i<3;i++)
      for (j=0;j<=i;j++)
         A[i][j]=A[j][i]=inner_prod(x[i],x[j],nsf);
   
   {
      float C[9];
      signed char *ptr=gain_cdbk;
      int best_cdbk=0;
      float best_sum=0;
      C[0]=corr[2];
      C[1]=corr[1];
      C[2]=corr[0];
      C[3]=A[1][2];
      C[4]=A[0][1];
      C[5]=A[0][2];
      C[6]=A[2][2];
      C[7]=A[1][1];
      C[8]=A[0][0];
      
      for (i=0;i<gain_cdbk_size;i++)
      {
         float sum=0;
         float g0,g1,g2;
         ptr = gain_cdbk+3*i;
         g0=0.015625*ptr[0]+.5;
         g1=0.015625*ptr[1]+.5;
         g2=0.015625*ptr[2]+.5;

         sum += C[0]*g0;
         sum += C[1]*g1;
         sum += C[2]*g2;
         sum -= C[3]*g0*g1;
         sum -= C[4]*g2*g1;
         sum -= C[5]*g2*g0;
         sum -= .5*C[6]*g0*g0;
         sum -= .5*C[7]*g1*g1;
         sum -= .5*C[8]*g2*g2;

         /* If 1, force "safe" pitch values to handle packet loss better */
         if (0) {
            float tot = fabs(ptr[1]);
            if (ptr[0]>0)
               tot+=ptr[0];
            if (ptr[2]>0)
               tot+=ptr[2];
            if (tot>1)
               continue;
         }

         if (sum>best_sum || i==0)
         {
            best_sum=sum;
            best_cdbk=i;
         }
      }
      gain[0] = 0.015625*gain_cdbk[best_cdbk*3]  + .5;
      gain[1] = 0.015625*gain_cdbk[best_cdbk*3+1]+ .5;
      gain[2] = 0.015625*gain_cdbk[best_cdbk*3+2]+ .5;

      *cdbk_index=best_cdbk;
   }
   
   for (i=0;i<nsf;i++)
      exc[i]=gain[0]*e[2][i]+gain[1]*e[1][i]+gain[2]*e[0][i];
   
   err1=0;
   err2=0;
   for (i=0;i<nsf;i++)
      err1+=target[i]*target[i];
   for (i=0;i<nsf;i++)
      err2+=(target[i]-gain[2]*x[0][i]-gain[1]*x[1][i]-gain[0]*x[2][i])
      * (target[i]-gain[2]*x[0][i]-gain[1]*x[1][i]-gain[0]*x[2][i]);

   return err2;
}


/** Finds the best quantized 3-tap pitch predictor by analysis by synthesis */
int pitch_search_3tap(
float target[],                 /* Target vector */
float *sw,
float ak[],                     /* LPCs for this subframe */
float awk1[],                   /* Weighted LPCs #1 for this subframe */
float awk2[],                   /* Weighted LPCs #2 for this subframe */
float exc[],                    /* Excitation */
void *par,
int   start,                    /* Smallest pitch value allowed */
int   end,                      /* Largest pitch value allowed */
float pitch_coef,               /* Voicing (pitch) coefficient */
int   p,                        /* Number of LPC coeffs */
int   nsf,                      /* Number of samples in subframe */
SpeexBits *bits,
char *stack,
float *exc2,
float *r,
int complexity
)
{
   int i,j;
   int cdbk_index, pitch=0, best_gain_index=0;
   float *best_exc;
   int best_pitch=0;
   float err, best_err=-1;
   int N;
   ltp_params *params;
   int *nbest;
   float *gains;

   N=complexity;
   if (N>10)
      N=10;

   nbest=PUSH(stack, N, int);
   gains = PUSH(stack, N, float);
   params = (ltp_params*) par;

   if (N==0 || end<start)
   {
      speex_bits_pack(bits, 0, params->pitch_bits);
      speex_bits_pack(bits, 0, params->gain_bits);
      for (i=0;i<nsf;i++)
         exc[i]=0;
      return start;
   }
   
   best_exc=PUSH(stack,nsf, float);
   
   if (N>end-start+1)
      N=end-start+1;
   open_loop_nbest_pitch(sw, start, end, nsf, nbest, gains, N, stack);
   for (i=0;i<N;i++)
   {
      pitch=nbest[i];
      for (j=0;j<nsf;j++)
         exc[j]=0;
      err=pitch_gain_search_3tap(target, ak, awk1, awk2, exc, par, pitch, p, nsf,
                                 bits, stack, exc2, r, &cdbk_index);
      if (err<best_err || best_err<0)
      {
         for (j=0;j<nsf;j++)
            best_exc[j]=exc[j];
         best_err=err;
         best_pitch=pitch;
         best_gain_index=cdbk_index;
      }
   }
   
   /*printf ("pitch: %d %d\n", best_pitch, best_gain_index);*/
   speex_bits_pack(bits, best_pitch-start, params->pitch_bits);
   speex_bits_pack(bits, best_gain_index, params->gain_bits);
   /*printf ("encode pitch: %d %d\n", best_pitch, best_gain_index);*/
   for (i=0;i<nsf;i++)
      exc[i]=best_exc[i];

   return pitch;
}

void pitch_unquant_3tap(
float exc[],                    /* Excitation */
int   start,                    /* Smallest pitch value allowed */
int   end,                      /* Largest pitch value allowed */
float pitch_coef,               /* Voicing (pitch) coefficient */
void *par,
int   nsf,                      /* Number of samples in subframe */
int *pitch_val,
float *gain_val,
SpeexBits *bits,
char *stack,
int count_lost,
int subframe_offset,
float last_pitch_gain)
{
   int i;
   int pitch;
   int gain_index;
   float gain[3];
   signed char *gain_cdbk;
   ltp_params *params;
   params = (ltp_params*) par;
   gain_cdbk=params->gain_cdbk;

   pitch = speex_bits_unpack_unsigned(bits, params->pitch_bits);
   pitch += start;
   gain_index = speex_bits_unpack_unsigned(bits, params->gain_bits);
   /*printf ("decode pitch: %d %d\n", pitch, gain_index);*/
   gain[0] = 0.015625*gain_cdbk[gain_index*3]+.5;
   gain[1] = 0.015625*gain_cdbk[gain_index*3+1]+.5;
   gain[2] = 0.015625*gain_cdbk[gain_index*3+2]+.5;

   if (count_lost && pitch > subframe_offset)
   {
      float gain_sum;

      if (1) {
	 float tmp = count_lost < 4 ? last_pitch_gain : 0.4 * last_pitch_gain;
         if (tmp>.95)
            tmp=.95;
         gain_sum = fabs(gain[1]);
         if (gain[0]>0)
            gain_sum += gain[0];
         else
            gain_sum -= .5*gain[0];
         if (gain[2]>0)
            gain_sum += gain[2];
         else
            gain_sum -= .5*gain[2];
	 if (gain_sum > tmp) {
	    float fact = tmp/gain_sum;
	    for (i=0;i<3;i++)
	       gain[i]*=fact;

	 }

      }

      if (0) {
      gain_sum = fabs(gain[0])+fabs(gain[1])+fabs(gain[2]);
	 if (gain_sum>.95) {
         float fact = .95/gain_sum;
         for (i=0;i<3;i++)
            gain[i]*=fact;
      }
   }
   }

   *pitch_val = pitch;
   /**gain_val = gain[0]+gain[1]+gain[2];*/
   gain_val[0]=gain[0];
   gain_val[1]=gain[1];
   gain_val[2]=gain[2];

   {
      float *e[3];
      float *tmp2;
      tmp2=PUSH(stack, 3*nsf, float);
      e[0]=tmp2;
      e[1]=tmp2+nsf;
      e[2]=tmp2+2*nsf;
      
      for (i=0;i<3;i++)
      {
         int j;
         int pp=pitch+1-i;
#if 0
         for (j=0;j<nsf;j++)
         {
            if (j-pp<0)
               e[i][j]=exc[j-pp];
            else if (j-pp-pitch<0)
               e[i][j]=exc[j-pp-pitch];
            else
               e[i][j]=0;
         }
#else
         {
            int tmp1, tmp3;
            tmp1=nsf;
            if (tmp1>pp)
               tmp1=pp;
            for (j=0;j<tmp1;j++)
               e[i][j]=exc[j-pp];
            tmp3=nsf;
            if (tmp3>pp+pitch)
               tmp3=pp+pitch;
            for (j=tmp1;j<tmp3;j++)
               e[i][j]=exc[j-pp-pitch];
            for (j=tmp3;j<nsf;j++)
               e[i][j]=0;
         }
#endif
      }
      for (i=0;i<nsf;i++)
           exc[i]=gain[0]*e[2][i]+gain[1]*e[1][i]+gain[2]*e[0][i];
   }
}


/** Forced pitch delay and gain */
int forced_pitch_quant(
float target[],                 /* Target vector */
float *sw,
float ak[],                     /* LPCs for this subframe */
float awk1[],                   /* Weighted LPCs #1 for this subframe */
float awk2[],                   /* Weighted LPCs #2 for this subframe */
float exc[],                    /* Excitation */
void *par,
int   start,                    /* Smallest pitch value allowed */
int   end,                      /* Largest pitch value allowed */
float pitch_coef,               /* Voicing (pitch) coefficient */
int   p,                        /* Number of LPC coeffs */
int   nsf,                      /* Number of samples in subframe */
SpeexBits *bits,
char *stack,
float *exc2,
float *r,
int complexity
)
{
   int i;
   if (pitch_coef>.99)
      pitch_coef=.99;
   for (i=0;i<nsf;i++)
   {
      exc[i]=exc[i-start]*pitch_coef;
   }
   return start;
}

/** Unquantize forced pitch delay and gain */
void forced_pitch_unquant(
float exc[],                    /* Excitation */
int   start,                    /* Smallest pitch value allowed */
int   end,                      /* Largest pitch value allowed */
float pitch_coef,               /* Voicing (pitch) coefficient */
void *par,
int   nsf,                      /* Number of samples in subframe */
int *pitch_val,
float *gain_val,
SpeexBits *bits,
char *stack,
int count_lost,
int subframe_offset,
float last_pitch_gain)
{
   int i;
   /*pitch_coef=.9;*/
   if (pitch_coef>.99)
      pitch_coef=.99;
   for (i=0;i<nsf;i++)
   {
      exc[i]=exc[i-start]*pitch_coef;
   }
   *pitch_val = start;
   gain_val[0]=gain_val[2]=0;
   gain_val[1] = pitch_coef;
}
