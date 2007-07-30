/*
 * ES1370 Haiku Driver for ES1370 audio
 *
 * Copyright 2002-2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval, jerome.duval@free.fr
 */

#ifndef _MULTI_H_
#define _MULTI_H_

typedef struct _multi_mixer_control {
	struct _multi_dev 	*multi;
	void	(*get) (void *card, const void *cookie, int32 type, float *values);
	void	(*set) (void *card, const void *cookie, int32 type, float *values);
	const void    *cookie;
	int32 type;
	multi_mix_control	mix_control;
} multi_mixer_control;

#define EMU_MULTI_CONTROL_FIRSTID	1024
#define EMU_MULTI_CONTROL_MASTERID	0

typedef struct _multi_dev {
	void	*card;
#define EMU_MULTICONTROLSNUM 64
	multi_mixer_control controls[EMU_MULTICONTROLSNUM];
	uint32 control_count;
	
#define EMU_MULTICHANNUM 64
	multi_channel_info chans[EMU_MULTICHANNUM];
	uint32 output_channel_count;
	uint32 input_channel_count;
	uint32 output_bus_channel_count;
	uint32 input_bus_channel_count;
	uint32 aux_bus_channel_count;
} multi_dev;

#endif
