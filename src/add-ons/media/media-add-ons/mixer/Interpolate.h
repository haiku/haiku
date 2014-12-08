/*
 * Copyright 2010-2014, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _INTERPOLATE_H
#define _INTERPOLATE_H


#include "Resampler.h"


class Interpolate: public Resampler {
public:
							Interpolate(uint32 sourceFormat,
								uint32 destFormat);

			float			fOldSample;
};


#endif
