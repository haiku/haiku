/*
 * HalftoneEngine.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __HALFTONE_H
#define __HALFTONE_H

#include <GraphicsDefs.h>

struct CACHE_FOR_CMAP8 {
	uint density;
	bool hit;
};

class Halftone;
typedef int (Halftone::*PFN_dither)(uchar *dst, const uchar *src, int x, int y, int width);
typedef uint (*PFN_gray)(rgb_color c);

class Halftone {
public:
	enum DITHERTYPE {
		TYPE1,
		TYPE2,
		TYPE3
	};
	Halftone(color_space cs, double gamma = 1.4, DITHERTYPE dither_type = TYPE3);
	~Halftone();
	int dither(uchar *dst, const uchar *src, int x, int y, int width);
	int getPixelDepth() const;
	const rgb_color *getPalette() const;
	const uchar *getPattern() const;
	void setPattern(const uchar *pattern);
	PFN_gray getGrayFunction() const;
	void setGrayFunction(PFN_gray gray);

protected:
	void createGammaTable(double gamma);
	void initElements(int x, int y, uchar *elements);
	int ditherGRAY1(uchar *dst, const uchar *src, int x, int y, int width);
	int ditherGRAY8(uchar *dst, const uchar *src, int x, int y, int width);
	int ditherCMAP8(uchar *dst, const uchar *src, int x, int y, int width);
	int ditherRGB32(uchar *dst, const uchar *src, int x, int y, int width);

	Halftone(const Halftone &);
	Halftone &operator = (const Halftone &);

private:
	PFN_dither		__dither;
	PFN_gray        __gray;
	int             __pixel_depth;
	const rgb_color *__palette;
	const uchar     *__pattern;
	uint            __gamma_table[256];
	CACHE_FOR_CMAP8 __cache_table[256];
};

inline int Halftone::getPixelDepth() const
{
	return __pixel_depth;
}

inline const rgb_color *Halftone::getPalette() const
{
	return __palette;
}

inline const uchar * Halftone::getPattern() const
{
	return __pattern;
}

inline void Halftone::setPattern(const uchar *pattern)
{
	__pattern = pattern;
}

inline PFN_gray Halftone::getGrayFunction() const
{
	return __gray;
}
inline void Halftone::setGrayFunction(PFN_gray gray)
{
	__gray = gray;
}

#endif	/* __HALFTONE_H */
