/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000BITS_H_
#define _ET6000BITS_H_


/*****************************************************************************/
__inline void set8(volatile char *addr, char mask, char val);
__inline void set16(volatile short *addr, short mask, short val);
__inline void set32(volatile int *addr, int mask, int val);
__inline void ioSet8(short port, char mask, char val);
__inline char ioGet8(short port);
/*****************************************************************************/


#endif /* _ET6000BITS_H_ */
