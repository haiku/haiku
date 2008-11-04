
/* 
 *  M_APM  -  mapmfmul.c
 *
 *  Copyright (C) 1999 - 2007   Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and
 *  that both that copyright notice and this permission notice appear
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted. Permission to distribute
 *  the modified code is granted. Modifications are to be distributed by
 *  using the file 'license.txt' as a template to modify the file header.
 *  'license.txt' is available in the official MAPM distribution.
 *
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapmfmul.c,v 1.33 2007/12/03 01:52:22 mike Exp $
 *
 *      This file contains the divide-and-conquer FAST MULTIPLICATION 
 *	function as well as its support functions.
 *
 *      $Log: mapmfmul.c,v $
 *      Revision 1.33  2007/12/03 01:52:22  mike
 *      Update license
 *
 *      Revision 1.32  2004/02/18 03:16:15  mike
 *      optimize 4 byte multiply (when FFT is disabled)
 *
 *      Revision 1.31  2003/12/04 01:14:06  mike
 *      redo math on 'borrow'
 *
 *      Revision 1.30  2003/07/21 20:34:18  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.29  2003/03/31 21:55:07  mike
 *      call generic error handling function
 *
 *      Revision 1.28  2002/11/03 22:38:15  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.27  2002/02/14 19:53:32  mike
 *      add conditional compiler option to disable use
 *      of FFT multiply if the user so chooses.
 *
 *      Revision 1.26  2001/07/26 20:56:38  mike
 *      fix comment, no code change
 *
 *      Revision 1.25  2001/07/16 19:43:45  mike
 *      add function M_free_all_fmul
 *
 *      Revision 1.24  2001/02/11 22:34:47  mike
 *      modify parameters to REALLOC
 *
 *      Revision 1.23  2000/10/20 19:23:26  mike
 *      adjust power_of_2 function so it should work with
 *      64 bit processors and beyond.
 *
 *      Revision 1.22  2000/08/23 22:27:34  mike
 *      no real code change, re-named 2 local functions
 *      so they make more sense.
 *
 *      Revision 1.21  2000/08/01 22:24:38  mike
 *      use sizeof(int) function call to stop some
 *      compilers from complaining.
 *
 *      Revision 1.20  2000/07/19 17:12:00  mike
 *      lower the number of bytes that the FFT can handle. worst case
 *      testing indicated math overflow when >= 1048576
 *
 *      Revision 1.19  2000/07/08 18:29:03  mike
 *      increase define so FFT can handle bigger numbers
 *
 *      Revision 1.18  2000/07/06 23:20:12  mike
 *      changed my mind. use static local MAPM numbers
 *      for temp data storage
 *
 *      Revision 1.17  2000/07/06 20:52:34  mike
 *      use init function to get local writable copies
 *      instead of using the stack
 *
 *      Revision 1.16  2000/07/04 17:25:09  mike
 *      guarantee 16 bit compilers still work OK
 *
 *      Revision 1.15  2000/07/04 15:40:02  mike
 *      add call to use FFT algorithm
 *
 *      Revision 1.14  2000/05/05 21:10:46  mike
 *      add comment indicating availability of assembly language
 *      version of M_4_byte_multiply for Linux on x86 platforms.
 *
 *      Revision 1.13  2000/04/20 19:30:45  mike
 *      minor optimization to 4 byte multiply
 *
 *      Revision 1.12  2000/04/14 15:39:30  mike
 *      optimize the fast multiply function. don't re-curse down
 *      to a size of 1. recurse down to a size of '4' and then
 *      call a special 4 byte multiply function.
 *
 *      Revision 1.11  2000/02/03 23:02:13  mike
 *      put in RCS for real...
 *
 *      Revision 1.10  2000/02/03 22:59:08  mike
 *      remove the extra recursive function. not needed any
 *      longer since all current compilers should not have
 *      any problem with true recursive calls.
 *
 *      Revision 1.9  2000/02/03 22:47:39  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.8  1999/09/19 21:13:44  mike
 *      eliminate unneeded local int in _split
 *
 *      Revision 1.7  1999/08/12 22:36:23  mike
 *      move the 3 'simple' function to the top of file
 *      so GCC can in-line the code.
 *
 *      Revision 1.6  1999/08/12 22:01:14  mike
 *      more minor optimizations
 *
 *      Revision 1.5  1999/08/12 02:02:06  mike
 *      minor optimization
 *
 *      Revision 1.4  1999/08/10 22:51:59  mike
 *      minor tweak
 *
 *      Revision 1.3  1999/08/10 00:45:47  mike
 *      added more comments and a few minor tweaks
 *
 *      Revision 1.2  1999/08/09 02:50:02  mike
 *      add some comments
 *
 *      Revision 1.1  1999/08/08 18:27:57  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static int M_firsttimef = TRUE;

/*
 *      specify the max size the FFT routine can handle 
 *      (in MAPM, #digits = 2 * #bytes)
 *
 *      this number *must* be an exact power of 2.
 *
 *      **WORST** case input numbers (all 9's) has shown that
 *	the FFT math will overflow if the #define here is 
 *      >= 1048576. On my system, 524,288 worked OK. I will
 *      factor down another factor of 2 to safeguard against 
 *	other computers have less precise floating point math. 
 *	If you are confident in your system, 524288 will 
 *	theoretically work fine.
 *
 *      the define here allows the FFT algorithm to multiply two
 *      524,288 digit numbers yielding a 1,048,576 digit result.
 */

#define MAX_FFT_BYTES 262144

/*
 *      the Divide-and-Conquer multiplication kicks in when the size of
 *	the numbers exceed the capability of the FFT (#define just above).
 *
 *	#bytes    D&C call depth
 *	------    --------------
 *       512K           1
 *        1M            2
 *        2M            3
 *        4M            4
 *       ...           ...
 *    2.1990E+12       23 
 *
 *	the following stack sizes are sized to meet the 
 *      above 2.199E+12 example, though I wouldn't want to
 *	wait for it to finish...
 *
 *      Each call requires 7 stack variables to be saved so 
 *	we need a stack depth of 23 * 7 + PAD.  (we use 164)
 *
 *      For 'exp_stack', 3 integers also are required to be saved 
 *      for each recursive call so we need a stack depth of 
 *      23 * 3 + PAD. (we use 72)
 *
 *
 *      If the FFT multiply is disabled, resize the arrays
 *	as follows:
 *
 *      the following stack sizes are sized to meet the 
 *      worst case expected assuming we are multiplying 
 *      numbers with 2.14E+9 (2 ^ 31) digits. 
 *
 *      For sizeof(int) == 4 (32 bits) there may be up to 32 recursive
 *      calls. Each call requires 7 stack variables so we need a
 *      stack depth of 32 * 7 + PAD.  (we use 240)
 *
 *      For 'exp_stack', 3 integers also are required to be saved 
 *      for each recursive call so we need a stack depth of 
 *      32 * 3 + PAD. (we use 100)
 */

#ifdef NO_FFT_MULTIPLY
#define M_STACK_SIZE 240
#define M_ISTACK_SIZE 100
#else
#define M_STACK_SIZE 164
#define M_ISTACK_SIZE 72
#endif

static int    exp_stack[M_ISTACK_SIZE];
static int    exp_stack_ptr;

static UCHAR  *mul_stack_data[M_STACK_SIZE];
static int    mul_stack_data_size[M_STACK_SIZE];
static int    M_mul_stack_ptr;

static UCHAR  *fmul_a1, *fmul_a0, *fmul_a9, *fmul_b1, *fmul_b0, 
	      *fmul_b9, *fmul_t0;

static int    size_flag, bit_limit, stmp, itmp, mii;

static M_APM  M_ain;
static M_APM  M_bin;

static char   *M_stack_ptr_error_msg = "\'M_get_stack_ptr\', Out of memory";

extern void   M_fast_multiply(M_APM, M_APM, M_APM);
extern void   M_fmul_div_conq(UCHAR *, UCHAR *, UCHAR *, int);
extern void   M_fmul_add(UCHAR *, UCHAR *, int, int);
extern int    M_fmul_subtract(UCHAR *, UCHAR *, UCHAR *, int);
extern void   M_fmul_split(UCHAR *, UCHAR *, UCHAR *, int);
extern int    M_next_power_of_2(int);
extern int    M_get_stack_ptr(int);
extern void   M_push_mul_int(int);
extern int    M_pop_mul_int(void);

#ifdef NO_FFT_MULTIPLY
extern void   M_4_byte_multiply(UCHAR *, UCHAR *, UCHAR *);
#else
extern void   M_fast_mul_fft(UCHAR *, UCHAR *, UCHAR *, int);
#endif

/*
 *      the following algorithm is used in this fast multiply routine
 *	(sometimes called the divide-and-conquer technique.)
 *
 *	assume we have 2 numbers (a & b) with 2N digits. 
 *
 *      let : a = (2^N) * A1 + A0 , b = (2^N) * B1 + B0      
 *
 *	where 'A1' is the 'most significant half' of 'a' and 
 *      'A0' is the 'least significant half' of 'a'. Same for 
 *	B1 and B0.
 *
 *	Now use the identity :
 *
 *               2N   N            N                    N
 *	ab  =  (2  + 2 ) A1B1  +  2 (A1-A0)(B0-B1)  + (2 + 1)A0B0
 *
 *
 *      The original problem of multiplying 2 (2N) digit numbers has
 *	been reduced to 3 multiplications of N digit numbers plus some
 *	additions, subtractions, and shifts.
 *
 *	The fast multiplication algorithm used here uses the above 
 *	identity in a recursive process. This algorithm results in
 *	O(n ^ 1.585) growth.
 */


/****************************************************************************/
void	M_free_all_fmul()
{
int	k;

if (M_firsttimef == FALSE)
  {
   m_apm_free(M_ain);
   m_apm_free(M_bin);

   for (k=0; k < M_STACK_SIZE; k++)
     {
      if (mul_stack_data_size[k] != 0)
        {
         MAPM_FREE(mul_stack_data[k]);
	}
     }

   M_firsttimef = TRUE;
  }
}
/****************************************************************************/
void	M_push_mul_int(int val)
{
exp_stack[++exp_stack_ptr] = val;
}
/****************************************************************************/
int	M_pop_mul_int()
{
return(exp_stack[exp_stack_ptr--]);
}
/****************************************************************************/
void   	M_fmul_split(UCHAR *x1, UCHAR *x0, UCHAR *xin, int nbytes)
{
memcpy(x1, xin, nbytes);
memcpy(x0, (xin + nbytes), nbytes);
}
/****************************************************************************/
void	M_fast_multiply(M_APM rr, M_APM aa, M_APM bb)
{
void	*vp;
int	ii, k, nexp, sign;

if (M_firsttimef)
  {
   M_firsttimef = FALSE;

   for (k=0; k < M_STACK_SIZE; k++)
     mul_stack_data_size[k] = 0;

   size_flag = M_get_sizeof_int();
   bit_limit = 8 * size_flag + 1;

   M_ain = m_apm_init();
   M_bin = m_apm_init();
  }

exp_stack_ptr   = -1;
M_mul_stack_ptr = -1;

m_apm_copy(M_ain, aa);
m_apm_copy(M_bin, bb);

sign = M_ain->m_apm_sign * M_bin->m_apm_sign;
nexp = M_ain->m_apm_exponent + M_bin->m_apm_exponent;

if (M_ain->m_apm_datalength >= M_bin->m_apm_datalength)
  ii = M_ain->m_apm_datalength;
else
  ii = M_bin->m_apm_datalength;

ii = (ii + 1) >> 1;
ii = M_next_power_of_2(ii);

/* Note: 'ii' must be >= 4 here. this is guaranteed 
   by the caller: m_apm_multiply
*/

k = 2 * ii;                   /* required size of result, in bytes  */

M_apm_pad(M_ain, k);          /* fill out the data so the number of */
M_apm_pad(M_bin, k);          /* bytes is an exact power of 2       */

if (k > rr->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(rr->m_apm_data, (k + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'M_fast_multiply\', Out of memory");
     }
  
   rr->m_apm_malloclength = k + 28;
   rr->m_apm_data = (UCHAR *)vp;
  }

#ifdef NO_FFT_MULTIPLY

M_fmul_div_conq(rr->m_apm_data, M_ain->m_apm_data, 
                                M_bin->m_apm_data, ii);
#else

/*
 *     if the numbers are *really* big, use the divide-and-conquer
 *     routine first until the numbers are small enough to be handled 
 *     by the FFT algorithm. If the numbers are already small enough,
 *     call the FFT multiplication now.
 *
 *     Note that 'ii' here is (and must be) an exact power of 2.
 */

if (size_flag == 2)   /* if still using 16 bit compilers .... */
  {
   M_fast_mul_fft(rr->m_apm_data, M_ain->m_apm_data, 
                                  M_bin->m_apm_data, ii);
  }
else                  /* >= 32 bit compilers */
  {
   if (ii > (MAX_FFT_BYTES + 2))
     {
      M_fmul_div_conq(rr->m_apm_data, M_ain->m_apm_data, 
                                      M_bin->m_apm_data, ii);
     }
   else
     {
      M_fast_mul_fft(rr->m_apm_data, M_ain->m_apm_data, 
                                     M_bin->m_apm_data, ii);
     }
  }

#endif

rr->m_apm_sign       = sign;
rr->m_apm_exponent   = nexp;
rr->m_apm_datalength = 4 * ii;

M_apm_normalize(rr);
}
/****************************************************************************/
/*
 *      This is the recursive function to perform the multiply. The 
 *      design intent here is to have no local variables. Any local
 *      data that needs to be saved is saved on one of the two stacks.
 */
void	M_fmul_div_conq(UCHAR *rr, UCHAR *aa, UCHAR *bb, int sz)
{

#ifdef NO_FFT_MULTIPLY

if (sz == 4)                /* multiply 4x4 yielding an 8 byte result */
  {
   M_4_byte_multiply(rr, aa, bb);
   return;
  }

#else

/*
 *  if the numbers are now small enough, let the FFT algorithm
 *  finish up.
 */

if (sz == MAX_FFT_BYTES)     
  {
   M_fast_mul_fft(rr, aa, bb, sz);
   return;
  }

#endif

memset(rr, 0, (2 * sz));    /* zero out the result */
mii = sz >> 1;

itmp = M_get_stack_ptr(mii);
M_push_mul_int(itmp);

fmul_a1 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(mii);
fmul_a0 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(2 * sz);
fmul_a9 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(mii);
fmul_b1 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(mii);
fmul_b0 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(2 * sz);
fmul_b9 = mul_stack_data[itmp];

itmp    = M_get_stack_ptr(2 * sz);
fmul_t0 = mul_stack_data[itmp];

M_fmul_split(fmul_a1, fmul_a0, aa, mii);
M_fmul_split(fmul_b1, fmul_b0, bb, mii);

stmp  = M_fmul_subtract(fmul_a9, fmul_a1, fmul_a0, mii);
stmp *= M_fmul_subtract(fmul_b9, fmul_b0, fmul_b1, mii);

M_push_mul_int(stmp);
M_push_mul_int(mii);

M_fmul_div_conq(fmul_t0, fmul_a0, fmul_b0, mii);

mii  = M_pop_mul_int();
stmp = M_pop_mul_int();
itmp = M_pop_mul_int();

M_push_mul_int(itmp);
M_push_mul_int(stmp);
M_push_mul_int(mii);

/*   to restore all stack variables ...
fmul_a1 = mul_stack_data[itmp];
fmul_a0 = mul_stack_data[itmp+1];
fmul_a9 = mul_stack_data[itmp+2];
fmul_b1 = mul_stack_data[itmp+3];
fmul_b0 = mul_stack_data[itmp+4];
fmul_b9 = mul_stack_data[itmp+5];
fmul_t0 = mul_stack_data[itmp+6];
*/

fmul_a1 = mul_stack_data[itmp];
fmul_b1 = mul_stack_data[itmp+3];
fmul_t0 = mul_stack_data[itmp+6];

memcpy((rr + sz), fmul_t0, sz);    /* first 'add', result is now zero */
				   /* so we just copy in the bytes    */
M_fmul_add(rr, fmul_t0, mii, sz);

M_fmul_div_conq(fmul_t0, fmul_a1, fmul_b1, mii);

mii  = M_pop_mul_int();
stmp = M_pop_mul_int();
itmp = M_pop_mul_int();

M_push_mul_int(itmp);
M_push_mul_int(stmp);
M_push_mul_int(mii);

fmul_a9 = mul_stack_data[itmp+2];
fmul_b9 = mul_stack_data[itmp+5];
fmul_t0 = mul_stack_data[itmp+6];

M_fmul_add(rr, fmul_t0, 0, sz);
M_fmul_add(rr, fmul_t0, mii, sz);

if (stmp != 0)
  M_fmul_div_conq(fmul_t0, fmul_a9, fmul_b9, mii);

mii  = M_pop_mul_int();
stmp = M_pop_mul_int();
itmp = M_pop_mul_int();

fmul_t0 = mul_stack_data[itmp+6];

/*
 *  if the sign of (A1 - A0)(B0 - B1) is positive, ADD to
 *  the result. if it is negative, SUBTRACT from the result.
 */

if (stmp < 0)
  {
   fmul_a9 = mul_stack_data[itmp+2];
   fmul_b9 = mul_stack_data[itmp+5];

   memset(fmul_b9, 0, (2 * sz)); 
   memcpy((fmul_b9 + mii), fmul_t0, sz); 
   M_fmul_subtract(fmul_a9, rr, fmul_b9, (2 * sz));
   memcpy(rr, fmul_a9, (2 * sz));
  }

if (stmp > 0)
  M_fmul_add(rr, fmul_t0, mii, sz);

M_mul_stack_ptr -= 7;
}
/****************************************************************************/
/*
 *	special addition function for use with the fast multiply operation
 */
void    M_fmul_add(UCHAR *r, UCHAR *a, int offset, int sz)
{
int	i, j;
UCHAR   carry;

carry = 0;
j = offset + sz;
i = sz;

while (TRUE)
  {
   r[--j] += carry + a[--i];

   if (r[j] >= 100)
     {
      r[j] -= 100;
      carry = 1;
     }
   else
      carry = 0;

   if (i == 0)
     break;
  }

if (carry)
  {
   while (TRUE)
     {
      r[--j] += 1;
   
      if (r[j] < 100)
        break;

      r[j] -= 100;
     }
  }
}
/****************************************************************************/
/*
 *	special subtraction function for use with the fast multiply operation
 */
int     M_fmul_subtract(UCHAR *r, UCHAR *a, UCHAR *b, int sz)
{
int	k, jtmp, sflag, nb, borrow;

nb    = sz;
sflag = 0;      /* sign flag: assume the numbers are equal */

/*
 *   find if a > b (so we perform a-b)
 *   or      a < b (so we perform b-a)
 */

for (k=0; k < nb; k++)
  {
   if (a[k] < b[k])
     {
      sflag = -1;
      break;
     }

   if (a[k] > b[k])
     {
      sflag = 1;
      break;
     }
  }

if (sflag == 0)
  {
   memset(r, 0, nb);            /* zero out the result */
  }
else
  {
   k = nb;
   borrow = 0;

   while (TRUE)
     {
      k--;

      if (sflag == 1)
        jtmp = (int)a[k] - ((int)b[k] + borrow);
      else
        jtmp = (int)b[k] - ((int)a[k] + borrow);

      if (jtmp >= 0)
        {
         r[k] = (UCHAR)jtmp;
         borrow = 0;
        }
      else
        {
         r[k] = (UCHAR)(100 + jtmp);
         borrow = 1;
        }

      if (k == 0)
        break;
     }
  }

return(sflag);
}
/****************************************************************************/
int     M_next_power_of_2(int n)
{
int     ct, k;

if (n <= 2)
  return(n);

k  = 2;
ct = 0;

while (TRUE)
  {
   if (k >= n)
     break;

   k = k << 1;

   if (++ct == bit_limit)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, 
               "\'M_next_power_of_2\', ERROR :sizeof(int) too small ??");
     }
  }

return(k);
}
/****************************************************************************/
int	M_get_stack_ptr(int sz)
{
int	i, k;
UCHAR   *cp;

k = ++M_mul_stack_ptr;

/* if size is 0, just need to malloc and return */
if (mul_stack_data_size[k] == 0)
  {
   if ((i = sz) < 16)
     i = 16;

   if ((cp = (UCHAR *)MAPM_MALLOC(i + 4)) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, M_stack_ptr_error_msg);
     }

   mul_stack_data[k]      = cp;
   mul_stack_data_size[k] = i;
  }
else        /* it has been malloc'ed, see if it's big enough */
  {
   if (sz > mul_stack_data_size[k])
     {
      cp = mul_stack_data[k];

      if ((cp = (UCHAR *)MAPM_REALLOC(cp, (sz + 4))) == NULL)
        {
         /* fatal, this does not return */
   
         M_apm_log_error_msg(M_APM_FATAL, M_stack_ptr_error_msg);
        }
   
      mul_stack_data[k]      = cp;
      mul_stack_data_size[k] = sz;
     }
  }

return(k);
}
/****************************************************************************/

#ifdef NO_FFT_MULTIPLY

/*
 *      multiply a 4 byte number by a 4 byte number
 *      yielding an 8 byte result. each byte contains
 *      a base 100 'digit', i.e.: range from 0-99.
 *
 *             MSB         LSB
 *
 *      a,b    [0] [1] [2] [3]
 *   result    [0]  .....  [7]
 */

void	M_4_byte_multiply(UCHAR *r, UCHAR *a, UCHAR *b)
{
int	      jj;
unsigned int  *ip, t1, rr[8];

memset(rr, 0, (8 * sizeof(int)));        /* zero out result */
jj = 3;
ip = rr + 5;

/*
 *   loop for one number [b], un-roll the inner 'loop' [a]
 *
 *   accumulate partial sums in UINT array, release carries 
 *   and convert back to base 100 at the end
 */

while (1)
  {
   t1  = (unsigned int)b[jj];
   ip += 2;

   *ip-- += t1 * a[3];
   *ip-- += t1 * a[2];
   *ip-- += t1 * a[1];
   *ip   += t1 * a[0];
   
   if (jj-- == 0)
     break;
  }

jj = 7;

while (1)
  {
   t1 = rr[jj] / 100;
   r[jj] = (UCHAR)(rr[jj] - 100 * t1);

   if (jj == 0)
     break;

   rr[--jj] += t1;
  }
}

#endif

/****************************************************************************/
