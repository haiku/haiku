#ifndef PRINTTESTAPP_HPP
#define PRINTTESTAPP_HPP

class PrintTestApp;

#include <Application.h>
#include <Message.h>
#include <Rect.h>

#define PRINTTEST_SIGNATURE	"application/x-vnd.OpenBeOS-printtest"

class PrintTestApp : public BApplication
{
	typedef BApplication Inherited;
public:
	PrintTestApp();

	void MessageReceived(BMessage* msg);
	void ReadyToRun();
private:
	status_t DoTestPageSetup();
	status_t Print();

	BMessage* fSetupData;
	BRect fPrintableRect;
	BRect fPaperRect;
	
};

#endif
