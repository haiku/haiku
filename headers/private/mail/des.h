#ifndef ZOIDBERG_DES_H
#define ZOIDBERG_DES_H
/* DES - encryption algorithm, removed double and triple DES
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/

/* des.h - adapted from d3des.h:
 *
 *	Headers and defines for d3des.c
 *	Graven Imagery, 1992.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
 *	(GEnie : OUTER; CIS : [71755,204])
 */

#define DES_ENCRYPT	0	/* MODE == encrypt */
#define DES_DECRYPT	1	/* MODE == decrypt */


#ifdef __cplusplus
extern "C" {
#endif

extern void des_setkey(unsigned char *, short);
/*		      hexkey[8]     MODE
 * Sets the internal key register according to the hexadecimal
 * key contained in the 8 bytes of hexkey, according to the DES,
 * for encryption or decryption according to MODE.
 */

extern void des_usekey(unsigned long *);
/*		    cookedkey[32]
 * Loads the internal key register with the data in cookedkey.
 */

extern void des_cpkey(unsigned long *);
/*		   cookedkey[32]
 * Copies the contents of the internal key register into the storage
 * located at &cookedkey[0].
 */

extern void des_crypt(unsigned char *, unsigned char *);
/*		    from[8]	      to[8]
 * Encrypts/Decrypts (according to the key currently loaded in the
 * internal key register) one block of eight bytes at address 'from'
 * into the block at address 'to'.  They can be the same.
 */

extern void des_encrypt(char *from,char *to);
extern void des_decrypt(char *from,int fromLength,char *to);

#ifdef __cplusplus
}
#endif

#endif	/* ZOIDBERG_DES_H */
