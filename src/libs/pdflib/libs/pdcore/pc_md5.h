/* $Id: pc_md5.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Header file for the PDFlib MD5 message digest routines
 *
 */

/* This is a slightly modified version of the RSA reference
 * implementation for MD5, which originally contained
 * the following copyright notice:
 */

/*  Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
    rights reserved.

    License to copy and use this software is granted provided that it
    is identified as the "RSA Data Security, Inc. MD5 Message-Digest
    Algorithm" in all material mentioning or referencing this software
    or this function.

    License is also granted to make and use derivative works provided
    that such works are identified as "derived from the RSA Data
    Security, Inc. MD5 Message-Digest Algorithm" in all material
    mentioning or referencing the derived work.

    RSA Data Security, Inc. makes no representations concerning either
    the merchantability of this software or the suitability of this
    software for any particular purpose. It is provided "as is"
    without express or implied warranty of any kind.

    These notices must be retained in any copies of any part of this
    documentation and/or software.
 */


/* we prefix our MD5 function names with "pdc_", so you can
 * link your program with another MD5 lib without troubles.
 */
#define MD5_Init	pdc_MD5_Init
#define MD5_Update	pdc_MD5_Update
#define MD5_Final	pdc_MD5_Final

typedef unsigned int MD5_UINT4;

#define MD5_DIGEST_LENGTH	16


/* MD5 context. */
typedef struct {
    MD5_UINT4 state[4];		/* state (ABCD) */
    MD5_UINT4 count[2];		/* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];	/* input buffer */
} MD5_CTX;

void MD5_Init(MD5_CTX *context);
void MD5_Update(
	MD5_CTX *context, const unsigned char *input, unsigned int inputLen);
void MD5_Final(unsigned char digest[MD5_DIGEST_LENGTH], MD5_CTX *context);
