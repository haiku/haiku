#include "Window.h"
#include "Box.h"
#include "TextControl.h"
#include "Button.h"

class pwWindow : public BWindow
{
	public:
		pwWindow (void) : BWindow(BRect(100,100,400,230),"pwView",B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL  ,
				B_NOT_MOVABLE | B_NOT_CLOSABLE |B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS ,
				B_ALL_WORKSPACES), fDie(false) {setup();}
		void setup(void);
		const char *GetPassword(void) {return fPassword->Text();}

		bool fDie;
	private:
		BView *fBgd;
		BBox *fCustomBox;
		BTextControl *fPassword;
		BButton *fUnlock;
};

