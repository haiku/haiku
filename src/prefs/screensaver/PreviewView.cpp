#include "PreviewView.h"
#include "Constants.h"
#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <iostream>
#include <ScreenSaver.h>
#include <Errors.h>
#include <Screen.h>

struct
{
	int32 			previewThreadId;
	BView*			previewArea;
	BScreenSaver* 	saver;
	bool			stopMe;
} previewData;

// viewer thread
int32 previewThread(void* data)
{
	int cycleNumber = 0;
	if (previewData.saver == 0)
	{
		std::cout << "saver not there!\n";
		return 0;
	}
	
	int32 loopOnCount = previewData.saver->LoopOnCount();
	int32 loopOnCounter = 0;
	int32 loopOffCount = previewData.saver->LoopOffCount();
	int32 loopOffCounter = 0;
	
	if (loopOnCount == 0 && loopOffCount == 0)
	{
		// TODO: fill in setup for "normal" loop counting here?
	}
	
	snooze( previewData.saver->TickSize() );
	while (!previewData.stopMe)
	{
		if (previewData.previewArea->Window()->Lock())
		{
			if (!previewData.previewArea->IsHidden())
			{
				previewData.saver->Draw( previewData.previewArea, cycleNumber );
			}
			previewData.previewArea->Window()->Unlock();
		//		previewData.previewArea->Flush();
		}
		snooze( previewData.saver->TickSize() );
		++cycleNumber;
	}
} // end previewThread()

inline BPoint scaleDirect(float x, float y,BRect area)
{
	return BPoint(area.Width()*x+area.left,area.Height()*y+area.top);
}

inline BRect scaleDirect (float x1,float x2,float y1,float y2,BRect area)
{
	return BRect(area.Width()*x1+area.left,area.Height()*y1+area.top, area.Width()*x2+area.left,area.Height()*y2+area.top);
}

float sampleX[]= {0,.025,.25,.6,.625,.7,.725,.75,.975,1.0};
float sampleY[]= {0,.05,.8,.9,.933,.966,1.0};
inline BPoint scale2(int x, int y,BRect area) { return scaleDirect(sampleX[x],sampleY[y],area); }
inline BRect scale2(int x1, int x2, int y1, int y2,BRect area) { return scaleDirect(sampleX[x1],sampleX[x2],sampleY[y1],sampleY[y2],area); }

PreviewView::PreviewView(BRect frame, const char *name)
: BView (frame,name,B_FOLLOW_NONE,B_WILL_DRAW),
  addonImage (0),
  saver (0),
  settingsBoxPtr (0),
  configView (0),
  stopSaver (false),
  stopConfigView (false),
  removeConfigView (false),
  deleteSaver (false),
  removePreviewArea (false),
  unloadAddon (false),
  noPreview (false)
{
   SetViewColor(216,216,216);
   AddChild(previewArea=new BView (scale2(1,8,1,2,Bounds()),"sampleScreen",B_FOLLOW_NONE,B_WILL_DRAW));
   previewArea->SetViewColor(0,0,0);
   previewData.previewArea = previewArea;
   previewData.previewThreadId = 0;
   previewData.saver = 0;
   previewData.stopMe = false;
} // end PreviewView::PreviewView()

PreviewView::~PreviewView()
{
	if (previewData.previewThreadId != 0)
	{
		previewData.stopMe = true;
		snooze( saver->TickSize() );
		kill_thread( previewData.previewThreadId );
		previewData.stopMe = false;
	}
	
	if (stopSaver)
	{
		saver->StopSaver();
		stopSaver = false;
	}
	
	if (stopConfigView)
	{
		saver->StopConfig();
		stopConfigView = false;
	}
		
	if (deleteSaver)
	{
		delete saver;
		saver = 0;
		deleteSaver = false;
	}
	
	if (removeConfigView)
	{
		settingsBoxPtr->RemoveChild(configView);
		delete configView;
		settingsBoxPtr->Draw(settingsBoxPtr->Bounds());
		removeConfigView = false;
	}
	
	if (removePreviewArea)
	{
		previewArea->RemoveSelf();
		delete previewArea;
	}
	
} // end PreviewView::~PreviewView()

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

void PreviewView::LoadNewAddon(const char* addOnFilename, BMessage* settingsMsg)
{
	if (previewData.previewThreadId != 0)
	{
	    std::cout << "Ending old thread id=" << previewData.previewThreadId << '\n';
	    
		previewData.stopMe = true;
		// replace :
		//snooze( saver->TickSize() );
		//kill_thread( previewData.previewThreadId );
		// with :
		suspend_thread( previewData.previewThreadId );
		resume_thread( previewData.previewThreadId );
		// end replace
		previewData.stopMe = false;
		previewData.previewThreadId = 0;
	}
	
	status_t lastOpStatus;
	
	//screen saver's exported instantiation function
	BScreenSaver *(*instantiate)(BMessage *, image_id );
	
	if (stopSaver)
	{
	    std::cout << "stopping old saver\n";
		saver->StopSaver();
		stopSaver = false;
	}
	
	if (stopConfigView)
	{
	    std::cout << "stopping config view\n";
		saver->StopConfig();
		stopConfigView = false;
	}
		
	if (deleteSaver)
	{
	    std::cout << "deleting old saver\n";
	    
		delete saver;
		saver = 0;
		deleteSaver = false;
	}
	
	if (removeConfigView)
	{
	    std::cout << "removing old config view\n";
		settingsBoxPtr->RemoveChild(configView);
		delete configView;
		settingsBoxPtr->Draw(settingsBoxPtr->Bounds());
		removeConfigView = false;
	}
	
	if (noPreview)
	{
	    std::cout << "no preview\n";
		noPreviewView->RemoveSelf();
		delete noPreviewView;
	}
	
	if (removePreviewArea)
	{
	    std::cout << "removing preview area\n";
		previewArea->RemoveSelf();
		delete previewArea;
		previewArea=new BView (scale2(1,8,1,2,Bounds()),"sampleScreen",B_FOLLOW_NONE,B_WILL_DRAW);
		previewArea->SetViewColor( 0,0,0 );
		AddChild(previewArea);
		previewData.previewArea = previewArea;
		removePreviewArea = false;
	}
		
	if (unloadAddon)
	{
	    std::cout << "unloading add-on\n";
		unload_add_on(addonImage);
		addonImage = 0;
		unloadAddon = false;
	}

	addonImage = load_add_on(addOnFilename);
	if (addonImage < 0 )
	{
		std::cout << "Unable to open the add-on " << addOnFilename << "\n"
					 "load_add_on returned " << std::hex << addonImage << std::dec << "!\n";
		return;
	}
	unloadAddon = true;

    std::cout << "Add-on loaded\n";

	lastOpStatus = get_image_symbol(addonImage, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT,(void **) &instantiate);
	if (lastOpStatus != B_OK)
	{
		// add-on does not have proper function exported!
		std::cout << "Add-on does not export instantiation function, "
		             "get_image_symbol() returned " << lastOpStatus << '\n';
		return;
	}

    std::cout << "symbol imported\n";

	saver = instantiate(settingsMsg, addonImage);
	if (saver == 0)
	{
		std::cout << "Saver not instantiated.\n";
		return;
	}
	std::cout << "saver instantiated\n";
	deleteSaver = true;
	previewData.saver = saver;
			
	lastOpStatus = saver->InitCheck();
	if ( lastOpStatus != B_OK )
	{
	 	std::cout << "InitCheck() said no go, returned " << lastOpStatus << '\n';
	 	return;
	}
	
	std::cout << "InitCheck() says we're a go!\n";
	
	lastOpStatus = saver->StartSaver(previewArea, true);
	if ( lastOpStatus != B_OK )
	{
	 	std::cout << "StartSaver() said no go, returned " << lastOpStatus << '\n';
	 	noPreview = true;
	}
	else
	{
	    std::cout << "Saver started\n";
		stopSaver = true;
		noPreview = false;
	}
	
	// make config view
	BRect configViewBounds = settingsBoxPtr->Bounds();
	configViewBounds.InsetBy( 4, 16 );
	configView = new BView(configViewBounds, "settings", B_FOLLOW_ALL_SIDES, 0);
	configView->SetViewColor(216,216,216);
	settingsBoxPtr->AddChild( configView );
	saver->StartConfig( configView );
	stopConfigView = true;
	removeConfigView = true;

    std::cout << "Config view made\n";

	if ( noPreview )
	{
		noPreviewView = new BStringView(previewArea->Bounds(), "no_preview", "NO PREVIEW AVAILABLE");
		rgb_color white = { 255, 255, 255, 0};
		noPreviewView->SetHighColor(white);
		noPreviewView->SetAlignment(B_ALIGN_CENTER);
		noPreviewView->Draw(previewArea->Bounds());
		previewArea->AddChild(noPreviewView);
		std::cout << "no preview\n";
	}
	else
	{
		previewData.previewThreadId = spawn_thread( previewThread, "preview_thread", 50, NULL );
		resume_thread( previewData.previewThreadId );
		removePreviewArea = true;
		std::cout << "Preview thread started\n";
	}
		
} // end PreviewView::LoadNewAddon()


