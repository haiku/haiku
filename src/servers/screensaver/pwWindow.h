#include "Window.h"
#include "Box.h"
#include "TextControl.h"
#include "Button.h"

class pwWindow : public BWindow
{
	public:
		pwWindow (void) : BWindow(BRect(100,100,400,230),"pwView",B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL  ,
				B_NOT_MOVABLE | B_NOT_CLOSABLE |B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS ,
				B_ALL_WORKSPACES), die(false) {setup();}
		void setup(void);
		bool die;
		const char *GetPassword(void) {return password->Text();}
	private:
		BView *bgd;
		BBox *customBox;
		BTextControl *password;
		BButton *unlock;
};

