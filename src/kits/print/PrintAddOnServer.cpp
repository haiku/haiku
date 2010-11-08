#include "PrintAddOnServer.h"

#include <Entry.h>
#include <Roster.h>

#include "PrinterDriverAddOn.h"
#include "PrintAddOnServerProtocol.h"

static const bigtime_t kSeconds = 1000000L;
static const bigtime_t kDeliveryTimeout = 30 * kSeconds;
static const bigtime_t kReplyTimeout = B_INFINITE_TIMEOUT;


PrintAddOnServer::PrintAddOnServer(const char* driver)
	:
	fDriver(driver)
{
	fLaunchStatus = Launch(fMessenger);
}


PrintAddOnServer::~PrintAddOnServer()
{
	if (fLaunchStatus == B_OK)
		Quit();
}


status_t
PrintAddOnServer::AddPrinter(const char* spoolFolderName)
{
	BMessage message(kMessageAddPrinter);
	message.AddString(kPrinterDriverAttribute, Driver());
	message.AddString(kPrinterNameAttribute, spoolFolderName);

	BMessage reply;
	status_t result = SendRequest(message, reply);
	if (result != B_OK)
		return result;

	return GetResult(reply);
}


status_t
PrintAddOnServer::ConfigPage(BDirectory* spoolFolder,
	BMessage* settings)
{
	BMessage message(kMessageConfigPage);
	message.AddString(kPrinterDriverAttribute, Driver());
	AddDirectory(message, kPrinterFolderAttribute, spoolFolder);
	message.AddMessage(kPrintSettingsAttribute, settings);

	BMessage reply;
	status_t result = SendRequest(message, reply);
	if (result != B_OK)
		return result;

	return GetResultAndUpdateSettings(reply, settings);
}


status_t
PrintAddOnServer::ConfigJob(BDirectory* spoolFolder,
	BMessage* settings)
{
	BMessage message(kMessageConfigJob);
	message.AddString(kPrinterDriverAttribute, Driver());
	AddDirectory(message, kPrinterFolderAttribute, spoolFolder);
	message.AddMessage(kPrintSettingsAttribute, settings);

	BMessage reply;
	status_t result = SendRequest(message, reply);
	if (result != B_OK)
		return result;

	return GetResultAndUpdateSettings(reply, settings);
}


status_t
PrintAddOnServer::DefaultSettings(BDirectory* spoolFolder,
	BMessage* settings)
{
	BMessage message(kMessageDefaultSettings);
	message.AddString(kPrinterDriverAttribute, Driver());
	AddDirectory(message, kPrinterFolderAttribute, spoolFolder);

	BMessage reply;
	status_t result = SendRequest(message, reply);
	if (result != B_OK)
		return result;

	return GetResultAndUpdateSettings(reply, settings);
}


status_t
PrintAddOnServer::TakeJob(const char* spoolFile,
				BDirectory* spoolFolder)
{
	BMessage message(kMessageTakeJob);
	message.AddString(kPrinterDriverAttribute, Driver());
	message.AddString(kPrintJobFileAttribute, spoolFile);
	AddDirectory(message, kPrinterFolderAttribute, spoolFolder);

	BMessage reply;
	status_t result = SendRequest(message, reply);
	if (result != B_OK)
		return result;

	return GetResult(reply);
}


status_t
PrintAddOnServer::FindPathToDriver(const char* driver, BPath* path)
{
	return PrinterDriverAddOn::FindPathToDriver(driver, path);
}


const char*
PrintAddOnServer::Driver() const
{
	return fDriver.String();
}


status_t
PrintAddOnServer::Launch(BMessenger& messenger)
{
	team_id team;
	status_t result =
		be_roster->Launch(kPrintAddOnServerApplicationSignature,
			(BMessage*)NULL, &team);
	if (result != B_OK) {
		return result;
	}

	fMessenger = BMessenger(kPrintAddOnServerApplicationSignature, team);
	return result;
}


bool
PrintAddOnServer::IsLaunched()
{
	return fLaunchStatus == B_OK;
}


void
PrintAddOnServer::Quit()
{
	if (fLaunchStatus == B_OK) {
		fMessenger.SendMessage(B_QUIT_REQUESTED);
		fLaunchStatus = B_ERROR;
	}
}


void
PrintAddOnServer::AddDirectory(BMessage& message, const char* name,
	BDirectory* directory)
{
	BEntry entry;
	status_t result = directory->GetEntry(&entry);
	if (result != B_OK)
		return;

	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return;

	message.AddString(name, path.Path());
}


void
PrintAddOnServer::AddEntryRef(BMessage& message, const char* name,
	const entry_ref* entryRef)
{
	BPath path(entryRef);
	if (path.InitCheck() != B_OK)
		return;

	message.AddString(name, path.Path());
}


status_t
PrintAddOnServer::SendRequest(BMessage& request, BMessage& reply)
{
	if (!IsLaunched())
		return fLaunchStatus;

	return fMessenger.SendMessage(&request, &reply, kDeliveryTimeout,
		kReplyTimeout);
}


status_t
PrintAddOnServer::GetResult(BMessage& reply)
{
	int32 status;
	status_t result = reply.FindInt32(kPrintAddOnServerStatusAttribute,
		&status);
	if (result != B_OK)
		return result;
	return static_cast<status_t>(status);
}


status_t
PrintAddOnServer::GetResultAndUpdateSettings(BMessage& reply, BMessage* settings)
{
	BMessage newSettings;
	if (reply.FindMessage(kPrintSettingsAttribute, &newSettings) == B_OK)
		*settings = newSettings;

	settings->PrintToStream();

	return GetResult(reply);
}
