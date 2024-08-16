#ifndef T_FIND_PANEL_LOOPER
#define T_FIND_PANEL_LOOPER


#include <utility>
#include <iostream>

#include <fs_attr.h>

#include <Looper.h>


class BMessage;

namespace BPrivate {

class BQueryPoseView;


class TFindPanelLooper : public BLooper {
public:
								TFindPanelLooper(BQueryPoseView*);
								~TFindPanelLooper();

protected:
	virtual	void				MessageReceived(BMessage*);
	
private:
			BQueryPoseView*		fQueryPoseView;

	typedef	BLooper _inherited;
};

}

#endif