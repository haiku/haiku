#include "InitParamsPanel.h"

#include <stdio.h>

#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>



class InitParamsPanel::EscapeFilter : public BMessageFilter {
public:
	EscapeFilter(InitParamsPanel* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fPanel(target)
	{
	}
	virtual	~EscapeFilter()
	{
	}
	virtual filter_result Filter(BMessage* message, BHandler** target)
	{
		filter_result result = B_DISPATCH_MESSAGE;
		switch (message->what) {
			case B_KEY_DOWN:
			case B_UNMAPPED_KEY_DOWN: {
				uint32 key;
				if (message->FindInt32("raw_char", (int32*)&key) >= B_OK) {
					if (key == B_ESCAPE) {
						result = B_SKIP_MESSAGE;
						fPanel->Cancel();
					}
				}
				break;
			}
			default:
				break;
		}
		return result;
	}
private:
 	InitParamsPanel*		fPanel;
};


// #pragma mark -


enum {
	MSG_OK						= 'okok',
	MSG_CANCEL					= 'cncl'
};



InitParamsPanel::InitParamsPanel(BWindow *window)
	: Panel(BRect(300.0, 200.0, 600.0, 400.0), 0, B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
	, fEscapeFilter(new EscapeFilter(this))
	, fExitSemaphore(create_sem(0, "InitParamsPanel exit")
	, fWindow(window)
	, fReturnValue(GO_CANCELED)
{
	AddCommonFilter(fEscapeFilter);

	...

	AddChild(topView);
	SetDefaultButton(okButton);

	AddToSubset(fWindow);
}


InitParamsPanel::~InitParamsPanel()
{
	RemoveCommonFilter(fEscapeFilter);
	delete fEscapeFilter;

	delete_sem(fExitSemaphore);
}


bool
InitParamsPanel::QuitRequested()
{
	release_sem(fExitSemaphore);
	return false;
}


void
InitParamsPanel::MessageReceived(BMessage* message)
{
	switch (msg->what) {
		case MSG_CANCEL:
			Cancel();
			break;

		case MSG_OK:
			fReturnValue = GO_SUCCESS;
			release_sem(fExitSemaphore);
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}


int32
InitParamsPanel::Answer(BString& name, BString& parameters)
{
	// run the window thread, to get an initial layout of the controls
	Hide();
	Show();
	if (!Lock())
		return false;

	// center the panel above the parent window
	BRect frame = Frame();
	BRect parentFrame = fWindow->Frame();
	MoveTo((parentFrame.left + parentFrame.right - frame.Width()) / 2.0,
		(parentFrame.top + parentFrame.bottom - frame.Height()) / 2.0);

	fNameTC->SetText(name.String());
	fNameTC->MakeFocus(true);

	Show();
	Unlock();

	// block this thread now
	acquire_sem(fExitSemaphore);

	if (fMode == GO_SUCCESS) {
		name = fNameTC->Text();
		parameters ...
	}

	Lock();
	Quit();
	return fReturnValue;
}


void
InitParamsPanel::Cancel()
{
	fReturnValue = GO_CANCELED;
	release_sem(fExitSemaphore);
}






