#include "DirectWindow.h"
#include "ScreenSaver.h"

class SSAwindow : public BDirectWindow {
public:
	SSAwindow(BRect frame);
	~SSAwindow();
	virtual bool QuitRequested();
	virtual void DirectConnected(direct_buffer_info *info);
	BView *view;
	void SetSaver(BScreenSaver *s) {saver=s;}
private:
	BScreenSaver *saver;
};
