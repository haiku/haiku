/*
** Copyright 2002, Michael Noisternig. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_SELECTOR_H
#define _KERNEL_ARCH_x86_SELECTOR_H

typedef uint32 selector_id;
typedef uint64 selector_type;

// DATA segments are read-only
// CODE segments are execute-only
// both can be modified by using the suffixed enum versions
// legend:	w = writable
//			d = expand down
//			r = readable
//			c = conforming
enum segment_type {
	DATA = 0x8, DATA_w, DATA_d, DATA_wd, CODE, CODE_r, CODE_c, CODE_rc
};

#define SELECTOR(base,limit,type,mode32) \
	(((uint64)(((((uint32)base)>>16)&0xff) | (((uint32)base)&0xff000000) | ((type)<<9) | ((mode32)<<22) | (1<<15))<<32) \
	| ( (limit) >= (1<<20) ? (((limit)>>12)&0xffff) | ((uint64)(((limit)>>12)&0xf0000)<<32) | ((uint64)1<<(23+32)) : ((limit)&0xffff) | ((uint64)((limit)&0xf0000)<<32) ) \
	| ((((uint32)base)&0xffff)<<16))

void			i386_selector_init( void *gdt );
selector_id		i386_selector_add( selector_type selector );
void			i386_selector_remove( selector_id id );
selector_type	i386_selector_get( selector_id id );

#endif
