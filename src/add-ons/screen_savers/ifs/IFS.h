// Polygon.h

#ifndef IFS_H
#define IFS_H

#include <Screen.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>

typedef struct Similitude_Struct	SIMI;
typedef struct Fractal_Struct		FRACTAL;

#define FIX 12
#define UNIT   ( 1<<FIX )
#define MAX_SIMI  6

// settings for a PC 120Mhz...
#define MAX_DEPTH_2  10
#define MAX_DEPTH_3  6
#define MAX_DEPTH_4  4
#define MAX_DEPTH_5  3

struct buffer_info {
	void*			bits;
	uint32			bytesPerRow;
	uint32			bits_per_pixel;
	color_space		format;
	clipping_rect	bounds;
};

struct Point {
	int32		x;
	int32		y;
};

struct Similitude_Struct {

	float		c_x, c_y;
	float		r, r2, A, A2;
	int32		Ct, St, Ct2, St2;
	int32		Cx, Cy;
	int32		R, R2;
};

struct Fractal_Struct {

	int			Nb_Simi;
	SIMI		Components[5 * MAX_SIMI];
	int			Depth, Col;
	int			Count, Speed;
	int			Width, Height, Lx, Ly;
	float		r_mean, dr_mean, dr2_mean;
	int			Cur_Pt, Max_Pt;
	Point*		buffer1;
	Point*		buffer2;
	BBitmap*	bitmap;
	BBitmap*	markBitmap;
};

class IFS {
 public:
								IFS(BRect bounds);
	virtual						~IFS();

			void				Draw(BView* view, const buffer_info* info,
									 int32 frames);

			void				SetAdditive(bool additive);
			void				SetSpeed(int32 speed);

 private:
			void				_DrawFractal(BView* view,
											 const buffer_info* info);
			void				_Trace(FRACTAL* F,
									   int32 xo, int32 yo);
			void				_RandomSimis(FRACTAL* f,
											 SIMI* cur,
											 int i) const;
			void				_FreeBuffers(FRACTAL* f);
			void				_FreeIFS(FRACTAL* f);


			FRACTAL*			fRoot;
			FRACTAL*			fCurrentFractal;
			Point*				fPointBuffer;
			int32				fCurrentPoint;

			bool				fAdditive;
			uint8				fCurrentMarkValue;
};

#endif // ABOUT_VPOLYGON_HIEW_H
