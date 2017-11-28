#ifndef _ACTIVITY_H_
#define _ACTIVITY_H_

#include <stdlib.h>

#include <Box.h>
#include <Bitmap.h>
#include <Polygon.h>
#include <View.h>
#include <Window.h>


class Activity : public BView 
{
public:
							Activity(const char* name);
							~Activity();

			void			Start();
			void			Pause();
			void			Stop();
			bool			IsRunning();
	virtual	void			AllAttached();
	virtual	void			Pulse();
	virtual	void			Draw(BRect draw);
	virtual	void			FrameResized(float width, float height);
			void			SetColors(const rgb_color* colors,
								uint32 numColors);
				
private:
			void			_CreateBitmap();
			void			_DrawOnBitmap(bool running);
			void			_ActiveColors();
			void			_InactiveColors();

			bool			fIsRunning;
			pattern			fPattern;
			BBitmap*		fBitmap;
			BView*			fBitmapView;

			float				fSpinSpeed;
			const rgb_color*	fColors;
			uint32				fNumColors;

			float				fScrollOffset;
			BPolygon			fStripe;
			float				fStripeWidth;
			uint32				fNumStripes;
};

#endif	// _ACTIVITY_H_

