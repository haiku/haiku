#include <View.h>
#include <Box.h>

class BScreenSaver;

class PreviewView : public BView
{ 
public:	
	PreviewView(BRect frame, const char *name);
	void Draw(BRect update); 
	void LoadNewAddon(const char* addOnFilename);

	void SetSettingsBoxPtr( BBox* settingsBox )
	{ settingsBoxPtr = settingsBox; }

private:	
	BView *previewArea;
	BBox *settingsBoxPtr;
	image_id addonImage;
	BScreenSaver* saver;
};
