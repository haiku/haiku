#ifndef _ROOTLAYER_H_
#define _ROOTLAYER_H_

#include "Layer.h"

class DisplayDriver;
class RGBColor;

class RootLayer : public Layer
{
public:
	RootLayer(BRect frame, const char *name);
	~RootLayer(void);
	void RequestDraw(void);
	void MoveBy(float x, float y);
	void MoveBy(BPoint pt);
	void SetDriver(DisplayDriver *driver);
	void SetColor(const RGBColor &col);
	RGBColor GetColor(void) const;
	void RebuildRegions(bool recursive=false);
private:
	DisplayDriver *_driver;
	RGBColor *_bgcolor;
	bool _visible;
};

#endif
