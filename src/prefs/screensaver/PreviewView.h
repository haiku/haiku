#include <View.h>
#include <Box.h>
#include <ScreenSaverThread.h>
#include <ScreenSaverPrefs.h>

class BScreenSaver;

class PreviewView : public BView
{ 
public:	
	PreviewView(BRect frame, const char *name,ScreenSaverPrefs *prefp);
	~PreviewView();
	void Draw(BRect update); 
	void SetScreenSaver(BString name);
	BScreenSaver *ScreenSaver(void) {return saver;}	
private:	
	BScreenSaver* saver;
	BView *configView;
	ScreenSaverThread *sst;
	thread_id threadID;
	ScreenSaverPrefs *prefPtr;
	
}; // end class PreviewView

