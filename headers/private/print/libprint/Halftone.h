/*
 * HalftoneEngine.h
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
typedef int (Halftone::*PFN_dither)(uchar *dst, const uchar *src, int x, int y, int width);
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
	Halftone(color_space cs, double gamma = 1.4, double min = 0.0, DitherType dither_type = kTypeFloydSteinberg);
	~Halftone();
	void setPlanes(Planes planes);
	void setBlackValue(BlackValue blackValue);
	int dither(uchar *dst, const uchar *src, int x, int y, int width);
	int getPixelDepth() const;
	const uchar *getPattern() const;
	void setPattern(const uchar *pattern);

protected:
	// PFN_gray: return value of 0 means low density (or black) and value of 255 means high density (or white)
	PFN_gray getGrayFunction() const;
	void setGrayFunction(PFN_gray gray);
	void setGrayFunction(GrayFunction grayFunction);

	void createGammaTable(double gamma, double min);
	void initElements(int x, int y, uchar *elements);
	uint getDensity(ColorRGB32 c) const;
	uchar convertUsingBlackValue(uchar byte) const;
	int ditherRGB32(uchar *dst, const uchar *src, int x, int y, int width);

	void initFloydSteinberg();
	void deleteErrorTables();
	void uninitFloydSteinberg();
	void setupErrorBuffer(int x, int y, int width);
	int ditherFloydSteinberg(uchar *dst, const uchar* src, int x, int y, int width);

	Halftone(const Halftone &);
	Halftone &operator = (const Halftone &);

private:
	enum {
		kGammaTableSize = 256,
		kMaxNumberOfPlanes = 3
	};
	PFN_dither		fDither;
	PFN_gray        fGray;
	int             fPixelDepth;
	Planes          fPlanes;
	BlackValue      fBlackValue;
	const uchar     *fPattern;
	uint            fGammaTable[kGammaTableSize];
	int             fNumberOfPlanes;
	int             fCurrentPlane;
	// fields used for floyd-steinberg dithering
	int             fX;
	int             fY;
	int             fWidth;
	int             *fErrorTables[kMaxNumberOfPlanes];
};

inline int Halftone::getPixelDepth() const
{
	return fPixelDepth;
}

inline const uchar * Halftone::getPattern() const
{
	return fPattern;
}

inline void Halftone::setPattern(const uchar *pattern)
{
	fPattern = pattern;
}

inline PFN_gray Halftone::getGrayFunction() const
{
	return fGray;
}
inline void Halftone::setGrayFunction(PFN_gray gray)
{
	fGray = gray;
}

inline uint Halftone::getDensity(ColorRGB32 c) const
{
	return fGammaTable[fGray(c)];
}

inline uchar Halftone::convertUsingBlackValue(uchar byte) const
{
	// bits with value = '1' in byte mean black
	if (fBlackValue == kHighValueMeansBlack) {
		return byte;
	} else {
		return ~byte;
	}
}

#endif	/* __HALFTONE_H */
