#include <View.h>
#include <Box.h>
#include <MessageRunner.h>
#include <Bitmap.h>

class BScreenSaver;

class PreviewView : public BView
{ 
public:	
	PreviewView(BRect frame, const char *name);
	~PreviewView();
	void Draw(BRect update); 
	void LoadNewAddon(const char* addOnFilename);

	void SetSettingsBoxPtr( BBox* settingsBox )
	{ settingsBoxPtr = settingsBox; }

private:	
	BView *previewArea;
	BBox *settingsBoxPtr;
	BView *configView;
	image_id addonImage;
	BScreenSaver* saver;
	BMessageRunner* messageRunner;
	
	// to keep track of what to tear down
	bool stopSaver;
	bool stopConfigView;
	bool removeConfigView;
	bool deleteSaver;
	bool removePreviewArea;
	bool unloadAddon;
	
}; // end class PreviewView

