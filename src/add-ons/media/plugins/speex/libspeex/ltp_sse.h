/* Copyright (C) 2002 Jean-Marc Valin 
   File: ltp.c
   Lont-Term Prediction functions

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


static float inner_prod(float *a, float *b, int len)
{
  float sum;
  __asm__ __volatile__ (
  "\tpush %%eax\n"
  "\tpush %%edi\n"
  "\tpush %%ecx\n"
  "\txorps %%xmm3, %%xmm3\n"
  "\txorps %%xmm4, %%xmm4\n"

  "\tsub $20, %%ecx\n"

".mul20_loop%=:\n"

  "\tmovups (%%eax), %%xmm0\n"
  "\tmovups (%%edi), %%xmm1\n"
  "\tmulps %%xmm0, %%xmm1\n"

  "\tmovups 16(%%eax), %%xmm5\n"
  "\tmovups 16(%%edi), %%xmm6\n"
  "\tmulps %%xmm5, %%xmm6\n"
  "\taddps %%xmm1, %%xmm3\n"

  "\tmovups 32(%%eax), %%xmm0\n"
  "\tmovups 32(%%edi), %%xmm1\n"
  "\tmulps %%xmm0, %%xmm1\n"
  "\taddps %%xmm6, %%xmm4\n"

  "\tmovups 48(%%eax), %%xmm5\n"
  "\tmovups 48(%%edi), %%xmm6\n"
  "\tmulps %%xmm5, %%xmm6\n"
  "\taddps %%xmm1, %%xmm3\n"

  "\tmovups 64(%%eax), %%xmm0\n"
  "\tmovups 64(%%edi), %%xmm1\n"
  "\tmulps %%xmm0, %%xmm1\n"
  "\taddps %%xmm6, %%xmm4\n"
  "\taddps %%xmm1, %%xmm3\n"


  "\tadd $80, %%eax\n"
  "\tadd $80, %%edi\n"

  "\tsub $20,  %%ecx\n"

  "\tjae .mul20_loop%=\n"

  "\taddps %%xmm4, %%xmm3\n"

  "\tmovhlps %%xmm3, %%xmm4\n"
  "\taddps %%xmm4, %%xmm3\n"
  "\tmovaps %%xmm3, %%xmm4\n"
  "\tshufps $0x55, %%xmm4, %%xmm4\n"
  "\taddss %%xmm4, %%xmm3\n"
  "\tmovss %%xmm3, (%%edx)\n"
  
  "\tpop %%ecx\n"
  "\tpop %%edi\n"
  "\tpop %%eax\n"
  : : "a" (a), "D" (b), "c" (len), "d" (&sum) : "memory");
  return sum;
}
