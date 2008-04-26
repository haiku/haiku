#include "Window.h"
#include "View.h"
#include "Button.h"
#include "CheckBox.h"
#include "Bitmap.h"

#include "betalk.h"

const uint32 MSG_INSTALL_OK = 'InOK';
const uint32 MSG_INSTALL_CANCEL = 'InCn';


// ----- InstallPrinterView -----------------------------------------------------

class InstallPrinterView : public BView
{
	public:
		InstallPrinterView(BRect rect, char *host, char *printer);
		~InstallPrinterView();

		void Draw(BRect rect);

		bool isDefault()		{ return chkDefault->Value() == B_CONTROL_ON; }
		bool printTestPage()	{ return chkTestPage->Value() == B_CONTROL_ON; }

	private:
		BBitmap *icon;
		BButton *okBtn;
		BCheckBox *chkDefault;
		BCheckBox *chkTestPage;
		char host[B_FILE_NAME_LENGTH];
		char printer[B_FILE_NAME_LENGTH];
};


// ----- InstallPrinterPanel ----------------------------------------------------------------------

class InstallPrinterPanel : public BWindow
{
	public:
		InstallPrinterPanel(BRect frame, char *host, char *printer);
		~InstallPrinterPanel();

		bool CreatePrinter(char *printer, char *host, int type, char *user, char *password);
		void RestartPrintServer();
		void SetDefaultPrinter(char *printer);
		void TestPrinter(char *printer);

		void MessageReceived(BMessage *msg);
		bool IsCancelled()		{ return cancelled; }
		bool isDefault()		{ return defaultPrinter; }
		bool printTestPage()	{ return sendTestPage; }

		sem_id installSem;

	private:
		bool NotifyPrintServer(char *printer, int type);
		bool GetPrintServerMessenger(BMessenger &msgr);

		InstallPrinterView *installView;
		bool cancelled;
		bool defaultPrinter;
		bool sendTestPage;
};


// ----- TestPrintView -----------------------------------------------------

class TestPrintView : public BView
{
	public:
		TestPrintView(BRect rect);
		~TestPrintView();

		void Draw(BRect rect);

	private:
		BBitmap *beServedIcon;
};
