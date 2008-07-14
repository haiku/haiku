//
// StickIt
// File: StickItApp.h
// Joystick application definitions.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include <Application.h>
#include <Joystick.h>

class StickItApp : public BApplication {
	public:
		StickItApp();
	private:
		StickItWindow	*fStickItWindow;
};
