/* Copyright (C) 2002 Jean-Marc Valin
   File: vq.c
   Vector quantization

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

#include "vq.h"

/*Finds the index of the entry in a codebook that best matches the input*/
int vq_index(float *in, float *codebook, int len, int entries)
{
   int i,j;
   float min_dist=0;
   int best_index=0;
   for (i=0;i<entries;i++)
   {
      float dist=0;
      for (j=0;j<len;j++)
      {
         float tmp = in[j]-*codebook++;
         dist += tmp*tmp;
      }
      if (i==0 || dist<min_dist)
      {
         min_dist=dist;
         best_index=i;
      }
   }
   return best_index;
}


/*Finds the indices of the n-best entries in a codebook*/
void vq_nbest(float *in, float *codebook, int len, int entries, float *E, int N, int *nbest, float *best_dist)
{
   int i,j,k,used;
   used = 0;
   for (i=0;i<entries;i++)
   {
      float dist=.5*E[i];
      for (j=0;j<len;j++)
         dist -= in[j]**codebook++;
      if (i<N || dist<best_dist[N-1])
      {
         for (k=N-1; (k >= 1) && (k > used || dist < best_dist[k-1]); k--)
         {
            best_dist[k]=best_dist[k-1];
            nbest[k] = nbest[k-1];
         }
         best_dist[k]=dist;
         nbest[k]=i;
         used++;
      }
   }
}

/*Finds the indices of the n-best entries in a codebook with sign*/
void vq_nbest_sign(float *in, float *codebook, int len, int entries, float *E, int N, int *nbest, float *best_dist)
{
   int i,j,k, sign, used;
   used=0;
   for (i=0;i<entries;i++)
   {
      float dist=0;
      for (j=0;j<len;j++)
         dist -= in[j]**codebook++;
      if (dist>0)
      {
         sign=1;
         dist=-dist;
      } else
      {
         sign=0;
      }
      dist += .5*E[i];
      if (i<N || dist<best_dist[N-1])
      {
         for (k=N-1; (k >= 1) && (k > used || dist < best_dist[k-1]); k--)
         {
            best_dist[k]=best_dist[k-1];
            nbest[k] = nbest[k-1];
         }
         best_dist[k]=dist;
         nbest[k]=i;
         used++;
         if (sign)
            nbest[k]+=entries;
      }
   }
}
