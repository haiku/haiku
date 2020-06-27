/* 
** Copyright 2004, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the MIT license.
**
** This file generates the lookup tables used by image
** format conversion from planar YCbCr420 format to
** linear RGB32 format in file gfx_conv_c_lookup.cpp
*/

#include <strings.h>
#include <stdio.h>

double precision = 32768.0;
int shift = 15;

/*
	R = 1.164(Y - 16) + 1.596(Cr - 128)
	G = 1.164(Y - 16) - 0.813(Cr - 128) - 0.392(Cb - 128)
	B = 1.164(Y - 16) + 2.017(Cb - 128)
*/

int main()
{
	printf("const int32 Cb_Gtab[256] = {\n\t");
	for (int i = 0; i < 256; i++) {
		int	Cr_R = (- 128 + i) * 1.596 * precision + 0.5;
		int	Cr_G = (- 128 + i) * -0.813 * precision + 0.5;
		int	Cb_G = (- 128 + i) * -0.392 * precision + 0.5;
		int	Cb_B = (- 128 + i) * 2.017 * precision + 0.5;
		printf("%d, ", Cb_G);	
		if ((i % 10) == 9)
			printf("\n\t");
	 }
	printf("\n};\n\n");

	printf("const int32 Cb_Btab[256] = {\n\t");
	for (int i = 0; i < 256; i++) {
		int	Cr_R = (- 128 + i) * 1.596 * precision + 0.5;
		int	Cr_G = (- 128 + i) * -0.813 * precision + 0.5;
		int	Cb_G = (- 128 + i) * -0.392 * precision + 0.5;
		int	Cb_B = (- 128 + i) * 2.017 * precision + 0.5;
		printf("%d, ", Cb_B);	
		if ((i % 10) == 9)
			printf("\n\t");
	 }
	printf("\n};\n\n");

	printf("const int32 Cr_Rtab[256] = {\n\t");
	for (int i = 0; i < 256; i++) {
		int	Cr_R = (- 128 + i) * 1.596 * precision + 0.5;
		int	Cr_G = (- 128 + i) * -0.813 * precision + 0.5;
		int	Cb_G = (- 128 + i) * -0.392 * precision + 0.5;
		int	Cb_B = (- 128 + i) * 2.017 * precision + 0.5;
		printf("%d, ", Cr_R);	
		if ((i % 10) == 9)
			printf("\n\t");
	 }
	printf("\n};\n\n");

	printf("const int32 Cr_Gtab[256] = {\n\t");
	for (int i = 0; i < 256; i++) {
		int	Cr_R = (- 128 + i) * 1.596 * precision + 0.5;
		int	Cr_G = (- 128 + i) * -0.813 * precision + 0.5;
		int	Cb_G = (- 128 + i) * -0.392 * precision + 0.5;
		int	Cb_B = (- 128 + i) * 2.017 * precision + 0.5;
		printf("%d, ", Cr_G);	
		if ((i % 10) == 9)
			printf("\n\t");
	 }
	printf("\n};\n\n");

	printf("const int32 Ytab[256] = {\n\t");
	for (int i = 0; i < 256; i++) {
		int Y = (i - 16) * 1.164 * precision + 0.5;
		printf("%d, ", Y);	
		if ((i % 10) == 9)
			printf("\n\t");
	 }
	printf("\n};\n\n");

	int Ymin = (0 - 16) * 1.164 * precision + 0.5;
	int Ymax = (255 - 16) * 1.164 * precision + 0.5;

	int Cr_Rmin = (- 128 + 0) * 1.596 * precision + 0.5;
	int Cr_Rmax = (- 128 + 255) * 1.596 * precision + 0.5;
	int Cr_Gmin = (- 128 + 255) * -0.813 * precision + 0.5;
	int Cr_Gmax = (- 128 + 0) * -0.813 * precision + 0.5;
	int Cb_Gmin = (- 128 + 255) * -0.392 * precision + 0.5;
	int Cb_Gmax = (- 128 + 0) * -0.392 * precision + 0.5;
	int Cb_Bmin = (- 128 + 0) * 2.017 * precision + 0.5;
	int Cb_Bmax = (- 128 + 255) * 2.017 * precision + 0.5;

	printf("Cr_Rmin %d\n", Cr_Rmin);
	printf("Cr_Rmax %d\n", Cr_Rmax);
	printf("Cr_Gmin %d\n", Cr_Gmin);
	printf("Cr_Gmax %d\n", Cr_Gmax);
	printf("Cb_Gmin %d\n", Cb_Gmin);
	printf("Cb_Gmax %d\n", Cb_Gmax);
	printf("Cb_Bmin %d\n", Cb_Bmin);
	printf("Cb_Bmax %d\n", Cb_Bmax);
	
	int Rmax = (Ymax + Cr_Rmax) >> shift;
	int Rmin = (Ymin + Cr_Rmin) >> shift;
	int Gmax = (Ymax + Cr_Gmax + Cb_Gmax) >> shift;
	int Gmin = (Ymin + Cr_Gmin + Cb_Gmin) >> shift;
	int Bmax = (Ymax + Cb_Bmax) >> shift;
	int Bmin = (Ymin + Cb_Bmin) >> shift;
	
	printf("Rmax %d\n", Rmax);
	printf("Rmin %d\n", Rmin);
	printf("Gmax %d\n", Gmax);
	printf("Gmin %d\n", Gmin);
	printf("Bmax %d\n", Bmax);
	printf("Bmin %d\n", Bmin);
	
	int num_Rneg = (Rmin < 0) ? -Rmin : 0;
	int num_Rpos = (Rmax > 255) ? Rmax - 255 : 0;
	int num_Rent = num_Rneg + 256 + num_Rpos;

	printf("num_Rneg %d\n", num_Rneg);
	printf("num_Rpos %d\n", num_Rpos);
	printf("num_Rent %d\n", num_Rent);
	
	printf("const uint32 Rsat[%d] = {\n\t", num_Rent);
	for (int i = 0; i < num_Rneg; i++) {
		printf("0x000000, ");	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < 256; i++) {
		printf("0x%06x, ", i << 16);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < num_Rpos; i++) {
		printf("0x%06x, ", 255 << 16);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n};\n");
	printf("const uint32 *Rtab = &Rsat[%d];\n\n", num_Rneg);
	
	
	
	int num_Gneg = (Gmin < 0) ? -Gmin : 0;
	int num_Gpos = (Gmax > 255) ? Gmax - 255 : 0;
	int num_Gent = num_Gneg + 256 + num_Gpos;

	printf("num_Gneg %d\n", num_Gneg);
	printf("num_Gpos %d\n", num_Gpos);
	printf("num_Gent %d\n", num_Gent);
	
	printf("const uint32 Gsat[%d] = {\n\t", num_Gent);
	for (int i = 0; i < num_Gneg; i++) {
		printf("0x000000, ");	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < 256; i++) {
		printf("0x%06x, ", i << 8);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < num_Gpos; i++) {
		printf("0x%06x, ", 255 << 8);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n};\n");
	printf("const uint32 *Gtab = &Gsat[%d];\n\n", num_Gneg);
	
	
	int num_Bneg = (Bmin < 0) ? -Bmin : 0;
	int num_Bpos = (Bmax > 255) ? Bmax - 255 : 0;
	int num_Bent = num_Bneg + 256 + num_Bpos;

	printf("num_Bneg %d\n", num_Bneg);
	printf("num_Bpos %d\n", num_Bpos);
	printf("num_Bent %d\n", num_Bent);
	
	printf("const uint32 Bsat[%d] = {\n\t", num_Bent);
	for (int i = 0; i < num_Bneg; i++) {
		printf("0x000000, ");	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < 256; i++) {
		printf("0x%06x, ", i << 0);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n\t");
	for (int i = 0; i < num_Bpos; i++) {
		printf("0x%06x, ", 255 << 0);	
		if ((i % 10) == 9)
			printf("\n\t");
	}
	printf("\n};\n");
	printf("const uint32 *Btab = &Bsat[%d];\n\n", num_Bneg);
	
	return 0;
}
