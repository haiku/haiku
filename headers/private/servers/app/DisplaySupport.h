#ifndef DDRIVER_SUPPORT_H_
#define DDRIVER_SUPPORT_H_

#include <Rect.h>
#include <List.h>
#include "ServerBitmap.h"

class BezierCurve
{
public:
	BezierCurve(BPoint* pts);
	~BezierCurve();
	BPoint* GetPointArray();
	BList points;
	BRect Frame(void) const { return fFrame; }
private:
	void GenerateFrame(BPoint *pts);
	int GeneratePoints(int startPos);
	BPoint* pointArray;
	BRect fFrame;
};

struct integer_rect
{
	int32 x;
	int32 y;
	int32 w;
	int32 h;
};

struct integer_scaling_info
{
	int32 k;
};

static inline void BRect_to_integer_rect(BRect source_rect, integer_rect &dest_rect)
{
	dest_rect.x = (int32)source_rect.left;
	dest_rect.y = (int32)source_rect.top;
	dest_rect.w = (int32)(source_rect.right - source_rect.left) + 1;
	dest_rect.h = (int32)(source_rect.bottom - source_rect.top) + 1;
}

static inline integer_rect BRect_to_integer_rect(BRect source_rect)
{
	integer_rect dest_rect;
	dest_rect.x = (int32)source_rect.left;
	dest_rect.y = (int32)source_rect.top;
	dest_rect.w = (int32)(source_rect.right - source_rect.left) + 1;
	dest_rect.h = (int32)(source_rect.bottom - source_rect.top) + 1;
	return dest_rect;
}

class Blitter
{
public:
	Blitter();
	void draw_8_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_8_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_8_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	
	void draw_16_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_16_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_16_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	
	void draw_32_to_8(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_32_to_16(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	void draw_32_to_32(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	
	void draw_none(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor) {}
	void Draw(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	
	void Select(int32 source_bits_per_pixel, int32 dest_bits_per_pixel);
	
	drawing_mode GetDrawMode() const { return mode; }
	void SetDrawMode(drawing_mode draw_mode) { mode = draw_mode; }
	
private:
	void (Blitter::*DrawFunc)(uint8 *src, uint8 *dst, int32 width, int32 xscale_position, int32 xscale_factor);
	drawing_mode mode;
};

class LineCalc
{
public:
	LineCalc();
	LineCalc(const BPoint &pta, const BPoint &ptb);
	void SetPoints(const BPoint &pta, const BPoint &ptb);
	float GetX(float y);
	float GetY(float x);
	float Slope(void) { return slope; }
	float Offset(void) { return offset; }
	float MinX() { return minx; }
	float MinY() { return miny; }
	float MaxX() { return maxx; }
	float MaxY() { return maxy; }
	void Swap(LineCalc &from);
	bool ClipToRect(const BRect &rect);
	BPoint GetStart() { return start; }
	BPoint GetEnd() { return end; }
private:
	float slope;
	float offset;
	BPoint start, end;
	float minx;
	float miny;
	float maxx;
	float maxy;
};

/*!
	\class FBBitmap DisplayDriver.h
	\brief Class used for easily passing around information about the framebuffer
*/
class FBBitmap : public ServerBitmap
{
public:
	FBBitmap(void) : ServerBitmap(BRect(0,0,0,0),B_NO_COLOR_SPACE,0) { }
	~FBBitmap(void) { }
	void SetBytesPerRow(const int32 &bpr) { _bytesperrow=bpr; }
	void SetSpace(const color_space &space) { _space=space; }
	
	// WARNING: - for some reason ServerBitmap adds 1 to the width and height. We do that also.
	void SetSize(const int32 &w, const int32 &h) { _width=w+1; _height=h+1; }
	void SetBuffer(void *ptr) { _buffer=(uint8*)ptr; }
	void SetBitsPerPixel(color_space space,int32 bytesperline) { _HandleSpace(space,bytesperline); }
	void ShallowCopy(const FBBitmap *from)
	{
		ServerBitmap::ShallowCopy((ServerBitmap*)from);
		SetBuffer(from->Bits());
	}
};

#endif
