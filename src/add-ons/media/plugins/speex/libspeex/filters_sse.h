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

void filter_mem2(float *x, float *_num, float *_den, float *y, int N, int ord, float *_mem)
{
   float __num[20], __den[20], __mem[20];
   float *num, *den, *mem;
   int i;

   num = (float*)(((int)(__num+4))&0xfffffff0)-1;
   den = (float*)(((int)(__den+4))&0xfffffff0)-1;
   mem = (float*)(((int)(__mem+4))&0xfffffff0)-1;
   for (i=0;i<=10;i++)
      num[i]=den[i]=0;
   for (i=0;i<10;i++)
      mem[i]=0;

   for (i=0;i<ord+1;i++)
   {
      num[i]=_num[i];
      den[i]=_den[i];
   }
   for (i=0;i<ord;i++)
      mem[i]=_mem[i];
   for (i=0;i<N;i+=4)
   {

      __asm__ __volatile__ 
      (
       "\tmovss (%1), %%xmm0\n"
       "\tmovss (%0), %%xmm1\n"
       "\taddss %%xmm0, %%xmm1\n"
       "\tmovss %%xmm1, (%2)\n"
       "\tshufps $0x00, %%xmm0, %%xmm0\n"
       "\tshufps $0x00, %%xmm1, %%xmm1\n"

       "\tmovaps 4(%3),  %%xmm2\n"
       "\tmovaps 4(%4),  %%xmm3\n"
       "\tmulps  %%xmm0, %%xmm2\n"
       "\tmulps  %%xmm1, %%xmm3\n"
       "\tmovaps 20(%3), %%xmm4\n"
       "\tmulps  %%xmm0, %%xmm4\n"
       "\taddps  4(%0),  %%xmm2\n"
       "\tmovaps 20(%4), %%xmm5\n"
       "\tmulps  %%xmm1, %%xmm5\n"
       "\taddps  20(%0), %%xmm4\n"
       "\tsubps  %%xmm3, %%xmm2\n"
       "\tmovups %%xmm2, (%0)\n"
       "\tsubps  %%xmm5, %%xmm4\n"
       "\tmovups %%xmm4, 16(%0)\n"

       "\tmovss  36(%3), %%xmm2\n"
       "\tmulss  %%xmm0, %%xmm2\n"
       "\tmovss  36(%4), %%xmm3\n"
       "\tmulss  %%xmm1, %%xmm3\n"
       "\taddss  36(%0), %%xmm2\n"
       "\tmovss  40(%3), %%xmm4\n"
       "\tmulss  %%xmm0, %%xmm4\n"
       "\tmovss  40(%4), %%xmm5\n"
       "\tmulss  %%xmm1, %%xmm5\n"
       "\tsubss  %%xmm3, %%xmm2\n"
       "\tmovss  %%xmm2, 32(%0)       \n"
       "\tsubss  %%xmm5, %%xmm4\n"
       "\tmovss  %%xmm4, 36(%0)\n"



       "\tmovss 4(%1), %%xmm0\n"
       "\tmovss (%0), %%xmm1\n"
       "\taddss %%xmm0, %%xmm1\n"
       "\tmovss %%xmm1, 4(%2)\n"
       "\tshufps $0x00, %%xmm0, %%xmm0\n"
       "\tshufps $0x00, %%xmm1, %%xmm1\n"

       "\tmovaps 4(%3),  %%xmm2\n"
       "\tmovaps 4(%4),  %%xmm3\n"
       "\tmulps  %%xmm0, %%xmm2\n"
       "\tmulps  %%xmm1, %%xmm3\n"
       "\tmovaps 20(%3), %%xmm4\n"
       "\tmulps  %%xmm0, %%xmm4\n"
       "\taddps  4(%0),  %%xmm2\n"
       "\tmovaps 20(%4), %%xmm5\n"
       "\tmulps  %%xmm1, %%xmm5\n"
       "\taddps  20(%0), %%xmm4\n"
       "\tsubps  %%xmm3, %%xmm2\n"
       "\tmovups %%xmm2, (%0)\n"
       "\tsubps  %%xmm5, %%xmm4\n"
       "\tmovups %%xmm4, 16(%0)\n"

       "\tmovss  36(%3), %%xmm2\n"
       "\tmulss  %%xmm0, %%xmm2\n"
       "\tmovss  36(%4), %%xmm3\n"
       "\tmulss  %%xmm1, %%xmm3\n"
       "\taddss  36(%0), %%xmm2\n"
       "\tmovss  40(%3), %%xmm4\n"
       "\tmulss  %%xmm0, %%xmm4\n"
       "\tmovss  40(%4), %%xmm5\n"
       "\tmulss  %%xmm1, %%xmm5\n"
       "\tsubss  %%xmm3, %%xmm2\n"
       "\tmovss  %%xmm2, 32(%0)       \n"
       "\tsubss  %%xmm5, %%xmm4\n"
       "\tmovss  %%xmm4, 36(%0)\n"



       "\tmovss 8(%1), %%xmm0\n"
       "\tmovss (%0), %%xmm1\n"
       "\taddss %%xmm0, %%xmm1\n"
       "\tmovss %%xmm1, 8(%2)\n"
       "\tshufps $0x00, %%xmm0, %%xmm0\n"
       "\tshufps $0x00, %%xmm1, %%xmm1\n"

       "\tmovaps 4(%3),  %%xmm2\n"
       "\tmovaps 4(%4),  %%xmm3\n"
       "\tmulps  %%xmm0, %%xmm2\n"
       "\tmulps  %%xmm1, %%xmm3\n"
       "\tmovaps 20(%3), %%xmm4\n"
       "\tmulps  %%xmm0, %%xmm4\n"
       "\taddps  4(%0),  %%xmm2\n"
       "\tmovaps 20(%4), %%xmm5\n"
       "\tmulps  %%xmm1, %%xmm5\n"
       "\taddps  20(%0), %%xmm4\n"
       "\tsubps  %%xmm3, %%xmm2\n"
       "\tmovups %%xmm2, (%0)\n"
       "\tsubps  %%xmm5, %%xmm4\n"
       "\tmovups %%xmm4, 16(%0)\n"

       "\tmovss  36(%3), %%xmm2\n"
       "\tmulss  %%xmm0, %%xmm2\n"
       "\tmovss  36(%4), %%xmm3\n"
       "\tmulss  %%xmm1, %%xmm3\n"
       "\taddss  36(%0), %%xmm2\n"
       "\tmovss  40(%3), %%xmm4\n"
       "\tmulss  %%xmm0, %%xmm4\n"
       "\tmovss  40(%4), %%xmm5\n"
       "\tmulss  %%xmm1, %%xmm5\n"
       "\tsubss  %%xmm3, %%xmm2\n"
       "\tmovss  %%xmm2, 32(%0)       \n"
       "\tsubss  %%xmm5, %%xmm4\n"
       "\tmovss  %%xmm4, 36(%0)\n"



       "\tmovss 12(%1), %%xmm0\n"
       "\tmovss (%0), %%xmm1\n"
       "\taddss %%xmm0, %%xmm1\n"
       "\tmovss %%xmm1, 12(%2)\n"
       "\tshufps $0x00, %%xmm0, %%xmm0\n"
       "\tshufps $0x00, %%xmm1, %%xmm1\n"

       "\tmovaps 4(%3),  %%xmm2\n"
       "\tmovaps 4(%4),  %%xmm3\n"
       "\tmulps  %%xmm0, %%xmm2\n"
       "\tmulps  %%xmm1, %%xmm3\n"
       "\tmovaps 20(%3), %%xmm4\n"
       "\tmulps  %%xmm0, %%xmm4\n"
       "\taddps  4(%0),  %%xmm2\n"
       "\tmovaps 20(%4), %%xmm5\n"
       "\tmulps  %%xmm1, %%xmm5\n"
       "\taddps  20(%0), %%xmm4\n"
       "\tsubps  %%xmm3, %%xmm2\n"
       "\tmovups %%xmm2, (%0)\n"
       "\tsubps  %%xmm5, %%xmm4\n"
       "\tmovups %%xmm4, 16(%0)\n"

       "\tmovss  36(%3), %%xmm2\n"
       "\tmulss  %%xmm0, %%xmm2\n"
       "\tmovss  36(%4), %%xmm3\n"
       "\tmulss  %%xmm1, %%xmm3\n"
       "\taddss  36(%0), %%xmm2\n"
       "\tmovss  40(%3), %%xmm4\n"
       "\tmulss  %%xmm0, %%xmm4\n"
       "\tmovss  40(%4), %%xmm5\n"
       "\tmulss  %%xmm1, %%xmm5\n"
       "\tsubss  %%xmm3, %%xmm2\n"
       "\tmovss  %%xmm2, 32(%0)       \n"
       "\tsubss  %%xmm5, %%xmm4\n"
       "\tmovss  %%xmm4, 36(%0)\n"

       : : "r" (mem), "r" (x+i), "r" (y+i), "r" (num), "r" (den)
       : "memory" );

   }
   for (i=0;i<ord;i++)
      _mem[i]=mem[i];

}


void iir_mem2(float *x, float *_den, float *y, int N, int ord, float *_mem)
{
   float  __den[20], __mem[20];
   float *den, *mem;
   int i;

   den = (float*)(((int)(__den+4))&0xfffffff0)-1;
   mem = (float*)(((int)(__mem+4))&0xfffffff0)-1;
   for (i=0;i<=10;i++)
      den[i]=0;
   for (i=0;i<10;i++)
      mem[i]=0;
   for (i=0;i<ord+1;i++)
   {
      den[i]=_den[i];
   }
   for (i=0;i<ord;i++)
      mem[i]=_mem[i];

   for (i=0;i<N;i++)
   {
#if 0
      y[i] = x[i] + mem[0];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] - den[j+1]*y[i];
      }
      mem[ord-1] = - den[ord]*y[i];
#else
      __asm__ __volatile__ 
      (
       "\tmovss (%1), %%xmm0\n"
       "\tmovss (%0), %%xmm1\n"
       "\taddss %%xmm0, %%xmm1\n"
       "\tmovss %%xmm1, (%2)\n"
       "\tshufps $0x00, %%xmm0, %%xmm0\n"
       "\tshufps $0x00, %%xmm1, %%xmm1\n"

       
       "\tmovaps 4(%3),  %%xmm2\n"
       "\tmovaps 20(%3), %%xmm3\n"
       "\tmulps  %%xmm1, %%xmm2\n"
       "\tmulps  %%xmm1, %%xmm3\n"
       "\tmovss  36(%3), %%xmm4\n"
       "\tmovss  40(%3), %%xmm5\n"
       "\tmulss  %%xmm1, %%xmm4\n"
       "\tmulss  %%xmm1, %%xmm5\n"
       "\tmovaps 4(%0),  %%xmm6\n"
       "\tsubps  %%xmm2, %%xmm6\n"
       "\tmovups %%xmm6, (%0)\n"
       "\tmovaps 20(%0), %%xmm7\n"
       "\tsubps  %%xmm3, %%xmm7\n"
       "\tmovups %%xmm7, 16(%0)\n"


       "\tmovss  36(%0), %%xmm7\n"
       "\tsubss  %%xmm4, %%xmm7\n"
       "\tmovss  %%xmm7, 32(%0)       \n"
       "\txorps  %%xmm2, %%xmm2\n"
       "\tsubss  %%xmm5, %%xmm2\n"
       "\tmovss  %%xmm2, 36(%0)\n"

       : : "r" (mem), "r" (x+i), "r" (y+i), "r" (den)
       : "memory" );
#endif
   }
   for (i=0;i<ord;i++)
      _mem[i]=mem[i];

}

