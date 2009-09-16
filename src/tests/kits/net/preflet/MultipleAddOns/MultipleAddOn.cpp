#include <stdlib.h>
#include <stdio.h>

#include "NetworkSetupAddOn.h"

class MultipleAddOn : public NetworkSetupAddOn {
	public:
		MultipleAddOn(image_id addon_image, int index);
		~MultipleAddOn();
		BView*			CreateView(BRect* bounds);
		const char* 	Name();
		
	private:
		char*	fName;
		rgb_color fViewColor;
};


MultipleAddOn::MultipleAddOn(image_id image, int i)
	:
	NetworkSetupAddOn(image)
{
	fName = (char*) malloc(64);
	sprintf(fName, "Multi #%d", i);
	
	fViewColor.red = rand() % 256;
	fViewColor.blue = rand() % 256;
	fViewColor.green = rand() % 256;
	fViewColor.alpha = 255;
}


MultipleAddOn::~MultipleAddOn()
{
	free(fName);
}


BView*
MultipleAddOn::CreateView(BRect* bounds)
{
	BView *v = new BView(*bounds, "a view", B_FOLLOW_ALL, B_WILL_DRAW);
	v->SetViewColor(fViewColor);
	return v;
}


const char*
MultipleAddOn::Name()
{
	return fName;
}


NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index < 0 || index > 3)
		return NULL;

	return new MultipleAddOn(image, index);
}
