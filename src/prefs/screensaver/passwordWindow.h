#include "Window.h"
#include "CheckBox.h"
#include "String.h"
#include "Box.h"
#include "TextControl.h"
#include "Button.h"
#include "Constants.h"

class pwWindow : public BWindow
{
	public:
		pwWindow (void) : BWindow(BRect(100,100,380,250),"",B_MODAL_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE) {setup();}
		void setup(void);
		void update(void);
		virtual void MessageReceived(BMessage *message);

	private:
		BRadioButton *useNetwork,*useCustom;
		BBox *customBox;
		BTextControl *password,*confirm;
		BButton *cancel,*done;
		BString thePassword; 
		bool useNetPassword;

};

