/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HSL_H
#define _HSL_H


#include <GraphicsDefs.h>


typedef struct hsl_color {
	float hue, saturation, lightness;

	static	hsl_color	from_rgb(const rgb_color& rgb);
			rgb_color	to_rgb() const;

private:
	static	float		hue_to_rgb(float p, float q, float t);
} hsl_color;


#endif // _HSL_H
