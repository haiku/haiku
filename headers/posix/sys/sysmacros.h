/*
** Distributed under the terms of the Haiku License.
*/
#ifndef _SYSMACROS_H_
#define _SYSMACROS_H_

/* from FreeBSD */

#define major(x)        ((int)(((u_int)(x) >> 8)&0xff)) /* major number */
#define minor(x)        ((int)((x)&0xffff00ff))         /* minor number */

#define makedev(x,y)    ((dev_t)(((x) << 8) | (y)))     /* create dev_t */

#endif // _SYSMACROS_H_
