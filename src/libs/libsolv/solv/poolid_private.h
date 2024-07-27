/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * poolid_private.h
 * 
 */

#ifndef LIBSOLV_POOLID_PRIVATE_H
#define LIBSOLV_POOLID_PRIVATE_H

/* the size of all buffers is incremented in blocks
 * these are the block values (increment values) for the
 * rel hashtable
 */
#define REL_BLOCK		1023	/* hashtable for relations */
#define WHATPROVIDES_BLOCK	1023

#endif /* LIBSOLV_POOLID_PRIVATE_H */
