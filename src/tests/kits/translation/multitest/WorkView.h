// WorkView.h

#ifndef WORK_VIEW_H
#define WORK_VIEW_H

#include <View.h>

class WorkView : public BView {
public:
	WorkView(BRect frame);
	
	virtual void AttachedToWindow();
	virtual void Pulse();

private:
	bool fbImage;
	const char *fPath;
};

#endif