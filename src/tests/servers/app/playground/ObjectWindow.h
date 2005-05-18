// ObjectWindow.h

#ifndef OBJECT_WINDOW_H
#define OBJECT_WINDOW_H

#include <Window.h>

/*
class Box;
class Button;
class CheckBox;
class Menu;
class MenuBar;
class RadioButton;
class TextView;*/
class BButton;
class BCheckBox;
class BTextControl;
class ObjectView;

class ObjectWindow : public BWindow {
 public:
						ObjectWindow(BRect frame, const char* name);
	virtual				~ObjectWindow();

	virtual	bool		QuitRequested();

	virtual	void		MessageReceived(BMessage* message);

 private:
			void		_UpdateControls() const;
			rgb_color	_GetColor() const;

	ObjectView*			fObjectView;

	BButton*			fNewB;
	BButton*			fClearB;

	BTextControl*		fRedTC;
	BTextControl*		fGreenTC;
	BTextControl*		fBlueTC;
	BTextControl*		fAlphaTC;

	BCheckBox*			fFillCB;
	BTextControl*		fPenSizeTC;
};

#endif // OBJECT_WINDOW_H
