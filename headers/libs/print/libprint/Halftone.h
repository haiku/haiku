/*
 * Halftone.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __HALFTONE_H
#define __HALFTONE_H

#include <GraphicsDefs.h>

// definition for B_RGB32 (=B_RGB32_LITTLE) and B_RGBA32
typedef struct {
	uchar blue;
	uchar green;
	uchar red;
	uchar alpha; // unused in B_RGB32
} ColorRGB32Little;

// definition for B_RGB32_BIG and B_RGBA32_BIG
typedef struct {
	uchar alpha; // unused in B_RGB32_BIG
	uchar red;
	uchar green;
	uchar blue;
} ColorRGB32Big;

typedef union {
	ColorRGB32Little little;
	ColorRGB32Big    big;
} ColorRGB32;

class Halftone;

typedef void (Halftone::*PFN_dither)(uchar* destination, const uchar* source,
	int x, int y, int width);

typedef uint (*PFN_gray)(ColorRGB32 c);

class Halftone {
public:
	enum DitherType {
		kType1,
		kType2,
		kType3,
		kTypeFloydSteinberg,
	};

	enum GrayFunction {
		kMixToGray,
		kRedChannel,
		kGreenChannel,
		kBlueChannel
	};

	enum Planes {
		kPlaneMonochrome1, // 1 bit depth (0 white, 1 black)
		kPlaneRGB1,        // 3 planes, 1 bit depth (0 black, 7 white)
	};

	enum BlackValue {
		kHighValueMeansBlack,
		kLowValueMeansBlack,
	};

					Halftone(color_space colorSpace, double gamma = 1.4,
						double min = 0.0,
						DitherType dither_type = kTypeFloydSteinberg);
					~Halftone();

			void	SetPlanes(Planes planes);
			void	SetBlackValue(BlackValue blackValue);

			void	Dither(uchar *destination, const uchar *source, int x,
						int y, int width);

			int		GetPixelDepth() const;

			const uchar*	GetPattern() const;
			void			SetPattern(const uchar *pattern);

protected:
			Halftone(const Halftone &);

			Halftone&	operator=(const Halftone &);

			// PFN_gray: return value of 0 means low density (or black) and
			// value of 255 means high density (or white)
			PFN_gray	GetGrayFunction() const;
			void		SetGrayFunction(PFN_gray gray);
			void		SetGrayFunction(GrayFunction grayFunction);

			void		CreateGammaTable(double gamma, double min);
			void		InitElements(int x, int y, uchar* elements);
			uint		GetDensity(ColorRGB32 c) const;
			uchar		ConvertUsingBlackValue(uchar byte) const;
			void		DitherRGB32(uchar* destination, const uchar* source,
							int x, int y, int width);

			void		InitFloydSteinberg();
			void		DeleteErrorTables();
			void		UninitFloydSteinberg();
			void		SetupErrorBuffer(int x, int y, int width);
			void		DitherFloydSteinberg(uchar* destination,
							const uchar* source, int x, int y, int width);
						// Do nothing method to get around a bug in
						// the gcc2 cross-compiler built on Mac OS X
						// Lion where a compiler error occurs when
						// assigning a member function pointer to NULL
						// or 0: cast specifies signature type.
						// However, this should be legal according to
						// the C++03 standard.
			void		DitherNone(uchar*, const uchar*, int, int, int) {};

private:
	enum {
		kGammaTableSize = 256,
		kMaxNumberOfPlanes = 3
	};

	PFN_dither		fDither;
	PFN_gray		fGray;
	int				fPixelDepth;
	Planes			fPlanes;
	BlackValue		fBlackValue;
	const uchar*	fPattern;
	uint			fGammaTable[kGammaTableSize];
	int				fNumberOfPlanes;
	int				fCurrentPlane;
	// fields used for floyd-steinberg dithering
	int				fX;
	int				fY;
	int				fWidth;
	int*			fErrorTables[kMaxNumberOfPlanes];
};


inline int
Halftone::GetPixelDepth() const
{
	return fPixelDepth;
}


inline const uchar*
Halftone::GetPattern() const
{
	return fPattern;
}


inline void
Halftone::SetPattern(const uchar* pattern)
{
	fPattern = pattern;
}


inline PFN_gray
Halftone::GetGrayFunction() const
{
	return fGray;
}


inline void
Halftone::SetGrayFunction(PFN_gray gray)
{
	fGray = gray;
}


inline uint
Halftone::GetDensity(ColorRGB32 c) const
{
	return fGammaTable[fGray(c)];
}


inline uchar
Halftone::ConvertUsingBlackValue(uchar byte) const
{
	// bits with value = '1' in byte mean black
	if (fBlackValue == kHighValueMeansBlack)
		return byte;

	return ~byte;
}


#endif	/* __HALFTONE_H */
