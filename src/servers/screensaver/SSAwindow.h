#include "DirectWindow.h"
#include "ScreenSaver.h"

class SSAwindow : public BDirectWindow {
public:
	SSAwindow(BRect frame,BScreenSaver *saverIn);
	~SSAwindow();
	virtual bool QuitRequested();
	virtual void DirectConnected(direct_buffer_info *info);
	BView *view;
private:
	BScreenSaver *saver;
};
