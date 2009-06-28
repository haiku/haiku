
/* 
 *  M_APM  -  mapm_fft.c
 *
 *  This FFT (Fast Fourier Transform) is from Takuya OOURA 
 *
 *  Copyright(C) 1996-1999 Takuya OOURA
 *  email: ooura@mmm.t.u-tokyo.ac.jp
 *
 *  See full FFT documentation below ...  (MCR)
 *
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapm_fft.c,v 1.15 2007/12/03 01:37:42 mike Exp $
 *
 *      This file contains the FFT based FAST MULTIPLICATION function 
 *      as well as its support functions.
 *
 *      $Log: mapm_fft.c,v $
 *      Revision 1.15  2007/12/03 01:37:42  mike
 *      no changes
 *
 *      Revision 1.14  2003/07/28 19:39:01  mike
 *      change 16 bit constant
 *
 *      Revision 1.13  2003/07/21 20:11:55  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.12  2003/05/01 21:55:36  mike
 *      remove math.h
 *
 *      Revision 1.11  2003/03/31 22:10:09  mike
 *      call generic error handling function
 *
 *      Revision 1.10  2002/11/03 22:11:48  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.9  2001/07/16 19:16:15  mike
 *      add function M_free_all_fft
 *
 *      Revision 1.8  2000/08/01 22:23:24  mike
 *      use sizeof(int) from function call to stop
 *      some compilers from complaining.
 *
 *      Revision 1.7  2000/07/30 22:39:21  mike
 *      lower 16 bit malloc size
 *
 *      Revision 1.6  2000/07/10 22:54:26  mike
 *      malloc the local data arrays
 *
 *      Revision 1.5  2000/07/10 00:09:02  mike
 *      use local static arrays for smaller numbers
 *
 *      Revision 1.4  2000/07/08 18:24:23  mike
 *      minor optimization tweak
 *
 *      Revision 1.3  2000/07/08 17:52:49  mike
 *      do the FFT in base 10000 instead of MAPM numbers base 100
 *      this runs faster and uses 1/2 the RAM
 *
 *      Revision 1.2  2000/07/06 21:04:34  mike
 *      added more comments
 *
 *      Revision 1.1  2000/07/06 20:42:05  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

#ifndef MM_PI_2
#define MM_PI_2      1.570796326794896619231321691639751442098584699687
#endif

#ifndef WR5000       /* cos(MM_PI_2*0.5000) */
#define WR5000       0.707106781186547524400844362104849039284835937688
#endif

#ifndef RDFT_LOOP_DIV     /* control of the RDFT's speed & tolerance */
#define RDFT_LOOP_DIV 64
#endif

extern void   M_fast_mul_fft(UCHAR *, UCHAR *, UCHAR *, int);

extern void   M_rdft(int, int, double *);
extern void   M_bitrv2(int, double *);
extern void   M_cftfsub(int, double *);
extern void   M_cftbsub(int, double *);
extern void   M_rftfsub(int, double *);
extern void   M_rftbsub(int, double *);
extern void   M_cft1st(int, double *);
extern void   M_cftmdl(int, int, double *);

static double *M_aa_array, *M_bb_array;
static int    M_size = -1;

static char   *M_fft_error_msg = "\'M_fast_mul_fft\', Out of memory";

/****************************************************************************/
void	M_free_all_fft()
{
if (M_size > 0)
  {
   MAPM_FREE(M_aa_array);
   MAPM_FREE(M_bb_array);
   M_size = -1;
  }
}
/****************************************************************************/
/*
 *      multiply 'uu' by 'vv' with nbytes each
 *      yielding a 2*nbytes result in 'ww'. 
 *      each byte contains a base 100 'digit', 
 *      i.e.: range from 0-99.
 *
 *             MSB              LSB
 *
 *   uu,vv     [0] [1] [2] ... [N-1]
 *   ww        [0] [1] [2] ... [2N-1]
 */

void	M_fast_mul_fft(UCHAR *ww, UCHAR *uu, UCHAR *vv, int nbytes)
{
int             mflag, i, j, nn2, nn;
double          carry, nnr, dtemp, *a, *b;
UCHAR           *w0;
unsigned long   ul;

if (M_size < 0)                  /* if first time in, setup working arrays */
  {
   if (M_get_sizeof_int() == 2)  /* if still using 16 bit compilers */
     M_size = 516;
   else
     M_size = 8200;

   M_aa_array = (double *)MAPM_MALLOC(M_size * sizeof(double));
   M_bb_array = (double *)MAPM_MALLOC(M_size * sizeof(double));
   
   if ((M_aa_array == NULL) || (M_bb_array == NULL))
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, M_fft_error_msg);
     }
  }

nn  = nbytes;
nn2 = nbytes >> 1;

if (nn > M_size)
  {
   mflag = TRUE;

   a = (double *)MAPM_MALLOC((nn + 8) * sizeof(double));
   b = (double *)MAPM_MALLOC((nn + 8) * sizeof(double));
   
   if ((a == NULL) || (b == NULL))
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, M_fft_error_msg);
     }
  }
else
  {
   mflag = FALSE;

   a = M_aa_array;
   b = M_bb_array;
  }

/*
 *   convert normal base 100 MAPM numbers to base 10000
 *   for the FFT operation.
 */

i = 0;
for (j=0; j < nn2; j++)
  {
   a[j] = (double)((int)uu[i] * 100 + uu[i+1]);
   b[j] = (double)((int)vv[i] * 100 + vv[i+1]);
   i += 2;
  }

/* zero fill the second half of the arrays */

for (j=nn2; j < nn; j++)
  {
   a[j] = 0.0;
   b[j] = 0.0;
  }

/* perform the forward Fourier transforms for both numbers */

M_rdft(nn, 1, a);
M_rdft(nn, 1, b);

/* perform the convolution ... */

b[0] *= a[0];
b[1] *= a[1];

for (j=3; j <= nn; j += 2)
  {
   dtemp  = b[j-1];
   b[j-1] = dtemp * a[j-1] - b[j] * a[j];
   b[j]   = dtemp * a[j] + b[j] * a[j-1];
  }

/* perform the inverse transform on the result */

M_rdft(nn, -1, b);

/* perform a final pass to release all the carries */
/* we are still in base 10000 at this point        */

carry = 0.0;
j     = nn;
nnr   = 2.0 / (double)nn;

while (1)
  {
   dtemp = b[--j] * nnr + carry + 0.5;
   ul    = (unsigned long)(dtemp * 1.0E-4);
   carry = (double)ul;
   b[j]  = dtemp - carry * 10000.0;

   if (j == 0)
     break;
  }

/* copy result to our destination after converting back to base 100 */

w0 = ww;
M_get_div_rem((int)ul, w0, (w0 + 1));

for (j=0; j <= (nn - 2); j++)
  {
   w0 += 2;
   M_get_div_rem((int)b[j], w0, (w0 + 1));
  }

if (mflag)
  {
   MAPM_FREE(b);
   MAPM_FREE(a);
  }
}
/****************************************************************************/

/*
 *    The following info is from Takuya OOURA's documentation : 
 *
 *    NOTE : MAPM only uses the 'RDFT' function (as well as the 
 *           functions RDFT calls). All the code from here down 
 *           in this file is from Takuya OOURA. The only change I
 *           made was to add 'M_' in front of all the functions
 *           I used. This was to guard against any possible 
 *           name collisions in the future.
 *
 *    MCR  06 July 2000
 *
 *
 *    General Purpose FFT (Fast Fourier/Cosine/Sine Transform) Package
 *    
 *    Description:
 *        A package to calculate Discrete Fourier/Cosine/Sine Transforms of 
 *        1-dimensional sequences of length 2^N.
 *    
 *        fft4g_h.c  : FFT Package in C       - Simple Version I   (radix 4,2)
 *        
 *        rdft: Real Discrete Fourier Transform
 *    
 *    Method:
 *        -------- rdft --------
 *        A method with a following butterfly operation appended to "cdft".
 *        In forward transform :
 *            A[k] = sum_j=0^n-1 a[j]*W(n)^(j*k), 0<=k<=n/2, 
 *                W(n) = exp(2*pi*i/n), 
 *        this routine makes an array x[] :
 *            x[j] = a[2*j] + i*a[2*j+1], 0<=j<n/2
 *        and calls "cdft" of length n/2 :
 *            X[k] = sum_j=0^n/2-1 x[j] * W(n/2)^(j*k), 0<=k<n.
 *        The result A[k] are :
 *            A[k]     = X[k]     - (1+i*W(n)^k)/2 * (X[k]-conjg(X[n/2-k])),
 *            A[n/2-k] = X[n/2-k] + 
 *                            conjg((1+i*W(n)^k)/2 * (X[k]-conjg(X[n/2-k]))),
 *                0<=k<=n/2
 *            (notes: conjg() is a complex conjugate, X[n/2]=X[0]).
 *        ----------------------
 *    
 *    Reference:
 *        * Masatake MORI, Makoto NATORI, Tatuo TORII: Suchikeisan, 
 *          Iwanamikouzajyouhoukagaku18, Iwanami, 1982 (Japanese)
 *        * Henri J. Nussbaumer: Fast Fourier Transform and Convolution 
 *          Algorithms, Springer Verlag, 1982
 *        * C. S. Burrus, Notes on the FFT (with large FFT paper list)
 *          http://www-dsp.rice.edu/research/fft/fftnote.asc
 *    
 *    Copyright:
 *        Copyright(C) 1996-1999 Takuya OOURA
 *        email: ooura@mmm.t.u-tokyo.ac.jp
 *        download: http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html
 *        You may use, copy, modify this code for any purpose and 
 *        without fee. You may distribute this ORIGINAL package.
 */

/*

functions
    rdft: Real Discrete Fourier Transform

function prototypes
    void rdft(int, int, double *);

-------- Real DFT / Inverse of Real DFT --------
    [definition]
        <case1> RDFT
            R[k] = sum_j=0^n-1 a[j]*cos(2*pi*j*k/n), 0<=k<=n/2
            I[k] = sum_j=0^n-1 a[j]*sin(2*pi*j*k/n), 0<k<n/2
        <case2> IRDFT (excluding scale)
            a[k] = (R[0] + R[n/2]*cos(pi*k))/2 + 
                   sum_j=1^n/2-1 R[j]*cos(2*pi*j*k/n) + 
                   sum_j=1^n/2-1 I[j]*sin(2*pi*j*k/n), 0<=k<n
    [usage]
        <case1>
            rdft(n, 1, a);
        <case2>
            rdft(n, -1, a);
    [parameters]
        n              :data length (int)
                        n >= 2, n = power of 2
        a[0...n-1]     :input/output data (double *)
                        <case1>
                            output data
                                a[2*k] = R[k], 0<=k<n/2
                                a[2*k+1] = I[k], 0<k<n/2
                                a[1] = R[n/2]
                        <case2>
                            input data
                                a[2*j] = R[j], 0<=j<n/2
                                a[2*j+1] = I[j], 0<j<n/2
                                a[1] = R[n/2]
    [remark]
        Inverse of 
            rdft(n, 1, a);
        is 
            rdft(n, -1, a);
            for (j = 0; j <= n - 1; j++) {
                a[j] *= 2.0 / n;
            }
*/


void	M_rdft(int n, int isgn, double *a)
{
    double xi;

    if (isgn >= 0) {
        if (n > 4) {
            M_bitrv2(n, a);
            M_cftfsub(n, a);
            M_rftfsub(n, a);
        } else if (n == 4) {
            M_cftfsub(n, a);
        }
        xi = a[0] - a[1];
        a[0] += a[1];
        a[1] = xi;
    } else {
        a[1] = 0.5 * (a[0] - a[1]);
        a[0] -= a[1];
        if (n > 4) {
            M_rftbsub(n, a);
            M_bitrv2(n, a);
            M_cftbsub(n, a);
        } else if (n == 4) {
            M_cftfsub(n, a);
        }
    }
}



void    M_bitrv2(int n, double *a)
{
    int j0, k0, j1, k1, l, m, i, j, k;
    double xr, xi, yr, yi;
    
    l = n >> 2;
    m = 2;
    while (m < l) {
        l >>= 1;
        m <<= 1;
    }
    if (m == l) {
        j0 = 0;
        for (k0 = 0; k0 < m; k0 += 2) {
            k = k0;
            for (j = j0; j < j0 + k0; j += 2) {
                xr = a[j];
                xi = a[j + 1];
                yr = a[k];
                yi = a[k + 1];
                a[j] = yr;
                a[j + 1] = yi;
                a[k] = xr;
                a[k + 1] = xi;
                j1 = j + m;
                k1 = k + 2 * m;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m;
                k1 -= m;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += m;
                k1 += 2 * m;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                for (i = n >> 1; i > (k ^= i); i >>= 1);
            }
            j1 = j0 + k0 + m;
            k1 = j1 + m;
            xr = a[j1];
            xi = a[j1 + 1];
            yr = a[k1];
            yi = a[k1 + 1];
            a[j1] = yr;
            a[j1 + 1] = yi;
            a[k1] = xr;
            a[k1 + 1] = xi;
            for (i = n >> 1; i > (j0 ^= i); i >>= 1);
        }
    } else {
        j0 = 0;
        for (k0 = 2; k0 < m; k0 += 2) {
            for (i = n >> 1; i > (j0 ^= i); i >>= 1);
            k = k0;
            for (j = j0; j < j0 + k0; j += 2) {
                xr = a[j];
                xi = a[j + 1];
                yr = a[k];
                yi = a[k + 1];
                a[j] = yr;
                a[j + 1] = yi;
                a[k] = xr;
                a[k + 1] = xi;
                j1 = j + m;
                k1 = k + m;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                for (i = n >> 1; i > (k ^= i); i >>= 1);
            }
        }
    }
}



void    M_cftfsub(int n, double *a)
{
    int j, j1, j2, j3, l;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        M_cft1st(n, a);
        l = 8;
        while ((l << 2) < n) {
            M_cftmdl(n, l, a);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i - x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i + x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i - x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = a[j + 1] - a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] += a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}



void 	M_cftbsub(int n, double *a)
{
    int j, j1, j2, j3, l;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    l = 2;
    if (n > 8) {
        M_cft1st(n, a);
        l = 8;
        while ((l << 2) < n) {
            M_cftmdl(n, l, a);
            l <<= 2;
        }
    }
    if ((l << 2) == n) {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = -a[j + 1] - a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = -a[j + 1] + a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i - x2i;
            a[j2] = x0r - x2r;
            a[j2 + 1] = x0i + x2i;
            a[j1] = x1r - x3i;
            a[j1 + 1] = x1i - x3r;
            a[j3] = x1r + x3i;
            a[j3 + 1] = x1i + x3r;
        }
    } else {
        for (j = 0; j < l; j += 2) {
            j1 = j + l;
            x0r = a[j] - a[j1];
            x0i = -a[j + 1] + a[j1 + 1];
            a[j] += a[j1];
            a[j + 1] = -a[j + 1] - a[j1 + 1];
            a[j1] = x0r;
            a[j1 + 1] = x0i;
        }
    }
}



void 	M_cft1st(int n, double *a)
{
    int j, kj, kr;
    double ew, wn4r, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    x0r = a[0] + a[2];
    x0i = a[1] + a[3];
    x1r = a[0] - a[2];
    x1i = a[1] - a[3];
    x2r = a[4] + a[6];
    x2i = a[5] + a[7];
    x3r = a[4] - a[6];
    x3i = a[5] - a[7];
    a[0] = x0r + x2r;
    a[1] = x0i + x2i;
    a[4] = x0r - x2r;
    a[5] = x0i - x2i;
    a[2] = x1r - x3i;
    a[3] = x1i + x3r;
    a[6] = x1r + x3i;
    a[7] = x1i - x3r;
    wn4r = WR5000;
    x0r = a[8] + a[10];
    x0i = a[9] + a[11];
    x1r = a[8] - a[10];
    x1i = a[9] - a[11];
    x2r = a[12] + a[14];
    x2i = a[13] + a[15];
    x3r = a[12] - a[14];
    x3i = a[13] - a[15];
    a[8] = x0r + x2r;
    a[9] = x0i + x2i;
    a[12] = x2i - x0i;
    a[13] = x0r - x2r;
    x0r = x1r - x3i;
    x0i = x1i + x3r;
    a[10] = wn4r * (x0r - x0i);
    a[11] = wn4r * (x0r + x0i);
    x0r = x3i + x1r;
    x0i = x3r - x1i;
    a[14] = wn4r * (x0i - x0r);
    a[15] = wn4r * (x0i + x0r);
    ew = MM_PI_2 / n;
    kr = 0;
    for (j = 16; j < n; j += 16) {
        for (kj = n >> 2; kj > (kr ^= kj); kj >>= 1);
        wk1r = cos(ew * kr);
        wk1i = sin(ew * kr);
        wk2r = 1 - 2 * wk1i * wk1i;
        wk2i = 2 * wk1i * wk1r;
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        x0r = a[j] + a[j + 2];
        x0i = a[j + 1] + a[j + 3];
        x1r = a[j] - a[j + 2];
        x1i = a[j + 1] - a[j + 3];
        x2r = a[j + 4] + a[j + 6];
        x2i = a[j + 5] + a[j + 7];
        x3r = a[j + 4] - a[j + 6];
        x3i = a[j + 5] - a[j + 7];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 4] = wk2r * x0r - wk2i * x0i;
        a[j + 5] = wk2r * x0i + wk2i * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 2] = wk1r * x0r - wk1i * x0i;
        a[j + 3] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 6] = wk3r * x0r - wk3i * x0i;
        a[j + 7] = wk3r * x0i + wk3i * x0r;
        x0r = wn4r * (wk1r - wk1i);
        wk1i = wn4r * (wk1r + wk1i);
        wk1r = x0r;
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        x0r = a[j + 8] + a[j + 10];
        x0i = a[j + 9] + a[j + 11];
        x1r = a[j + 8] - a[j + 10];
        x1i = a[j + 9] - a[j + 11];
        x2r = a[j + 12] + a[j + 14];
        x2i = a[j + 13] + a[j + 15];
        x3r = a[j + 12] - a[j + 14];
        x3i = a[j + 13] - a[j + 15];
        a[j + 8] = x0r + x2r;
        a[j + 9] = x0i + x2i;
        x0r -= x2r;
        x0i -= x2i;
        a[j + 12] = -wk2i * x0r - wk2r * x0i;
        a[j + 13] = -wk2i * x0i + wk2r * x0r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j + 10] = wk1r * x0r - wk1i * x0i;
        a[j + 11] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j + 14] = wk3r * x0r - wk3i * x0i;
        a[j + 15] = wk3r * x0i + wk3i * x0r;
    }
}



void 	M_cftmdl(int n, int l, double *a)
{
    int j, j1, j2, j3, k, kj, kr, m, m2;
    double ew, wn4r, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
    double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
    m = l << 2;
    for (j = 0; j < l; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x0r - x2r;
        a[j2 + 1] = x0i - x2i;
        a[j1] = x1r - x3i;
        a[j1 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
    }
    wn4r = WR5000;
    for (j = m; j < l + m; j += 2) {
        j1 = j + l;
        j2 = j1 + l;
        j3 = j2 + l;
        x0r = a[j] + a[j1];
        x0i = a[j + 1] + a[j1 + 1];
        x1r = a[j] - a[j1];
        x1i = a[j + 1] - a[j1 + 1];
        x2r = a[j2] + a[j3];
        x2i = a[j2 + 1] + a[j3 + 1];
        x3r = a[j2] - a[j3];
        x3i = a[j2 + 1] - a[j3 + 1];
        a[j] = x0r + x2r;
        a[j + 1] = x0i + x2i;
        a[j2] = x2i - x0i;
        a[j2 + 1] = x0r - x2r;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j1] = wn4r * (x0r - x0i);
        a[j1 + 1] = wn4r * (x0r + x0i);
        x0r = x3i + x1r;
        x0i = x3r - x1i;
        a[j3] = wn4r * (x0i - x0r);
        a[j3 + 1] = wn4r * (x0i + x0r);
    }
    ew = MM_PI_2 / n;
    kr = 0;
    m2 = 2 * m;
    for (k = m2; k < n; k += m2) {
        for (kj = n >> 2; kj > (kr ^= kj); kj >>= 1);
        wk1r = cos(ew * kr);
        wk1i = sin(ew * kr);
        wk2r = 1 - 2 * wk1i * wk1i;
        wk2i = 2 * wk1i * wk1r;
        wk3r = wk1r - 2 * wk2i * wk1i;
        wk3i = 2 * wk2i * wk1r - wk1i;
        for (j = k; j < l + k; j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = wk2r * x0r - wk2i * x0i;
            a[j2 + 1] = wk2r * x0i + wk2i * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
        x0r = wn4r * (wk1r - wk1i);
        wk1i = wn4r * (wk1r + wk1i);
        wk1r = x0r;
        wk3r = wk1r - 2 * wk2r * wk1i;
        wk3i = 2 * wk2r * wk1r - wk1i;
        for (j = k + m; j < l + (k + m); j += 2) {
            j1 = j + l;
            j2 = j1 + l;
            j3 = j2 + l;
            x0r = a[j] + a[j1];
            x0i = a[j + 1] + a[j1 + 1];
            x1r = a[j] - a[j1];
            x1i = a[j + 1] - a[j1 + 1];
            x2r = a[j2] + a[j3];
            x2i = a[j2 + 1] + a[j3 + 1];
            x3r = a[j2] - a[j3];
            x3i = a[j2 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            x0r -= x2r;
            x0i -= x2i;
            a[j2] = -wk2i * x0r - wk2r * x0i;
            a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j1] = wk1r * x0r - wk1i * x0i;
            a[j1 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r - wk3i * x0i;
            a[j3 + 1] = wk3r * x0i + wk3i * x0r;
        }
    }
}



void 	M_rftfsub(int n, double *a)
{
    int i, i0, j, k;
    double ec, w1r, w1i, wkr, wki, wdr, wdi, ss, xr, xi, yr, yi;
    
    ec = 2 * MM_PI_2 / n;
    wkr = 0;
    wki = 0;
    wdi = cos(ec);
    wdr = sin(ec);
    wdi *= wdr;
    wdr *= wdr;
    w1r = 1 - 2 * wdr;
    w1i = 2 * wdi;
    ss = 2 * w1i;
    i = n >> 1;
    while (1) {
        i0 = i - 4 * RDFT_LOOP_DIV;
        if (i0 < 4) {
            i0 = 4;
        }
        for (j = i - 4; j >= i0; j -= 4) {
            k = n - j;
            xr = a[j + 2] - a[k - 2];
            xi = a[j + 3] + a[k - 1];
            yr = wdr * xr - wdi * xi;
            yi = wdr * xi + wdi * xr;
            a[j + 2] -= yr;
            a[j + 3] -= yi;
            a[k - 2] += yr;
            a[k - 1] -= yi;
            wkr += ss * wdi;
            wki += ss * (0.5 - wdr);
            xr = a[j] - a[k];
            xi = a[j + 1] + a[k + 1];
            yr = wkr * xr - wki * xi;
            yi = wkr * xi + wki * xr;
            a[j] -= yr;
            a[j + 1] -= yi;
            a[k] += yr;
            a[k + 1] -= yi;
            wdr += ss * wki;
            wdi += ss * (0.5 - wkr);
        }
        if (i0 == 4) {
            break;
        }
        wkr = 0.5 * sin(ec * i0);
        wki = 0.5 * cos(ec * i0);
        wdr = 0.5 - (wkr * w1r - wki * w1i);
        wdi = wkr * w1i + wki * w1r;
        wkr = 0.5 - wkr;
        i = i0;
    }
    xr = a[2] - a[n - 2];
    xi = a[3] + a[n - 1];
    yr = wdr * xr - wdi * xi;
    yi = wdr * xi + wdi * xr;
    a[2] -= yr;
    a[3] -= yi;
    a[n - 2] += yr;
    a[n - 1] -= yi;
}



void 	M_rftbsub(int n, double *a)
{
    int i, i0, j, k;
    double ec, w1r, w1i, wkr, wki, wdr, wdi, ss, xr, xi, yr, yi;
    
    ec = 2 * MM_PI_2 / n;
    wkr = 0;
    wki = 0;
    wdi = cos(ec);
    wdr = sin(ec);
    wdi *= wdr;
    wdr *= wdr;
    w1r = 1 - 2 * wdr;
    w1i = 2 * wdi;
    ss = 2 * w1i;
    i = n >> 1;
    a[i + 1] = -a[i + 1];
    while (1) {
        i0 = i - 4 * RDFT_LOOP_DIV;
        if (i0 < 4) {
            i0 = 4;
        }
        for (j = i - 4; j >= i0; j -= 4) {
            k = n - j;
            xr = a[j + 2] - a[k - 2];
            xi = a[j + 3] + a[k - 1];
            yr = wdr * xr + wdi * xi;
            yi = wdr * xi - wdi * xr;
            a[j + 2] -= yr;
            a[j + 3] = yi - a[j + 3];
            a[k - 2] += yr;
            a[k - 1] = yi - a[k - 1];
            wkr += ss * wdi;
            wki += ss * (0.5 - wdr);
            xr = a[j] - a[k];
            xi = a[j + 1] + a[k + 1];
            yr = wkr * xr + wki * xi;
            yi = wkr * xi - wki * xr;
            a[j] -= yr;
            a[j + 1] = yi - a[j + 1];
            a[k] += yr;
            a[k + 1] = yi - a[k + 1];
            wdr += ss * wki;
            wdi += ss * (0.5 - wkr);
        }
        if (i0 == 4) {
            break;
        }
        wkr = 0.5 * sin(ec * i0);
        wki = 0.5 * cos(ec * i0);
        wdr = 0.5 - (wkr * w1r - wki * w1i);
        wdi = wkr * w1i + wki * w1r;
        wkr = 0.5 - wkr;
        i = i0;
    }
    xr = a[2] - a[n - 2];
    xi = a[3] + a[n - 1];
    yr = wdr * xr + wdi * xi;
    yi = wdr * xi - wdi * xr;
    a[2] -= yr;
    a[3] = yi - a[3];
    a[n - 2] += yr;
    a[n - 1] = yi - a[n - 1];
    a[1] = -a[1];
}

