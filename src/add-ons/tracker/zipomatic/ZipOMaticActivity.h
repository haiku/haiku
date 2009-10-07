#ifndef _ACTIVITY_H_
#define _ACTIVITY_H_

#include <stdlib.h>

#include <Box.h>
#include <Bitmap.h>
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
				
private:
			void			_CreateBitmap();
			void			_LightenBitmapHighColor(rgb_color* color);
			void			_DrawOnBitmap(bool running);

			bool			fIsRunning;
			pattern			fPattern;
			BBitmap*		fBitmap;
			BView*			fBitmapView;
};

#endif	// _ACTIVITY_H_

