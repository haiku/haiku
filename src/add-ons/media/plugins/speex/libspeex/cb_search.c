/* Copyright (C) 2002 Jean-Marc Valin 
   File: cb_search.c

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


#include "cb_search.h"
#include "filters.h"
#include "stack_alloc.h"
#include "vq.h"
#include "misc.h"

void split_cb_search_shape_sign(
float target[],			/* target vector */
float ak[],			/* LPCs for this subframe */
float awk1[],			/* Weighted LPCs for this subframe */
float awk2[],			/* Weighted LPCs for this subframe */
void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
float *exc,
float *r,
SpeexBits *bits,
char *stack,
int   complexity
)
{
   int i,j,k,m,n,q;
   float *resp;
   float *t, *e, *E, *r2;
   float *tmp;
   float *ndist, *odist;
   int *itmp;
   float **ot, **nt;
   int **nind, **oind;
   int *ind;
   signed char *shape_cb;
   int shape_cb_size, subvect_size, nb_subvect;
   split_cb_params *params;
   int N=2;
   int *best_index;
   float *best_dist;
   int have_sign;

   N=complexity;
   if (N>10)
      N=10;

   ot=PUSH(stack, N, float*);
   nt=PUSH(stack, N, float*);
   oind=PUSH(stack, N, int*);
   nind=PUSH(stack, N, int*);

   params = (split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   shape_cb_size = 1<<params->shape_bits;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;
   resp = PUSH(stack, shape_cb_size*subvect_size, float);
   t = PUSH(stack, nsf, float);
   e = PUSH(stack, nsf, float);
   r2 = PUSH(stack, nsf, float);
   E = PUSH(stack, shape_cb_size, float);
   ind = PUSH(stack, nb_subvect, int);

   tmp = PUSH(stack, 2*N*nsf, float);
   for (i=0;i<N;i++)
   {
      ot[i]=tmp;
      tmp += nsf;
      nt[i]=tmp;
      tmp += nsf;
   }

   best_index = PUSH(stack, N, int);
   best_dist = PUSH(stack, N, float);
   ndist = PUSH(stack, N, float);
   odist = PUSH(stack, N, float);
   
   itmp = PUSH(stack, 2*N*nb_subvect, int);
   for (i=0;i<N;i++)
   {
      nind[i]=itmp;
      itmp+=nb_subvect;
      oind[i]=itmp;
      itmp+=nb_subvect;
      for (j=0;j<nb_subvect;j++)
         nind[i][j]=oind[i][j]=-1;
   }

   for (j=0;j<N;j++)
      for (i=0;i<nsf;i++)
         ot[j][i]=target[i];

   for (i=0;i<nsf;i++)
      t[i]=target[i];

   /* Pre-compute codewords response and energy */
   for (i=0;i<shape_cb_size;i++)
   {
      float *res;
      signed char *shape;

      res = resp+i*subvect_size;
      shape = shape_cb+i*subvect_size;

      /* Compute codeword response using convolution with impulse response */
      for(j=0;j<subvect_size;j++)
      {
         res[j]=0;
         for (k=0;k<=j;k++)
            res[j] += 0.03125*shape[k]*r[j-k];
      }
      
      /* Compute codeword energy */
      E[i]=0;
      for(j=0;j<subvect_size;j++)
         E[i]+=res[j]*res[j];
   }

   for (j=0;j<N;j++)
      odist[j]=0;
   /*For all subvectors*/
   for (i=0;i<nb_subvect;i++)
   {
      /*"erase" nbest list*/
      for (j=0;j<N;j++)
         ndist[j]=-1;

      /*For all n-bests of previous subvector*/
      for (j=0;j<N;j++)
      {
         float *x=ot[j]+subvect_size*i;
         /*Find new n-best based on previous n-best j*/
         if (have_sign)
            vq_nbest_sign(x, resp, subvect_size, shape_cb_size, E, N, best_index, best_dist);
         else
            vq_nbest(x, resp, subvect_size, shape_cb_size, E, N, best_index, best_dist);

         /*For all new n-bests*/
         for (k=0;k<N;k++)
         {
            float *ct;
            float err=0;
            ct = ot[j];
            /*update target*/

            /*previous target*/
            for (m=i*subvect_size;m<(i+1)*subvect_size;m++)
               t[m]=ct[m];

            /* New code: update only enough of the target to calculate error*/
            {
               int rind;
               float *res;
               float sign=1;
               rind = best_index[k];
               if (rind>=shape_cb_size)
               {
                  sign=-1;
                  rind-=shape_cb_size;
               }
               res = resp+rind*subvect_size;
               if (sign>0)
                  for (m=0;m<subvect_size;m++)
                     t[subvect_size*i+m] -= res[m];
               else
                  for (m=0;m<subvect_size;m++)
                     t[subvect_size*i+m] += res[m];
            }
            
            /*compute error (distance)*/
            err=odist[j];
            for (m=i*subvect_size;m<(i+1)*subvect_size;m++)
               err += t[m]*t[m];
            /*update n-best list*/
            if (err<ndist[N-1] || ndist[N-1]<-.5)
            {

               /*previous target (we don't care what happened before*/
               for (m=(i+1)*subvect_size;m<nsf;m++)
                  t[m]=ct[m];
               /* New code: update the rest of the target only if it's worth it */
               for (m=0;m<subvect_size;m++)
               {
                  float g;
                  int rind;
                  float sign=1;
                  rind = best_index[k];
                  if (rind>=shape_cb_size)
                  {
                     sign=-1;
                     rind-=shape_cb_size;
                  }

                  g=sign*0.03125*shape_cb[rind*subvect_size+m];
                  q=subvect_size-m;
                  for (n=subvect_size*(i+1);n<nsf;n++,q++)
                     t[n] -= g*r[q];
               }


               for (m=0;m<N;m++)
               {
                  if (err < ndist[m] || ndist[m]<-.5)
                  {
                     for (n=N-1;n>m;n--)
                     {
                        for (q=(i+1)*subvect_size;q<nsf;q++)
                           nt[n][q]=nt[n-1][q];
                        for (q=0;q<nb_subvect;q++)
                           nind[n][q]=nind[n-1][q];
                        ndist[n]=ndist[n-1];
                     }
                     for (q=(i+1)*subvect_size;q<nsf;q++)
                        nt[m][q]=t[q];
                     for (q=0;q<nb_subvect;q++)
                        nind[m][q]=oind[j][q];
                     nind[m][i]=best_index[k];
                     ndist[m]=err;
                     break;
                  }
               }
            }
         }
         if (i==0)
           break;
      }

      /*update old-new data*/
      /* just swap pointers instead of a long copy */
      {
         float **tmp2;
         tmp2=ot;
         ot=nt;
         nt=tmp2;
      }
      for (j=0;j<N;j++)
         for (m=0;m<nb_subvect;m++)
            oind[j][m]=nind[j][m];
      for (j=0;j<N;j++)
         odist[j]=ndist[j];
   }

   /*save indices*/
   for (i=0;i<nb_subvect;i++)
   {
      ind[i]=nind[0][i];
      speex_bits_pack(bits,ind[i],params->shape_bits+have_sign);
   }
   
   /* Put everything back together */
   for (i=0;i<nb_subvect;i++)
   {
      int rind;
      float sign=1;
      rind = ind[i];
      if (rind>=shape_cb_size)
      {
         sign=-1;
         rind-=shape_cb_size;
      }

      for (j=0;j<subvect_size;j++)
         e[subvect_size*i+j]=sign*0.03125*shape_cb[rind*subvect_size+j];
   }   
   /* Update excitation */
   for (j=0;j<nsf;j++)
      exc[j]+=e[j];
   
   /* Update target */
   syn_percep_zero(e, ak, awk1, awk2, r2, nsf,p, stack);
   for (j=0;j<nsf;j++)
      target[j]-=r2[j];

}


void split_cb_shape_sign_unquant(
float *exc,
void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack
)
{
   int i,j;
   int *ind, *signs;
   signed char *shape_cb;
   int shape_cb_size, subvect_size, nb_subvect;
   split_cb_params *params;
   int have_sign;

   params = (split_cb_params *) par;
   subvect_size = params->subvect_size;
   nb_subvect = params->nb_subvect;
   shape_cb_size = 1<<params->shape_bits;
   shape_cb = params->shape_cb;
   have_sign = params->have_sign;

   ind = PUSH(stack, nb_subvect, int);
   signs = PUSH(stack, nb_subvect, int);

   /* Decode codewords and gains */
   for (i=0;i<nb_subvect;i++)
   {
      if (have_sign)
         signs[i] = speex_bits_unpack_unsigned(bits, 1);
      else
         signs[i] = 0;
      ind[i] = speex_bits_unpack_unsigned(bits, params->shape_bits);
   }
   /* Compute decoded excitation */
   for (i=0;i<nb_subvect;i++)
   {
      float s=1;
      if (signs[i])
         s=-1;
      for (j=0;j<subvect_size;j++)
         exc[subvect_size*i+j]+=s*0.03125*shape_cb[ind[i]*subvect_size+j];
   }
}

void noise_codebook_quant(
float target[],			/* target vector */
float ak[],			/* LPCs for this subframe */
float awk1[],			/* Weighted LPCs for this subframe */
float awk2[],			/* Weighted LPCs for this subframe */
void *par,                      /* Codebook/search parameters*/
int   p,                        /* number of LPC coeffs */
int   nsf,                      /* number of samples in subframe */
float *exc,
float *r,
SpeexBits *bits,
char *stack,
int   complexity
)
{
   int i;
   float *tmp=PUSH(stack, nsf, float);
   residue_percep_zero(target, ak, awk1, awk2, tmp, nsf, p, stack);

   for (i=0;i<nsf;i++)
      exc[i]+=tmp[i];
   for (i=0;i<nsf;i++)
      target[i]=0;

}


void noise_codebook_unquant(
float *exc,
void *par,                      /* non-overlapping codebook */
int   nsf,                      /* number of samples in subframe */
SpeexBits *bits,
char *stack
)
{
   speex_rand_vec(1, exc, nsf);
}
