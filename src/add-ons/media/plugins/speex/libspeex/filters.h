/* Copyright (C) 2002 Jean-Marc Valin 
   File: filters.h
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

#ifndef FILTERS_H
#define FILTERS_H


typedef struct CombFilterMem {
   int   last_pitch;
   float last_pitch_gain[3];
   float smooth_gain;
} CombFilterMem;


void qmf_decomp(float *xx, float *aa, float *y1, float *y2, int N, int M, float *mem, char *stack);
void fir_mem_up(float *x, float *a, float *y, int N, int M, float *mem, char *stack);


void filter_mem2(float *x, float *num, float *den, float *y, int N, int ord, float *mem);
void fir_mem2(float *x, float *num, float *y, int N, int ord, float *mem);
void iir_mem2(float *x, float *den, float *y, int N, int ord, float *mem);

/* Apply bandwidth expansion on LPC coef */
void bw_lpc(float gamma, float *lpc_in, float *lpc_out, int order);



/* FIR filter */
void fir_decim_mem(float *x, float *a, float *y, int N, int M, float *mem);

void syn_percep_zero(float *x, float *ak, float *awk1, float *awk2, float *y, int N, int ord, char *stack);

void residue_percep_zero(float *xx, float *ak, float *awk1, float *awk2, float *y, int N, int ord, char *stack);

void comp_filter_mem_init (CombFilterMem *mem);

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
);


#endif
