#include "Layer.h"

Layer::Layer(ServerWindow *Win, const char *Name, BRect &Frame, int32 Flags)
{
}

Layer::Layer(ServerBitmap *Bitmap)
{
}

Layer::~Layer(void)
{
}

void Layer::Initialize(void)
{
}

void Layer::Show(void)
{
}

void Layer::Hide(void)
{
}

bool Layer::IsVisible(void)
{
	return false;
}

void Layer::Update(bool ForceUpdate)
{
}

bool Layer::IsBackground(void)
{
	return false;
}

void Layer::SetFrame(const BRect &Rect)
{
}

BRect Layer::Frame(void)
{
	return BRect(0,0,0,0);
}

BRect Layer::Bounds(void)
{
	return BRect(0,0,0,0);
}

void Layer::Invalidate(const BRect &Rect)
{
}

void Layer::AddRegion(BRegion *Region)
{
}

void Layer::RemoveRegions(void)
{
}

void Layer::UpdateRegions(bool ForceUpdate=true, bool Root=true)
{
}

void Layer::Draw(const BRect &Rect)
{
}

