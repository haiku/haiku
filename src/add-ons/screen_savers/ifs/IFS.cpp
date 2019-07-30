/*
 * Copyright (c) 1997 by Massimino Pascal <Pascal.Massimon@ens.fr>
 * Copyright 2006-2014, Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Massimino Pascal, Pascal.Massimon@ens.fr
 *		John Scipione, jscipione@gmail.com
 */

/*! When shown ifs, Diana Rose (4 years old) said, "It looks like dancing."
 */

#include "IFS.h"

#include <new>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <OS.h>
#include <Screen.h>
#include <View.h>

#include <unistd.h>
	// for getpid()
#include <sys/time.h>
	// for gettimeofday()


#define HALF 0
#define random() ya_random()

#define FLOAT_TO_INT(x) (int32)((float)(UNIT)*(x))

#define LRAND() ((long) (random() & 0x7fffffff))
#define NRAND(n) ((int) (LRAND() % (n)))
#define MAXRAND (2147483648.0)
	// unsigned 1<<31 as a float
#define SRAND(n)
	// already seeded by screenhack.c TODO: ?!? is it?

// The following 'random' numbers are taken from CRC, 18th Edition, page 622.
// Each array element was taken from the corresponding line in the table,
// except that a[0] was from line 100. 8s and 9s in the table were simply
// skipped. The high order digit was taken mod 4.

#define VECTOR_SIZE 55

static unsigned int a[VECTOR_SIZE] = {
	035340171546, 010401501101, 022364657325, 024130436022, 002167303062, //  5
	037570375137, 037210607110, 016272055420, 023011770546, 017143426366, // 10
	014753657433, 021657231332, 023553406142, 004236526362, 010365611275, // 14
	007117336710, 011051276551, 002362132524, 001011540233, 012162531646, // 20
	007056762337, 006631245521, 014164542224, 032633236305, 023342700176, // 25
	002433062234, 015257225043, 026762051606, 000742573230, 005366042132, // 30
	012126416411, 000520471171, 000725646277, 020116577576, 025765742604, // 35
	007633473735, 015674255275, 017555634041, 006503154145, 021576344247, // 40
	014577627653, 002707523333, 034146376720, 030060227734, 013765414060, // 45
	036072251540, 007255221037, 024364674123, 006200353166, 010126373326, // 50
	015664104320, 016401041535, 016215305520, 033115351014, 017411670323  // 55
};


static int i1;
static int i2;


unsigned int
ya_random(void)
{
	register int ret = a[i1] + a[i2];
	a[i1] = ret;
	if (++i1 >= VECTOR_SIZE)
		i1 = 0;

	if (++i2 >= VECTOR_SIZE)
		i2 = 0;

	return ret;
}


void
ya_rand_init(unsigned int seed)
{
	int i;
	if (seed == 0) {
		struct timeval tp;
		struct timezone tzp;
		gettimeofday(&tp, &tzp);
		// ignore overflow
		seed = (999*tp.tv_sec) + (1001*tp.tv_usec) + (1003 * getpid());
	}

	a[0] += seed;
	for (i = 1; i < VECTOR_SIZE; i++) {
		seed = a[i-1]*1001 + seed*999;
		a[i] += seed;
	}

	i1 = a[0] % VECTOR_SIZE;
	i2 = (i1 + 24) % VECTOR_SIZE;
}



static float
gauss_rand(float c, float A, float S)
{
	float y = (float) LRAND() / MAXRAND;
	y = A * (1.0 - exp(-y * y * S)) / (1.0 - exp(-S));
	if (NRAND(2))
		return (c + y);

	return (c - y);
}


static float
half_gauss_rand(float c, float A, float S)
{
	float y = (float) LRAND() / MAXRAND;
	y = A * (1.0 - exp(-y * y * S)) / (1.0 - exp(-S));

	return (c + y);
}


inline void
transform(SIMILITUDE* Similitude, int32 xo, int32 yo, int32* x, int32* y)
{
	int32 xx;
	int32 yy;

	xo = xo - Similitude->Cx;
	xo = (xo * Similitude->R) / UNIT;
	yo = yo - Similitude->Cy;
	yo = (yo * Similitude->R) / UNIT;

	xx = xo - Similitude->Cx;
	xx = (xx * Similitude->R2) / UNIT;
	yy = -yo - Similitude->Cy;
	yy = (yy * Similitude->R2) / UNIT;

	*x = ((xo * Similitude->Ct - yo * Similitude->St + xx * Similitude->Ct2
		- yy * Similitude->St2) / UNIT) + Similitude->Cx;
	*y = ((xo * Similitude->St + yo * Similitude->Ct + xx * Similitude->St2
		+ yy * Similitude->Ct2) / UNIT) + Similitude->Cy;
}



IFS::IFS(BRect bounds)
	:
	fRoot(NULL),
	fCurrentFractal(NULL),
	fPointBuffer(NULL),
	fCurrentPoint(0),
	fAdditive(false),
	fCurrentMarkValue(1)
{
	if (!bounds.IsValid())
		return;
	
	ya_rand_init(system_time());

	int i;
	FRACTAL* Fractal;

	if (fRoot == NULL) {
		fRoot = (FRACTAL*)calloc(1, sizeof(FRACTAL));
		if (fRoot == NULL)
			return;
	}
	Fractal = fRoot;

	_FreeBuffers(Fractal);
	i = (NRAND(4)) + 2;
		// Number of centers
	switch (i) {
		case 2:
		default:
			Fractal->Depth = fAdditive ? MAX_DEPTH_2 + 1 : MAX_DEPTH_2;
			Fractal->r_mean = 0.7;
			Fractal->dr_mean = 0.3;
			Fractal->dr2_mean = 0.4;
			break;

		case 3:
			Fractal->Depth = fAdditive ? MAX_DEPTH_3 + 1 : MAX_DEPTH_3;
			Fractal->r_mean = 0.6;
			Fractal->dr_mean = 0.4;
			Fractal->dr2_mean = 0.3;
			break;

		case 4:
			Fractal->Depth = MAX_DEPTH_4;
			Fractal->r_mean = 0.5;
			Fractal->dr_mean = 0.4;
			Fractal->dr2_mean = 0.3;
			break;

		case 5:
			Fractal->Depth = MAX_DEPTH_5;
			Fractal->r_mean = 0.5;
			Fractal->dr_mean = 0.4;
			Fractal->dr2_mean = 0.3;
			break;
	}

	Fractal->SimilitudeCount = i;
	Fractal->MaxPoint = Fractal->SimilitudeCount - 1;
	for (i = 0; i <= Fractal->Depth + 2; ++i)
		Fractal->MaxPoint *= Fractal->SimilitudeCount;

	if ((Fractal->buffer1 = (Point *)calloc(Fractal->MaxPoint,
			sizeof(Point))) == NULL) {
		_FreeIFS(Fractal);
		return;
	}
	if ((Fractal->buffer2 = (Point *)calloc(Fractal->MaxPoint,
			sizeof(Point))) == NULL) {
		_FreeIFS(Fractal);
		return;
	}
	Fractal->Speed = 6;
#if HALF
	Fractal->Width = bounds.IntegerWidth() / 2 + 1;
	Fractal->Height = bounds.IntegerHeight() / 2 + 1;
#else
	Fractal->Width = bounds.IntegerWidth() + 1;
	Fractal->Height = bounds.IntegerHeight() + 1;
#endif
	Fractal->CurrentPoint = 0;
	Fractal->Count = 0;
	Fractal->Lx = (Fractal->Width - 1) / 2;
	Fractal->Ly = (Fractal->Height - 1) / 2;
	Fractal->Col = NRAND(Fractal->Width * Fractal->Height - 1) + 1;

	_RandomSimilitudes(Fractal, Fractal->Components, 5 * MAX_SIMILITUDE);

	delete Fractal->bitmap;
	Fractal->bitmap = new BBitmap(BRect(0.0, 0.0,
		Fractal->Width - 1, Fractal->Height - 1), 0, B_RGB32);
	delete Fractal->markBitmap;
	Fractal->markBitmap = new BBitmap(BRect(0.0, 0.0,
		Fractal->Width - 1, Fractal->Height - 1), 0, B_GRAY8);

	// allocation checked
	if (Fractal->bitmap != NULL && Fractal->bitmap->IsValid())
		memset(Fractal->bitmap->Bits(), 0, Fractal->bitmap->BitsLength());
	else {
		delete Fractal->bitmap;
		Fractal->bitmap = NULL;
	}

	if (Fractal->markBitmap != NULL && Fractal->markBitmap->IsValid()) {
		memset(Fractal->markBitmap->Bits(), 0,
			Fractal->markBitmap->BitsLength());
	} else {
		delete Fractal->markBitmap;
		Fractal->markBitmap = NULL;
	}
}


IFS::~IFS()
{
	if (fRoot != NULL) {
		_FreeIFS(fRoot);
		free((void*)fRoot);
	}
}


void
IFS::Draw(BView* view, const buffer_info* info, int32 frames)
{
	int i;
	float u;
	float uu;
	float v;
	float vv;
	float u0;
	float u1;
	float u2;
	float u3;
	SIMILITUDE* S;
	SIMILITUDE* S1;
	SIMILITUDE* S2;
	SIMILITUDE* S3;
	SIMILITUDE* S4;
	FRACTAL* F;

	if (fRoot == NULL)
		return;

	F = fRoot;
	if (F->buffer1 == NULL)
		return;

	// do this as many times as necessary to calculate the missing frames
	// so the animation doesn't jerk when we miss a few frames
	for (int32 frame = 0; frame < frames; frame++) {
		u = (float) (F->Count) * (float) (F->Speed) / 1000.0;
		uu = u * u;
		v = 1.0 - u;
		vv = v * v;
		u0 = vv * v;
		u1 = 3.0 * vv * u;
		u2 = 3.0 * v * uu;
		u3 = u * uu;

		S = F->Components;
		S1 = S + F->SimilitudeCount;
		S2 = S1 + F->SimilitudeCount;
		S3 = S2 + F->SimilitudeCount;
		S4 = S3 + F->SimilitudeCount;

		for (i = F->SimilitudeCount; i; --i, S++, S1++, S2++, S3++, S4++) {
			S->c_x = u0 * S1->c_x + u1 * S2->c_x + u2 * S3->c_x + u3 * S4->c_x;
			S->c_y = u0 * S1->c_y + u1 * S2->c_y + u2 * S3->c_y + u3 * S4->c_y;
			S->r = u0 * S1->r + u1 * S2->r + u2 * S3->r + u3 * S4->r;
			S->r2 = u0 * S1->r2 + u1 * S2->r2 + u2 * S3->r2 + u3 * S4->r2;
			S->A = u0 * S1->A + u1 * S2->A + u2 * S3->A + u3 * S4->A;
			S->A2 = u0 * S1->A2 + u1 * S2->A2 + u2 * S3->A2 + u3 * S4->A2;
		}

		if (frame == frames - 1)
			_DrawFractal(view, info);

		if (F->Count >= 1000 / F->Speed) {
			S = F->Components;
			S1 = S + F->SimilitudeCount;
			S2 = S1 + F->SimilitudeCount;
			S3 = S2 + F->SimilitudeCount;
			S4 = S3 + F->SimilitudeCount;
	
			for (i = F->SimilitudeCount; i; --i, S++, S1++, S2++, S3++, S4++) {
				S2->c_x = 2.0 * S4->c_x - S3->c_x;
				S2->c_y = 2.0 * S4->c_y - S3->c_y;
				S2->r = 2.0 * S4->r - S3->r;
				S2->r2 = 2.0 * S4->r2 - S3->r2;
				S2->A = 2.0 * S4->A - S3->A;
				S2->A2 = 2.0 * S4->A2 - S3->A2;

				*S1 = *S4;
			}
			_RandomSimilitudes(F, F->Components + 3 * F->SimilitudeCount,
				F->SimilitudeCount);
			_RandomSimilitudes(F, F->Components + 4 * F->SimilitudeCount,
				F->SimilitudeCount);

			F->Count = 0;
		} else
			F->Count++;
	}
}


void
IFS::SetAdditive(bool additive)
{
	fAdditive = additive;
}


void
IFS::SetSpeed(int32 speed)
{
	if (fRoot && speed > 0 && speed <= 12)
		fRoot->Speed = speed;
}


void
IFS::_DrawFractal(BView* view, const buffer_info* info)
{
	FRACTAL* F = fRoot;
	int i;
	int j;
	int32 x;
	int32 y;
	int32 xo;
	int32 yo;
	SIMILITUDE* Current;
	SIMILITUDE* Similitude;

	for (Current = F->Components, i = F->SimilitudeCount; i; --i, Current++) {
		Current->Cx = FLOAT_TO_INT(Current->c_x);
		Current->Cy = FLOAT_TO_INT(Current->c_y);

		Current->Ct = FLOAT_TO_INT(cos(Current->A));
		Current->St = FLOAT_TO_INT(sin(Current->A));
		Current->Ct2 = FLOAT_TO_INT(cos(Current->A2));
		Current->St2 = FLOAT_TO_INT(sin(Current->A2));

		Current->R = FLOAT_TO_INT(Current->r);
		Current->R2 = FLOAT_TO_INT(Current->r2);
	}

	fCurrentPoint = 0;
	fCurrentFractal = F;
	fPointBuffer = F->buffer2;
	for (Current = F->Components, i = F->SimilitudeCount; i; --i, Current++) {
		xo = Current->Cx;
		yo = Current->Cy;
		for (Similitude = F->Components, j = F->SimilitudeCount; j;
				--j, Similitude++) {
			if (Similitude == Current)
				continue;

			transform(Similitude, xo, yo, &x, &y);
			_Trace(F, x, y);
		}
	}

	if (F->bitmap != NULL && F->markBitmap != NULL) {
		uint8* bits = (uint8*)F->bitmap->Bits();
		uint32 bpr = F->bitmap->BytesPerRow();
		uint8* markBits = (uint8*)F->markBitmap->Bits();
		uint32 markBPR = F->markBitmap->BytesPerRow();
		int32 minX = F->Width;
		int32 minY = F->Height;
		int32 maxX = 0;
		int32 maxY = 0;

		// Erase previous dots from bitmap,
		// but only if we're not in BDirectWindow mode,
		// since the dots will have been erased already
		if (info == NULL) {
			if (F->CurrentPoint) {
				for (int32 i = 0; i <  F->CurrentPoint; i++) {
					Point p = F->buffer1[i];
					if (p.x >= 0 && p.x < F->Width
						&& p.y >= 0 && p.y < F->Height) {
						int32 offset = bpr * p.y + p.x * 4;
						*(uint32*)&bits[offset] = 0;
						if (minX > p.x)
							minX = p.x;

						if (minY > p.y)
							minY = p.y;

						if (maxX < p.x)
							maxX = p.x;

						if (maxY < p.y)
							maxY = p.y;
					}
				}
			}
		}

		// draw the new dots into the bitmap
		if (fCurrentPoint != 0) {
			if (info != NULL) {
				for (int32 i = 0; i <  fCurrentPoint; i++) {
					Point p = F->buffer2[i];
					if (p.x >= 0 && p.x < F->Width
						&& p.y >= 0 && p.y < F->Height) {
						int32 offset = bpr * p.y + p.x * 4;
						if (fAdditive) {
							if (bits[offset + 0] < 255) {
								bits[offset + 0] += 51;
								bits[offset + 1] += 51;
								bits[offset + 2] += 51;
							}
						} else
							*(uint32*)&bits[offset] = 0xffffffff;
					}
				}
			} else {
				// in this version, remember the bounds rectangle
				for (int32 i = 0; i < fCurrentPoint; i++) {
					Point p = F->buffer2[i];
					if (p.x >= 0 && p.x < F->Width
						&& p.y >= 0 && p.y < F->Height) {
						int32 offset = bpr * p.y + p.x * 4;
						if (fAdditive) {
							if (bits[offset + 0] < 255) {
								bits[offset + 0] += 15;
								bits[offset + 1] += 15;
								bits[offset + 2] += 15;
							}
						} else
							*(uint32*)&bits[offset] = 0xffffffff;

						if (minX > p.x)
							minX = p.x;

						if (minY > p.y)
							minY = p.y;

						if (maxX < p.x)
							maxX = p.x;

						if (maxY < p.y)
							maxY = p.y;
					}
				}
			}
		}

		if (info != NULL && info->bits != NULL) {
			uint8* screenBits = (uint8*)info->bits;
			uint32 screenBPR = info->bytesPerRow;
			int32 left = info->bounds.left;
			int32 top = info->bounds.top;
			int32 bpp = info->bits_per_pixel;
			screenBits += left * bpp + top * bpr;

			int32 screenWidth = info->bounds.right - left;
			int32 screenHeight = info->bounds.bottom - top;

			// redraw the previous points on screen
			// with the contents of the current bitmap
			//
			// draw the new points, erasing the bitmap as we go
			int32 maxPoints = max_c(F->CurrentPoint, fCurrentPoint);
			if (maxPoints > 0) {
				for (int32 i = 0; i < maxPoints; i++) {
					// copy previous points (black)
					if (i < F->CurrentPoint) {
						Point p = F->buffer1[i];
						if (p.x >= 0 && p.x < F->Width && p.x < screenWidth
							&& p.y >= 0 && p.y < F->Height
							&& p.y < screenHeight) {
							int32 markOffset = markBPR * p.y + p.x;
							if (markBits[markOffset] != fCurrentMarkValue) {
								int32 offset = bpr * p.y + p.x * 4;
								// copy the pixel to the screen
								uint32* src = (uint32*)&bits[offset];
								if (bpp == 32) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 4;
									*(uint32*)&screenBits[screenOffset] = *src;
								} else if (bpp == 16) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 2;
									*(uint16*)&screenBits[screenOffset] =
										(uint16)(((bits[offset + 2] & 0xf8)
											<< 8)
										| ((bits[offset + 1] & 0xfc) << 3)
										| (bits[offset] >> 3));
								} else if (bpp == 15) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 2;
									*(uint16*)&screenBits[screenOffset] =
										(uint16)(((bits[offset + 2] & 0xf8)
											<< 7)
										| ((bits[offset + 1] & 0xf8) << 2)
										| (bits[offset] >> 3));
								} else if (bpp == 8) {
									int32 screenOffset = screenBPR * p.y + p.x;
									screenBits[screenOffset] = bits[offset];
								}
								*src = 0;
								markBits[markOffset] = fCurrentMarkValue;
							}
							// else it means the pixel has been copied already
						}
					}

					// copy current points (white) and erase them from the
					// bitmap
					if (i < fCurrentPoint) {
						Point p = F->buffer2[i];
						if (p.x >= 0 && p.x < F->Width && p.x < screenWidth
							&& p.y >= 0 && p.y < F->Height
							&& p.y < screenHeight) {
							int32 markOffset = markBPR * p.y + p.x;
							int32 offset = bpr * p.y + p.x * 4;

							// copy the pixel to the screen
							uint32* src = (uint32*)&bits[offset];
							if (markBits[markOffset] != fCurrentMarkValue) {
								if (bpp == 32) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 4;
									*(uint32*)&screenBits[screenOffset] = *src;
								} else if (bpp == 16) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 2;
									*(uint16*)&screenBits[screenOffset] =
										(uint16)(((bits[offset + 2] & 0xf8)
											<< 8)
										| ((bits[offset + 1] & 0xfc) << 3)
										| (bits[offset] >> 3));
								} else if (bpp == 15) {
									int32 screenOffset = screenBPR * p.y
										+ p.x * 2;
									*(uint16*)&screenBits[screenOffset] =
										(uint16)(((bits[offset + 2] & 0xf8)
											<< 7)
										| ((bits[offset + 1] & 0xf8) << 2)
										| (bits[offset] >> 3));
								} else if (bpp == 1) {
									int32 screenOffset = screenBPR * p.y + p.x;
									screenBits[screenOffset] = bits[offset];
								}
								markBits[markOffset] = fCurrentMarkValue;
							}
							// else it means the pixel has been copied already
							*src = 0;
						}
					}
				}
			}
		} else {
			// if not in BDirectWindow mode, draw the bitmap
			BRect b(minX, minY, maxX, maxY);
			view->DrawBitmapAsync(F->bitmap, b, b);
		}
	}

	// flip buffers
	F->CurrentPoint = fCurrentPoint;
	fPointBuffer = F->buffer1;
	F->buffer1 = F->buffer2;
	F->buffer2 = fPointBuffer;

	if (fCurrentMarkValue == 255)
		fCurrentMarkValue = 0;
	else
		fCurrentMarkValue++;
}


void
IFS::_Trace(FRACTAL* F, int32 xo, int32 yo)
{
	int32 x;
	int32 y;
	SIMILITUDE* Current;

	Current = fCurrentFractal->Components;
	for (int32 i = fCurrentFractal->SimilitudeCount; i; --i, Current++) {
		transform(Current, xo, yo, &x, &y);
		fPointBuffer->x = (UNIT * 2 + x) * F->Lx / (UNIT * 2);
		fPointBuffer->y = (UNIT * 2 - y) * F->Ly / (UNIT * 2);
		fPointBuffer++;
		fCurrentPoint++;

		if (F->Depth && ((x - xo) >> 4) && ((y - yo) >> 4)) {
			F->Depth--;
			_Trace(F, x, y);
			F->Depth++;
		}
	}
}


void
IFS::_RandomSimilitudes(FRACTAL* fractal, SIMILITUDE* current, int i) const
{
	while (i-- > 0) {
		current->c_x = gauss_rand(0.0, .8, 4.0);
		current->c_y = gauss_rand(0.0, .8, 4.0);
		current->r   = gauss_rand(fractal->r_mean, fractal->dr_mean, 3.0);
		current->r2  = half_gauss_rand(0.0,fractal->dr2_mean, 2.0);
		current->A   = gauss_rand(0.0, 360.0, 4.0) * (M_PI / 180.0);
		current->A2  = gauss_rand(0.0, 360.0, 4.0) * (M_PI / 180.0);
		current++;
	}
}


void
IFS::_FreeBuffers(FRACTAL* f)
{
	if (f->buffer1) {
		free((void*)f->buffer1);
		f->buffer1 = (Point*)NULL;
	}

	if (f->buffer2) {
		free((void*)f->buffer2);
		f->buffer2 = (Point*)NULL;
	}
}


void
IFS::_FreeIFS(FRACTAL* f)
{
	_FreeBuffers(f);
	delete f->bitmap;
	f->bitmap = NULL;
	delete f->markBitmap;
	f->markBitmap = NULL;
}
