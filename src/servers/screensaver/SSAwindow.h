#include "DirectWindow.h"

class SSAwindow : public BDirectWindow {
public:
	SSAwindow(BRect frame);
	~SSAwindow();
	virtual bool QuitRequested();
	virtual void DirectConnected(direct_buffer_info *info);
};
