#include <OS.h>
#include <View.h>
#include <Region.h>

class Layer;

class MyView: public BView
{
public:
						MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
	virtual				~MyView();

	virtual	void		Draw(BRect area);

			void		CopyRegion(BRegion *reg, float dx, float dy);
			void		RequestRedraw();			

			Layer*		FindLayer(Layer *lay, const char *bytes) const;

			Layer		*topLayer;
			BRegion		fRedrawReg;
private:
			void		DrawSubTree(Layer* lay);
};