/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <math.h>
#include <stdio.h>

#include "support.h"

#include "lab_convert.h"

#define GAMMA_ZERO_ENTRIES 256
#define GAMMA_ENTRIES 10240
#define GAMMA_MAX_ENTRIES 256
#define GAMMA_TOTAL_ENTRIES GAMMA_ZERO_ENTRIES + GAMMA_ENTRIES + GAMMA_MAX_ENTRIES

// init_gamma_table
uint8*
init_gamma_table()
{
	uint8* table = new uint8[GAMMA_TOTAL_ENTRIES];
	for (int32 i = 0; i < GAMMA_ZERO_ENTRIES; i++)
		table[i] = 0;
	for (int32 i = 0; i < GAMMA_ENTRIES; i++)
		table[i + GAMMA_ZERO_ENTRIES] = (uint8)(pow((float)i / (float)(GAMMA_ENTRIES - 1), 0.4) * 255.0 + 0.5);
	for (int32 i = 0; i < GAMMA_MAX_ENTRIES; i++)
		table[i + GAMMA_ZERO_ENTRIES + GAMMA_ENTRIES] = 255;
	return table;
}

// init_linear_table
float*
init_linear_table()
{
	float* table = new float[256];
	for (int32 i = 0; i < 256; i++)
		table[i] = pow((float)i / 255.0, 2.5);
	return table;
}

// conversion from RGB (0...255) to linear and normalized RGB (0...1)
static uint8* gammaTable = init_gamma_table();
static float* linearTable = init_linear_table();

// matrix entries: XYZ -> RGB (709 RGB, D65 Whitepoint)
/*const float Rx = 3.240479;
const float Ry = -1.537150;
const float Rz = -0.498535;
const float Gx = -0.969256;
const float Gy = 1.875992;
const float Gz = 0.041556;
const float Bx = 0.055648;
const float By = -0.204043;
const float Bz = 1.057311;*/

// matrix entries: XYZ -> sRGB (D65 Whitepoint)
const float Rx = 3.24071;
const float Ry = -1.53726;
const float Rz = -0.498571;
const float Gx = -0.969258;
const float Gy = 1.87599;
const float Gz = 0.0415557;
const float Bx = 0.0556352;
const float By = -0.203996;
const float Bz = 1.05707;


// matrix entries scaled: XYZ(0...1) -> RGB(0...255)
const float RX = Rx * 255;
const float RY = Ry * 255;
const float RZ = Rz * 255;
const float GX = Gx * 255;
const float GY = Gy * 255;
const float GZ = Gz * 255;
const float BX = Bx * 255;
const float BY = By * 255;
const float BZ = Bz * 255;

// matrix etries: RGB -> XYZ
/*const float Xr = 0.412453;
const float Xg = 0.357580;
const float Xb = 0.189423;
const float Yr = 0.212671;
const float Yg = 0.715160;
const float Yb = 0.072169;
const float Zr = 0.019334;
const float Zg = 0.119193;
const float Zb = 0.950227;*/

// matrix etries: sRGB -> XYZ (D65 Whitepoint)
const float Xr = 0.412424;
const float Xg = 0.357579;
const float Xb = 0.180464;
const float Yr = 0.212656;
const float Yg = 0.715158;
const float Yb = 0.0721856;
const float Zr = 0.0193324;
const float Zg = 0.119193;
const float Zb = 0.950444;

// matrix entries scaled: RGB(0...255) -> XYZ(0...1)
const float XR = Xr / 255;
const float XG = Xg / 255;
const float XB = Xb / 255;
const float YR = Yr / 255;
const float YG = Yg / 255;
const float YB = Yb / 255;
const float ZR = Zr / 255;
const float ZG = Zg / 255;
const float ZB = Zb / 255;

// white point
const float Xn = Xr + Xg + Xb;
const float Yn = Yr + Yg + Yb;
const float Zn = Zr + Zg + Zb;

// matrix entries scaled and white point normalized: RGB(0...255) -> XYZ(0...1/WP)
const float XRn = Xr / Xn;
const float XGn = Xg / Xn;
const float XBn = Xb / Xn;
const float YRn = Yr / Yn;
const float YGn = Yg / Yn;
const float YBn = Yb / Yn;
const float ZRn = Zr / Zn;
const float ZGn = Zg / Zn;
const float ZBn = Zb / Zn;

// lab2rgb
void
lab2rgb(float L, float a, float b, uint8& R, uint8& G, uint8& B)
{
	float P = (L + 16) / 116;
	float Pa500 = P + a / 500;
	float Pb200 = P - b / 200;
	float X = Xn * Pa500 * Pa500 * Pa500;
	float Y = Yn * P * P * P;
	float Z = Zn * Pb200 * Pb200 * Pb200;
/*	float linearR = max_c(0.0, min_c(1.0, Rx * X + Ry * Y + Rz * Z));
	float linearG = max_c(0.0, min_c(1.0, Gx * X + Gy * Y + Gz * Z));
	float linearB = max_c(0.0, min_c(1.0, Bx * X + By * Y + Bz * Z));
	R = (uint8)(pow(linearR, 0.4) * 255.0 + 0.5);
	G = (uint8)(pow(linearG, 0.4) * 255.0 + 0.5);
	B = (uint8)(pow(linearB, 0.4) * 255.0 + 0.5);*/
	float linearR = Rx * X + Ry * Y + Rz * Z;
	float linearG = Gx * X + Gy * Y + Gz * Z;
	float linearB = Bx * X + By * Y + Bz * Z;
	R = (uint8)constrain_int32_0_255((int32)(pow(linearR, 0.4) * 255.0 + 0.5));
	G = (uint8)constrain_int32_0_255((int32)(pow(linearG, 0.4) * 255.0 + 0.5));
	B = (uint8)constrain_int32_0_255((int32)(pow(linearB, 0.4) * 255.0 + 0.5));
/*	float linearR = Rx * X + Ry * Y + Rz * Z;
	float linearG = Gx * X + Gy * Y + Gz * Z;
	float linearB = Bx * X + By * Y + Bz * Z;
	R = gammaTable[(uint32)(linearR * (GAMMA_ENTRIES - 1) + 0.5) + GAMMA_ZERO_ENTRIES];
	G = gammaTable[(uint32)(linearG * (GAMMA_ENTRIES - 1) + 0.5) + GAMMA_ZERO_ENTRIES];
	B = gammaTable[(uint32)(linearB * (GAMMA_ENTRIES - 1) + 0.5) + GAMMA_ZERO_ENTRIES];*/
}

inline float
f(float t)
{
	if (t > 0.008856)
		return pow(t, 1.0 / 3.0);
	return 7.787 * t + 16.0 / 116;
}

// rgb2lab
void
rgb2lab(uint8 R, uint8 G, uint8 B, float& L, float& a, float& b)
{
/*	float linearR = pow((float)R / 255.0, 2.5);
	float linearG = pow((float)G / 255.0, 2.5);
	float linearB = pow((float)B / 255.0, 2.5);*/
	float linearR = linearTable[R];
	float linearG = linearTable[G];
	float linearB = linearTable[B];
	float Xq = XRn * linearR + XGn * linearG + XBn * linearB; // == X/Xn
	float Yq = YRn * linearR + YGn * linearG + YBn * linearB; // == Y/Yn
	float Zq = ZRn * linearR + ZGn * linearG + ZBn * linearB; // == Z/Zn
	if (Yq > 0.008856)
		L = 116.0 * pow(Yq, 1.0 / 3.0) - 16;
	else
		L = 903.3 * Yq;
	a = 500 * (f(Xq) - f(Yq));
	b = 200 * (f(Yq) - f(Zq));
}

// replace_luminance
void
replace_luminance(uint8& r1, uint8& g1, uint8& b1, uint8 r2, uint8 g2, uint8 b2)
{
	float CIEL1, CIEa1, CIEb1, CIEL2;//, CIEa2, CIEb2;
	rgb2lab(r1, g1, b1, CIEL1, CIEa1, CIEb1);
//	rgb2lab(r2, g2, b2, CIEL2, CIEa2, CIEb2);
	CIEL2 = ((linearTable[r2] + linearTable[g2] + linearTable[b2]) / 3.0) * 100.0;
	lab2rgb(CIEL2, CIEa1, CIEb1, r1, g1, b1);
}
