#include <View.h>
#include <Box.h>
#include <MessageRunner.h>
#include <Bitmap.h>
#include <StringView.h>

class BScreenSaver;

class PreviewView : public BView
{ 
public:	
	PreviewView(BRect frame, const char *name);
	~PreviewView();
	void Draw(BRect update); 
	void LoadNewAddon(const char* addOnFilename, BMessage* settingsMsg);

	void SetSettingsBoxPtr( BBox* settingsBox )
	{ settingsBoxPtr = settingsBox; }
	
private:	
	BView *previewArea;
	BBox *settingsBoxPtr;
	BView *configView;
	image_id addonImage;
	BScreenSaver* saver;
	
	// to keep track of what to tear down
	bool stopSaver;
	bool stopConfigView;
	bool removeConfigView;
	bool deleteSaver;
	bool removePreviewArea;
	bool unloadAddon;
	bool noPreview;
	BStringView *noPreviewView;
	
}; // end class PreviewView

