/*
 * For license terms, see the file COPYING in this directory.
 *
 * md5global.h    Global declarations for MD5 module used by fetchmail
 *
 */

#ifndef MD5GLOBAL_H__
#define MD5GLOBAL_H__ 
/* GLOBAL.H - RSAREF types and constants
 */

/* force prototypes on, we need ANSI C anyway */
#ifndef PROTOTYPES
#define PROTOTYPES 1
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
#if SIZEOF_INT == 4
typedef unsigned int UINT4;
#else
typedef unsigned long int UINT4;
#endif

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

#endif  /* MD5GLOBAL_H__ */
