/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <HSL.h>


hsl_color
hsl_color::from_rgb(const rgb_color& rgb)
{
	hsl_color result;

	float r = rgb.red / 255.0f;
	float g = rgb.green / 255.0f;
	float b = rgb.blue / 255.0f;

	float max = max_c(max_c(r, g), b);
	float min = min_c(min_c(r, g), b);

	result.hue = result.saturation = result.lightness = (max + min) / 2;

	if (max == min) {
		// grayscale
		result.hue = result.saturation = 0;
	} else {
		float diff = max - min;
		result.saturation
			= (result.lightness > 0.5) ? (diff / (2 - max - min)) : (diff / (max + min));

		if (max == r)
			result.hue = (g - b) / diff + (g < b ? 6 : 0);
		else if (max == g)
			result.hue = (b - r) / diff + 2;
		else if (max == b)
			result.hue = (r - g) / diff + 4;

		result.hue /= 6;
	}

	return result;
}


rgb_color
hsl_color::to_rgb() const
{
	rgb_color result;
	result.alpha = 255;

	if (saturation == 0) {
		// grayscale
		result.red = result.green = result.blue = uint8(lightness * 255);
	} else {
		float q = lightness < 0.5 ? (lightness * (1 + saturation))
			: (lightness + saturation - lightness * saturation);
		float p = 2 * lightness - q;
		result.red = uint8(hue_to_rgb(p, q, hue + 1./3) * 255);
		result.green = uint8(hue_to_rgb(p, q, hue) * 255);
		result.blue = uint8(hue_to_rgb(p, q, hue - 1./3) * 255);
	}

	return result;
}


// reference: https://en.wikipedia.org/wiki/HSL_and_HSV#Color_conversion_formulae
// (from_rgb() and to_rgb() are derived from the same)
float
hsl_color::hue_to_rgb(float p, float q, float t)
{
	if (t < 0)
		t += 1;
	if (t > 1)
		t -= 1;
	if (t < 1./6)
		return p + (q - p) * 6 * t;
	if (t < 1./2)
		return q;
	if (t < 2./3)
		return p + (q - p) * (2./3 - t) * 6;

	return p;
}
