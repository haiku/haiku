#ifndef POP_UP_TEXT_CONTROL_H
#define POP_UP_TEXT_CONTROL_H


#include <TextControl.h>


class BMenuItem;
class BPopUpMenu;

namespace BPrivate {

// Declaration of Message Constants
const uint32 kOptionsMenuChanged = 'Fomc';

class PopUpTextControl : public BTextControl {
public:
								PopUpTextControl(const char*, const char*, const char*,
									BHandler*);
			BPopUpMenu*			PopUpMenu() {return fOptionsMenu;}

protected:
	virtual	void				MessageReceived(BMessage*);

private:
	virtual	void				LayoutChanged() override;
	virtual	void				DrawAfterChildren(BRect) override;
	virtual	void				MouseDown(BPoint) override;
private:
			BPopUpMenu*			fOptionsMenu;
			BHandler*			fTarget;
};

}

#endif
