#include "PreviewView.h"
#include "Constants.h"
#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <iostream>
#include <ScreenSaver.h>
#include <Errors.h>

inline BPoint scaleDirect(float x, float y,BRect area)
{
	return BPoint(area.Width()*x+area.left,area.Height()*y+area.top);
}

inline BRect scaleDirect (float x1,float x2,float y1,float y2,BRect area)
{
	return BRect(area.Width()*x1+area.left,area.Height()*y1+area.top, area.Width()*x2+area.left,area.Height()*y2+area.top);
}

//float positionalX[]= {0,.1,.25,.3,.7,.75,.9,1.0};
//float positionalY[]= {0,.1,.7,.8,.9,1.0};

//inline BPoint scale(int x, int y,BRect area) { return scaleDirect(positionalX[x],positionalY[y],area); }
//inline BRect scale(int x1, int x2, int y1, int y2,BRect area) { return scaleDirect(positionalX[x1],positionalX[x2],positionalY[y1],positionalY[y2],area); }

float sampleX[]= {0,.025,.25,.6,.625,.7,.725,.75,.975,1.0};
float sampleY[]= {0,.05,.8,.9,.933,.966,1.0};
inline BPoint scale2(int x, int y,BRect area) { return scaleDirect(sampleX[x],sampleY[y],area); }
inline BRect scale2(int x1, int x2, int y1, int y2,BRect area) { return scaleDirect(sampleX[x1],sampleX[x2],sampleY[y1],sampleY[y2],area); }

PreviewView::PreviewView(BRect frame, const char *name)
: BView (frame,name,B_FOLLOW_NONE,B_WILL_DRAW),
  addonImage (0),
  saver (0),
  settingsBoxPtr (0) 
{
   SetViewColor(216,216,216);
   AddChild(previewArea=new BView (scale2(1,8,1,2,Bounds()),"sampleScreen",B_FOLLOW_NONE,0));
   previewArea->SetViewColor(lightBlue);
}

void PreviewView::Draw(BRect update)
{
	SetViewColor(216,216,216);
	SetHighColor(grey);
	FillRoundRect(scale2(0,9,0,3,Bounds()),4,4);
	SetHighColor(black);
	StrokeRoundRect(scale2(0,9,0,3,Bounds()),4,4);
	FillRoundRect(scale2(1,8,1,2,Bounds()),4,4);
	SetHighColor(grey);
	FillRoundRect(scale2(2,7,3,6,Bounds()),4,4);
	SetHighColor(black);
	StrokeLine(scale2(2,3,Bounds()),scale2(2,6,Bounds()));
	StrokeLine(scale2(2,6,Bounds()),scale2(7,6,Bounds()));
	StrokeLine(scale2(7,6,Bounds()),scale2(7,3,Bounds()));
	SetHighColor(lightGreen);
	FillRect(scale2(3,4,4,5,Bounds()));
	SetHighColor(darkGrey);
	FillRect(scale2(5,6,4,5,Bounds()));
}

void removeChildren(BView* view)
{
	int children = view->CountChildren();
	std::cout << "Removing " << children << " child(ren) from view\n";
	BView* child = view->ChildAt(0);
	while( child != NULL )
	{
		removeChildren( child );
		view->RemoveChild(child);
		delete child;
		child = view->ChildAt(0);
	}
}

void PreviewView::LoadNewAddon(const char* addOnFilename)
{
	status_t lastOpStatus;
	
	BScreenSaver *(*instantiate)(BMessage *, image_id );
	if (saver != 0)
	{
		// tell module the show's over
		saver->StopSaver();
		saver->StopConfig();
		// get rid of the settings elements
		removeChildren( settingsBoxPtr );
		settingsBoxPtr->Draw(settingsBoxPtr->Bounds());
		// get rid of the module
		delete saver;
		saver = 0;
		
		// clean up preview area
		previewArea->ClearViewOverlay();
		previewArea->ClearViewBitmap();
	}
	if (addonImage != 0)
	{
		unload_add_on(addonImage);
		addonImage = 0;
	}

	std::cout << "Loading add-on: " << addOnFilename << '\n';
	addonImage = load_add_on(addOnFilename);
	if (addonImage < 0 )
	{
		std::cout << "Unable to open the add-on\n"
					 "load_add_on returned " << std::hex << addonImage << std::dec << "!\n";
		return;
	}
	std::cout << "Add-on loaded properly! image = " << addonImage << '\n';

	lastOpStatus = get_image_symbol(addonImage, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate);
	if (lastOpStatus != B_OK)
	{
		// add-on does not have proper function exported!
		std::cout << "Add-on does not export instantiation function, "
		             "get_image_symbol() returned " << lastOpStatus << '\n';
		return;
	}

	std::cout << "Add-on supports correct functionality!\n";
	saver = instantiate(new BMessage, addonImage);
	std::cout << "instantiate() called correctly!\n";
		
	std::cout << "setting up screen saver for preview\n";
	lastOpStatus = saver->InitCheck();
	if ( lastOpStatus != B_OK )
	{
	 	std::cout << "InitCheck() said no go, returned " << lastOpStatus << '\n';
	 	return;
	}
	
	std::cout << "InitCheck() returned B_OK, calling StartSaver()\n";
	lastOpStatus = saver->StartSaver(previewArea, true);
	if ( lastOpStatus != B_OK )
	{
	 	std::cout << "StartSaver() said no go, returned " << lastOpStatus << '\n';
	 	return;
	}
	
	std::cout << "StartSaver() returned B_OK, calling StartConfig()\n";
	if ( settingsBoxPtr == 0 )
	{
		std::cout << "Settings box not instantiated yet!\n";
	}
	else
	{
		saver->StartConfig( settingsBoxPtr );
	}
	
	std::cout << "StartConfig() called, drawing first frame for now.\n";
	saver->Draw(previewArea, 0);
		
}