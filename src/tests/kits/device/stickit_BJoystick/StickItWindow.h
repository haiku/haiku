//
// StickIt
// File: jwindow.h
// Joystick window definitions.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include <Window.h>
#include <View.h>

class JoystickWindow;
class BJoystick;
class BBox;
class BStringView;
class BListView;

class StickItWindow : public BWindow {
	public:
					 	StickItWindow(BRect rect);
				void 	PickJoystick(BJoystick *stick);
		virtual bool 	QuitRequested(void);
		virtual	void 	MessageReceived(BMessage *message);
				BString AddToList(BListView *bl, const char * str);
	
	private:
		BListView*		fListView1;
		BListView*		fListView2;
		BJoystick*		fJoystick;
		JoystickWindow*	fJoystickWindow;
};
