/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2002-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen, marcus@overhagen.de
 *		Jerome Duval, jerome.duval@free.fr
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define NUM_CARDS 3
#define DEVNAME 32

typedef struct
{
	uint32	base;
	uint32	irq;
	uint32	type;
} device_config;

#endif
