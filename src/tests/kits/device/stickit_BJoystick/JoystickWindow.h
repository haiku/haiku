//
// StickIt
// File: JoystickWindow.h
// Joystick window definitions.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include <Window.h>
#include <View.h>

class BJoystick;

class JoystickView : public BView {
	public:
							JoystickView(BRect bounds, BJoystick *stick);
		virtual void		Draw(BRect updateRect);
		virtual void		Pulse(void);
	
	private:
				BRect		_BuildButtons(BJoystick *stick);
				BRect		_BuildHats(BJoystick *stick, BRect rect);
				void		_BuildAxes(BJoystick *stick, BRect rect);
				BRect		_BuildString(BString name, const char* strName, 
								int number, BRect rect);

		BJoystick			*fStick;
		BRect				fLastHatRect;
};

class JoystickWindow : public BWindow {
	public:
							JoystickWindow(BJoystick *stick, BRect rect);
		virtual bool		QuitRequested(void);
	
	private:
		JoystickView		*fView;
};
