// ObjectWindow.h

#ifndef OBJECT_WINDOW_H
#define OBJECT_WINDOW_H

#include <Window.h>

class BButton;
class BCheckBox;
class BColorControl;
class BListView;
class BMenuField;
class BTextControl;
class BSlider;
class ObjectView;

class ObjectWindow : public BWindow {
 public:
						ObjectWindow(BRect frame, const char* name);
	virtual				~ObjectWindow();

	virtual	bool		QuitRequested();

	virtual	void		MessageReceived(BMessage* message);

 private:
			void		_UpdateControls() const;
			void		_UpdateColorControls() const;
			rgb_color	_GetColor() const;

	ObjectView*			fObjectView;

	BButton*			fNewB;
	BButton*			fClearB;

	BButton*			fUndoB;
	BButton*			fRedoB;

	BMenuField*			fDrawingModeMF;

	BColorControl*		fColorControl;
	BTextControl*		fAlphaTC;

	BCheckBox*			fFillCB;
	BSlider*			fPenSizeS;

	BListView*			fObjectLV;
};

#endif // OBJECT_WINDOW_H
