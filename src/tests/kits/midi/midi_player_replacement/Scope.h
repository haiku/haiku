/*
Author:	Michael Pfeiffer
Email:	michael.pfeiffer@utanet.at
*/
#ifndef SCOPE_H
#define SCOPE_H

#include <View.h>

class Scope : public BView {
	BBitmap *mBitmap;
	BView *mOffscreenView;
	int mWidth, mHeight;
	int32 mSampleCount;
	int16 *mLeft, *mRight;
public:
	Scope(BRect rect);
	
	void Draw(BRect updateRect);
	void DrawSample();
	
	void Pulse();
	void AttachedToWindow(void);
	void DetachedFromWindow(void);
};

#endif
