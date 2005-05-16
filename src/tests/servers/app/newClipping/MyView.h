#include <OS.h>
#include <View.h>

class Layer;

class MyView: public BView
{
public:
						MyView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
	virtual				~MyView();

	virtual	void		Draw(BRect area);			

			Layer*		FindLayer(Layer *lay, const char *bytes) const;

			Layer		*topLayer;
private:
			void		DrawSubTree(Layer* lay);
};