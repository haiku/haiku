#include "DirectWindow.h"
#include "ScreenSaver.h"

class SSAwindow : public BDirectWindow {
public:
	SSAwindow(BRect frame);
	~SSAwindow();
	virtual bool QuitRequested();
	virtual void DirectConnected(direct_buffer_info *info);
	void SetSaver(BScreenSaver *s) {fSaver=s;}

	BView *fView;
private:
	BScreenSaver *fSaver;
};
