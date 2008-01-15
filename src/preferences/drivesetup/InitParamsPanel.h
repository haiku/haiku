#ifndef INIT_PARAMS_PANEL_H
#define INIT_PARAMS_PANEL_H

#include <Window.h>

class BMenuField;
class BTextControl;

enum {
	GO_CANCELED	= 0,
	GO_SUCCESS
};

class InitParamsPanel : public BWindow {
public:
								InitParamsPanel(BWindow* window);
	virtual						~InitParamsPanel();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			int32				Go(BString& name, BString& parameters);
			void				Cancel();

private:
	class EscapeFilter;
			EscapeFilter*		fEscapeFilter;
			sem_id				fExitSemaphore;
			BWindow*			fWindow;
			int32				fReturnValue;
		
			BTextControl*		fNameTC;
			BMenuField*			fBlockSizeMF;
};

#endif // INIT_PARAMS_PANEL_H
