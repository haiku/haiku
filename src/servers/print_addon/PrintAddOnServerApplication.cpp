/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#include "PrintAddOnServerApplication.h"

#include <PrinterDriverAddOn.h>

#include <String.h>

PrintAddOnServerApplication::PrintAddOnServerApplication(const char* signature)
	:
	BApplication(signature)
{

}


void
PrintAddOnServerApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageAddPrinter:
			AddPrinter(message);
			break;

		case kMessageConfigPage:
			ConfigPage(message);
			break;

		case kMessageConfigJob:
			ConfigJob(message);
			break;

		case kMessageDefaultSettings:
			DefaultSettings(message);
			break;

		case kMessageTakeJob:
			TakeJob(message);
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
PrintAddOnServerApplication::AddPrinter(BMessage* message)
{
	BString driver;
	BString name;
	if (message->FindString(kPrinterDriverAttribute, &driver) != B_OK
			|| message->FindString(kPrinterNameAttribute, &name) != B_OK) {
		SendReply(message, B_BAD_VALUE);
		return;
	}

	status_t status = AddPrinter(driver.String(), name.String());
	SendReply(message, status);
}


status_t
PrintAddOnServerApplication::AddPrinter(const char* driver,
	const char* spoolFolderName)
{
	PrinterDriverAddOn addOn(driver);
	return addOn.AddPrinter(spoolFolderName);
}


void
PrintAddOnServerApplication::ConfigPage(BMessage* message)
{
	BString driver;
	BString folder;
	BMessage settings;
	if (message->FindString(kPrinterDriverAttribute, &driver) != B_OK
		|| message->FindString(kPrinterFolderAttribute, &folder) != B_OK
		|| message->FindMessage(kPrintSettingsAttribute, &settings) != B_OK) {
		SendReply(message, B_BAD_VALUE);
		return;
	}

	BDirectory spoolFolder(folder.String());
	status_t status = ConfigPage(driver.String(), &spoolFolder, &settings);

	if (status != B_OK)
		SendReply(message, status);
	else {
		BMessage reply(B_REPLY);
		reply.AddMessage(kPrintSettingsAttribute, &settings);
		SendReply(message, &reply);
	}
}


status_t
PrintAddOnServerApplication::ConfigPage(const char* driver,
	BDirectory* spoolFolder, BMessage* settings)
{
	PrinterDriverAddOn addOn(driver);
	return addOn.ConfigPage(spoolFolder, settings);
}


void
PrintAddOnServerApplication::ConfigJob(BMessage* message)
{
	BString driver;
	BString folder;
	BMessage settings;
	if (message->FindString(kPrinterDriverAttribute, &driver) != B_OK
		|| message->FindString(kPrinterFolderAttribute, &folder) != B_OK
		|| message->FindMessage(kPrintSettingsAttribute, &settings) != B_OK) {
		SendReply(message, B_BAD_VALUE);
		return;
	}

	BDirectory spoolFolder(folder.String());
	status_t status = ConfigJob(driver.String(), &spoolFolder, &settings);

	if (status != B_OK)
		SendReply(message, status);
	else {
		BMessage reply(B_REPLY);
		reply.AddMessage(kPrintSettingsAttribute, &settings);
		SendReply(message, &reply);
	}
}


status_t
PrintAddOnServerApplication::ConfigJob(const char* driver,
				BDirectory* spoolFolder,
				BMessage* settings)
{
	PrinterDriverAddOn addOn(driver);
	return addOn.ConfigJob(spoolFolder, settings);
}


void
PrintAddOnServerApplication::DefaultSettings(BMessage* message)
{
	BString driver;
	BString folder;
	if (message->FindString(kPrinterDriverAttribute, &driver) != B_OK
		|| message->FindString(kPrinterFolderAttribute, &folder) != B_OK) {
		SendReply(message, B_BAD_VALUE);
		return;
	}

	BMessage settings;
	BDirectory spoolFolder(folder.String());
	status_t status = DefaultSettings(driver.String(), &spoolFolder, &settings);

	if (status != B_OK)
		SendReply(message, status);
	else {
		BMessage reply(B_REPLY);
		reply.AddMessage(kPrintSettingsAttribute, &settings);
		SendReply(message, &reply);
	}
}


status_t
PrintAddOnServerApplication::DefaultSettings(const char* driver,
	BDirectory* spoolFolder, BMessage* settings)
{
	PrinterDriverAddOn addOn(driver);
	return addOn.DefaultSettings(spoolFolder, settings);
}


void
PrintAddOnServerApplication::TakeJob(BMessage* message)
{
	BString driver;
	BString folder;
	BString jobFile;
	if (message->FindString(kPrinterDriverAttribute, &driver) != B_OK
		|| message->FindString(kPrinterFolderAttribute, &folder) != B_OK
		|| message->FindString(kPrintJobFileAttribute, &jobFile) != B_OK) {
		SendReply(message, B_BAD_VALUE);
		return;
	}

	BDirectory spoolFolder(folder.String());
	status_t status = TakeJob(driver.String(), jobFile.String(),
		&spoolFolder);
	SendReply(message, status);
}


status_t
PrintAddOnServerApplication::TakeJob(const char* driver, const char* spoolFile,
				BDirectory* spoolFolder)
{
	PrinterDriverAddOn addOn(driver);
	return addOn.TakeJob(spoolFile, spoolFolder);
}


void
PrintAddOnServerApplication::SendReply(BMessage* message, status_t status)
{
	BMessage reply(B_REPLY);
	reply.AddInt32(kPrintAddOnServerStatusAttribute, status);
	message->SendReply(&reply);
}


void
PrintAddOnServerApplication::SendReply(BMessage* message, BMessage* reply)
{
	reply->AddInt32(kPrintAddOnServerStatusAttribute, B_OK);
	message->SendReply(reply);
}


int
main(int argc, char* argv[])
{
	PrintAddOnServerApplication application(
		kPrintAddOnServerApplicationSignature);
	application.Run();
}
