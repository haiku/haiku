#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/dirent.h>
#include <sys/stat.h>

#include <String.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <Box.h>
#include <Button.h> 
#include <Bitmap.h>
#include <Alert.h>

#include <NetworkSetupAddOn.h>

#include "DialUpView.h"

class AddOn : public NetworkSetupAddOn
{
public:
		AddOn(image_id addon_image);
		~AddOn();
		
		const char * 	Name();
		BView * 		CreateView(BRect *bounds);
};


NetworkSetupAddOn * get_nth_addon(image_id image, int index)
{
	if (index == 0)
		return new AddOn(image);
	return NULL;
}

// #pragma mark -

AddOn::AddOn(image_id image)
	: 	NetworkSetupAddOn(image)
{
}


AddOn::~AddOn()
{
}


const char * AddOn::Name()
{
	return "DialUp";
}


BView * AddOn::CreateView(BRect *bounds)
{
	BRect r = *bounds;
	
	if (r.Width() < 200 || r.Height() < 400)
		r.Set(0, 0, 200, 400);

	BView *view = new DialUpView(r);
	*bounds = view->Bounds();

	return view;
}

