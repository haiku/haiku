#include "Mime.h"
#include "TypeConstants.h"
#include "Application.h"
#include "InterfaceDefs.h"
#include "TranslationUtils.h"
#include "Button.h"
#include "Errors.h"
#include "Window.h"
#include "Alert.h"
#include "TextControl.h"
#include "FindDirectory.h"
#include "Directory.h"
#include "Roster.h"
#include "Bitmap.h"
#include "OS.h"
#include "PrintJob.h"

// POSIX includes
#include "errno.h"
#include "malloc.h"
#include "signal.h"

#include "InstallPrinter.h"

#define PRT_SERVER_SIGNATURE	"application/x-vnd.Be-PSRV"

#define PRT_ATTR_MIMETYPE		"application/x-vnd.Be.printer\0"
#define PRT_ATTR_STATE			"free\0"
#define PRT_ATTR_TRANSPORT		"beserved\0"
#define PRT_ATTR_TRANSPORTADDR	"local\0"
#define PRT_ATTR_CONNECTION		"local\0"
#define PRT_ATTR_COMMENTS		"\0"

char *printerTypes[] = { "HP PCL3 LaserJet Compatible\0" };


// ----- InstallPrinterView -----------------------------------------------------

InstallPrinterView::InstallPrinterView(BRect rect, char *host, char *printer)
	: BView(rect, "InstallPrinterView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	strcpy(this->host, host);
	strcpy(this->printer, printer);

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Be.printer");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(55, 50, 250, 65);
	chkDefault = new BCheckBox(r, "Default", "Make this my default printer", NULL);
	chkDefault->SetValue(B_CONTROL_ON);
	AddChild(chkDefault);

	r.top = 70;
	r.bottom = r.top + 20;
	chkTestPage = new BCheckBox(r, "TestPage", "Print a test page", NULL);
	chkTestPage->SetValue(B_CONTROL_OFF);
	chkTestPage->SetEnabled(false);
	AddChild(chkTestPage);

	r.Set(140, 105, 260, 125);
	BButton *okBtn = new BButton(r, "OkayBtn", "Configure Printer", new BMessage(MSG_INSTALL_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(270, 105, 340, 125);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_INSTALL_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
}

InstallPrinterView::~InstallPrinterView()
{
}

void InstallPrinterView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	char msg[512];

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	DrawString("Configure this Printer?");

	sprintf(msg, "'%s' on computer '%s?'", printer, host);
	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("Would you like to configure your computer to print to printer");
	MovePenTo(55, 40);
	DrawString(msg);

	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(0, 0, 0, 180);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}


// ----- InstallPrinterPanel ----------------------------------------------------------------------

InstallPrinterPanel::InstallPrinterPanel(BRect frame, char *host, char *printer)
	: BWindow(frame, "New Printer", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	cancelled = false;

	BRect r = Bounds();
	installView = new InstallPrinterView(r, host, printer);
	AddChild(installView);

	Show();
}

InstallPrinterPanel::~InstallPrinterPanel()
{
}

// MessageReceived()
//
void InstallPrinterPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_INSTALL_OK:
			cancelled = false;
			defaultPrinter = installView->isDefault();
			sendTestPage = installView->printTestPage();
			BWindow::Quit();
			break;

		case MSG_INSTALL_CANCEL:
			cancelled = true;
			BWindow::Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

// CreatePrinter()
//
bool InstallPrinterPanel::CreatePrinter(char *printer, char *host, int type, char *user, char *password)
{
	char path[B_PATH_NAME_LENGTH], msg[B_PATH_NAME_LENGTH + 256];

	// Create the printer spooling folder.  If it cannot be created, we'll have to abort.
	find_directory(B_USER_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/printers/");
	strcat(path, printer);
	if (create_directory(path, S_IRWXU | S_IRWXG | S_IRWXO) != B_OK)
	{
		sprintf(msg, "A printer spooling folder, %s, could not be created.", path);
		BAlert *alert = new BAlert("", msg, "OK");
		alert->Go();
		return false;
	}

	// Now set the necessary attributes for the printing system to work.  This includes
	// standard BeOS attributes, such as BEOS:TYPE and Driver Name, but also includes those
	// specific to BeServed, such as "server," which identifies the remote host.
	int file = open(path, O_RDONLY);
	if (file >= 0)
	{
		fs_write_attr(file, "BEOS:TYPE", B_STRING_TYPE, 0, PRT_ATTR_MIMETYPE, strlen(PRT_ATTR_MIMETYPE) + 1);
		fs_write_attr(file, "Driver Name", B_STRING_TYPE, 0, printerTypes[type], strlen(printerTypes[type]) + 1);
		fs_write_attr(file, "Printer Name", B_STRING_TYPE, 0, printer, strlen(printer) + 1);
		fs_write_attr(file, "state", B_STRING_TYPE, 0, PRT_ATTR_STATE, strlen(PRT_ATTR_STATE) + 1);
		fs_write_attr(file, "transport", B_STRING_TYPE, 0, PRT_ATTR_TRANSPORT, strlen(PRT_ATTR_TRANSPORT) + 1);
		fs_write_attr(file, "transport_address", B_STRING_TYPE, 0, PRT_ATTR_TRANSPORTADDR, strlen(PRT_ATTR_TRANSPORTADDR) + 1);
		fs_write_attr(file, "connection", B_STRING_TYPE, 0, PRT_ATTR_CONNECTION, strlen(PRT_ATTR_CONNECTION) + 1);
		fs_write_attr(file, "Comments", B_STRING_TYPE, 0, PRT_ATTR_COMMENTS, strlen(PRT_ATTR_COMMENTS) + 1);
		fs_write_attr(file, "server", B_STRING_TYPE, 0, host, strlen(host) + 1);
		fs_write_attr(file, "user", B_STRING_TYPE, 0, user, strlen(user) + 1);
		fs_write_attr(file, "password", B_STRING_TYPE, 0, password, BT_AUTH_TOKEN_LENGTH + 1);
		close(file);
	}

//	if (!NotifyPrintServer(printer, type))
		RestartPrintServer();

	return true;
}

void InstallPrinterPanel::RestartPrintServer()
{
	app_info appInfo;

	if (be_roster->GetAppInfo(PRT_SERVER_SIGNATURE, &appInfo) == B_OK)
	{
		int retries = 0;
		kill(appInfo.thread, SIGTERM);
		while (be_roster->IsRunning(PRT_SERVER_SIGNATURE) && retries++ < 5)
			sleep(1);
	}

	be_roster->Launch(PRT_SERVER_SIGNATURE);
}

void InstallPrinterPanel::SetDefaultPrinter(char *printer)
{
	FILE *fp;
	char path[B_PATH_NAME_LENGTH];
	int i;

	// Create the printer spooling folder.  If it cannot be created, we'll have to abort.
	find_directory(B_USER_SETTINGS_DIRECTORY, 0, false, path, sizeof(path));
	strcat(path, "/printer_data");

	fp = fopen(path, "wb");
	if (fp)
	{
		fputs(printer, fp);
		for (i = strlen(printer); i < 256; i++)
			fputc('\0', fp);

		fclose(fp);
	}
}

bool InstallPrinterPanel::NotifyPrintServer(char *printer, int type)
{
	BMessage msg('selp');
	BMessenger msgr;

	if (!GetPrintServerMessenger(msgr))
		return false;

	msg.AddString("driver", printerTypes[type]);
	msg.AddString("transport", PRT_ATTR_TRANSPORT);
	msg.AddString("transport path", PRT_ATTR_TRANSPORTADDR);
	msg.AddString("printer name", printer);
	msg.AddString("connection", PRT_ATTR_CONNECTION);
	msgr.SendMessage(&msg);
	return true;
}

bool InstallPrinterPanel::GetPrintServerMessenger(BMessenger &msgr)
{
	if (!be_roster->IsRunning(PRT_SERVER_SIGNATURE))
		be_roster->Launch(PRT_SERVER_SIGNATURE);

	msgr = BMessenger(PRT_SERVER_SIGNATURE);
	return msgr.IsValid();
}


void InstallPrinterPanel::TestPrinter(char *printer)
{
	char *jobName = new char [strlen(printer) + 20];
	if (!jobName)
		return;

	sprintf(jobName, "%s Test Page", printer);
	BPrintJob job(jobName);

	if (job.ConfigPage() == B_OK)
		if (job.ConfigJob() == B_OK)
		{
			BRect printableRect = job.PrintableRect();
			BPoint startPoint(printableRect.left, printableRect.top);
			TestPrintView *testView = new TestPrintView(printableRect);
		
			job.BeginJob();
			job.DrawView(testView, printableRect, startPoint);
			job.SpoolPage();
			job.CommitJob();
		
			delete testView;
		}

	delete jobName;
}

// ----- TestPrintView ------------------------------------------------------------------------

TestPrintView::TestPrintView(BRect rect)
	: BView(rect, "TestPrintView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BMessage archive;
	size_t size;
	char *bits;

	rgb_color white = { 255, 255, 255, 255 };
	SetViewColor(white);

//	bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), BMP_TESTPAGE_LOGO, &size);
//	if (bits)
//		if (archive.Unflatten(bits) == B_OK)
//			beServedIcon = new BBitmap(&archive);
}

TestPrintView::~TestPrintView()
{
}

void TestPrintView::Draw(BRect rect)
{
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color white = { 255, 255, 255, 255 };

	SetViewColor(white);
	SetLowColor(white);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	DrawString("");

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("This test page has been sent to your printer by request.  By reading this information, you are confirming");
	MovePenTo(55, 40);
	DrawString("that this printer is communicating correctly with the computer from which this test was sent.");

	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(0, 0, 0, 180);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(beServedIcon, iconRect);
}
