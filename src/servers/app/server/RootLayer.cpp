#include <View.h> 
#include "RootLayer.h"
#include "Desktop.h"
#include "DisplayDriver.h"

RootLayer::RootLayer(BRect rect, const char *layername)
	: Layer(rect,layername,B_FOLLOW_NONE,0, NULL)
{
	_driver=GetGfxDriver();
}

RootLayer::~RootLayer(void)
{
}

void RootLayer::RequestDraw(void)
{
	if(!_invalid)
		return;
	
	// Redraw the base
	for(int32 i=0; _invalid->CountRects();i++)
	{
		if(_invalid->RectAt(i).IsValid())
			_driver->FillRect(_invalid->RectAt(i),_layerdata, 0LL);
		else
			break;
	}

	delete _invalid;
	_invalid=NULL;
	_is_dirty=false;

	// force redraw of all dirty windows	
	for(Layer *lay=_topchild; lay!=NULL; lay=lay->_lowersibling)
	{
		if(lay->IsDirty())
			lay->RequestDraw(lay->Bounds());
	}

}

void RootLayer::SetColor(const RGBColor &col)
{
	_layerdata->lowcolor=col;
}

RGBColor RootLayer::GetColor(void) const
{	
	return _layerdata->lowcolor;
}

void RootLayer::MoveBy(float x, float y)
{
}

void RootLayer::MoveBy(BPoint pt)
{
}

void RootLayer::RebuildRegions(bool recursive)
{
}