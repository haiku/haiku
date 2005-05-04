/********************************************************************\
 *  FILE:     rmd160.h
 *  CONTENTS: Header file for a sample C-implementation of the
 *            RIPEMD-160 hash-function.
 *  AUTHOR:   Antoon Bosselaers, Dept. Electrical Eng.-ESAT/COSIC
 *  DATE:     1 March 1996       VERSION:  1.0
\********************************************************************/

#ifndef  RMD160H           /* make sure this file is read only once */
#define  RMD160H

/********************************************************************/
/* Type definitions of an 8 and a 32-bit integer type, respectively.
   Adapt these, if necessary, for your operating system and compiler
*/
typedef    unsigned char        byte;   /* unsigned 8-bit integer */
typedef    unsigned long        word;   /* unsigned 32-bit integer */

/********************************************************************/
/* Macro definitions */

/* ROL(x, n) cyclically rotates x over n bits to the left
   x must be of an unsigned 32 bits type and 0 <= n < 32.
*/
#define ROL(x, n)        (((x) << (n)) | ((x) >> (32-(n))))

/* The five basic RIPEMD-160 functions F1(), F2(), F3(), F4(), and F5()
*/
#define F1(x, y, z)        ((x) ^ (y) ^ (z))
#define F2(x, y, z)        (((x) & (y)) | (~(x) & (z)))
#define F3(x, y, z)        (((x) | ~(y)) ^ (z))
#define F4(x, y, z)        (((x) & (z)) | ((y) & ~(z)))
#define F5(x, y, z)        ((x) ^ ((y) | ~(z)))

/* The ten basic RIPEMD-160 transformations FF1() through FFF5()
*/
#define FF1(a, b, c, d, e, x, s)        {\
      (a) += F1((b), (c), (d)) + (x);\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FF2(a, b, c, d, e, x, s)        {\
      (a) += F2((b), (c), (d)) + (x) + 0x5a827999UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FF3(a, b, c, d, e, x, s)        {\
      (a) += F3((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FF4(a, b, c, d, e, x, s)        {\
      (a) += F4((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FF5(a, b, c, d, e, x, s)        {\
      (a) += F5((b), (c), (d)) + (x) + 0xa953fd4eUL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FFF1(a, b, c, d, e, x, s)        {\
      (a) += F1((b), (c), (d)) + (x);\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FFF2(a, b, c, d, e, x, s)        {\
      (a) += F2((b), (c), (d)) + (x) + 0x7a6d76e9UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FFF3(a, b, c, d, e, x, s)        {\
      (a) += F3((b), (c), (d)) + (x) + 0x6d703ef3UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FFF4(a, b, c, d, e, x, s)        {\
      (a) += F4((b), (c), (d)) + (x) + 0x5c4dd124UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }
#define FFF5(a, b, c, d, e, x, s)        {\
      (a) += F5((b), (c), (d)) + (x) + 0x50a28be6UL;\
      (a) = ROL((a), (s)) + (e);\
      (c) = ROL((c), 10);\
   }

/********************************************************************/
/* Function prototypes */

void MDinit(word *MDbuf);
void MDcompress(word *MDbuf, word *X);
void MDfinish(word *MDbuf, byte *string, word lswlen, word mswlen);

#endif  /* RMD160H */

/*********************** end of file rmd160.h ***********************/

