/*
 * PrinterDriver.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2004 Michael Pfeiffer.
 */

#include "PrinterDriver.h"

#include <fs_attr.h> // for attr_info
#include <DataIO.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <StackOrHeapArray.h>
#include <String.h>

#include "AboutBox.h"
#include "AddPrinterDlg.h"
#include "DbgMsg.h"
#include "Exports.h"
#include "GraphicsDriver.h"
#include "PrinterCap.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "Preview.h"
#include "PrintUtils.h"


// Implementation of PrinterDriver

PrinterDriver::PrinterDriver(BNode* spoolFolder)
	:
	fSpoolFolder(spoolFolder),
	fPrinterData(NULL),
	fPrinterCap(NULL),
	fGraphicsDriver(NULL)
{
}

PrinterDriver::~PrinterDriver()
{
	delete fGraphicsDriver;
	fGraphicsDriver = NULL;
	
	delete fPrinterCap;
	fPrinterCap = NULL;

	delete fPrinterData;
	fPrinterData = NULL;
}


PrinterData*
PrinterDriver::InstantiatePrinterData(BNode* node)
{
	return new PrinterData(node);
}

void
PrinterDriver::InitPrinterDataAndCap() {
	fPrinterData = InstantiatePrinterData(fSpoolFolder);
	fPrinterData->Load();
	// NOTE: moved the load above from the constructor of PrinterData as
	//   we're inheriting from PrinterData and want our overridden versions
	//   of load to be called
	fPrinterCap = InstantiatePrinterCap(fPrinterData);
}

void
PrinterDriver::About()
{
	BString copyright;
	copyright = "libprint Copyright © 1999-2000 Y.Takagi\n";
	copyright << GetCopyright();
	copyright << "All Rights Reserved.";
	
	AboutBox app(GetSignature(), GetDriverName(), GetVersion(), copyright.String());
	app.Run();
}

char*
PrinterDriver::AddPrinter(char* printerName)
{
	// print_server has created a spool folder with name printerName in
	// folder B_USER_PRINTERS_DIRECTORY. It can be used to store
	// settings in the folder attributes.
	DBGMSG((">%s: add_printer\n", GetDriverName()));
	DBGMSG(("\tprinter_name: %s\n", printerName));
	DBGMSG(("<%s: add_printer\n", GetDriverName()));
	
	if (fPrinterCap->Supports(PrinterCap::kProtocolClass)) {
		if (fPrinterCap->CountCap(PrinterCap::kProtocolClass) > 1) {
			AddPrinterDlg *dialog;
			dialog = new AddPrinterDlg(fPrinterData, fPrinterCap);
			if (dialog->Go() != B_OK) {
				// dialog canceled
				return NULL;
			}
		} else {
			const ProtocolClassCap* pcCap;
			pcCap = (const ProtocolClassCap*)fPrinterCap->GetDefaultCap(
				PrinterCap::kProtocolClass);
			if (pcCap != NULL) {
				fPrinterData->SetProtocolClass(pcCap->fProtocolClass);
				fPrinterData->Save();
			}
		}
	}
	return printerName;
}

BMessage*
PrinterDriver::ConfigPage(BMessage* settings)
{
	DBGMSG((">%s: config_page\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);
	
	BMessage pageSettings(*settings);
	_MergeWithPreviousSettings(kAttrPageSettings, &pageSettings);
	UIDriver uiDriver(&pageSettings, fPrinterData, fPrinterCap);
	BMessage *result = uiDriver.ConfigPage();
	_WriteSettings(kAttrPageSettings, result);

	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: config_page\n", GetDriverName()));
	return result;
}

BMessage*
PrinterDriver::ConfigJob(BMessage* settings)
{
	DBGMSG((">%s: config_job\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);
	
	BMessage jobSettings(*settings);
	_MergeWithPreviousSettings(kAttrJobSettings, &jobSettings);
	UIDriver uiDriver(&jobSettings, fPrinterData, fPrinterCap);
	BMessage *result = uiDriver.ConfigJob();
	_WriteSettings(kAttrJobSettings, result);
	
	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: config_job\n", GetDriverName()));
	return result;
}

BMessage*
PrinterDriver::TakeJob(BFile* printJob, BMessage* settings)
{
	DBGMSG((">%s: take_job\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);

	fGraphicsDriver = InstantiateGraphicsDriver(settings, fPrinterData, fPrinterCap);
	const JobData* jobData = fGraphicsDriver->GetJobData(printJob);
	if (jobData != NULL && jobData->GetShowPreview()) {
		off_t offset = printJob->Position();
		PreviewWindow *preview = new PreviewWindow(printJob, true);
		if (preview->Go() != B_OK) {
			return new BMessage('okok');
		}
		printJob->Seek(offset, SEEK_SET);
	}	
	BMessage *result = fGraphicsDriver->TakeJob(printJob);

	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: take_job\n", GetDriverName()));
	return result;
}

// read settings from spool folder attribute
bool
PrinterDriver::_ReadSettings(const char* attrName, BMessage* settings)
{
	attr_info info;
	ssize_t size;

	settings->MakeEmpty();
	
	if (fSpoolFolder->GetAttrInfo(attrName, &info) == B_OK && info.size > 0) {
		BStackOrHeapArray<char, 0> data(info.size);
		if (!data.IsValid())
			return false;
		size = fSpoolFolder->ReadAttr(attrName, B_MESSAGE_TYPE, 0, data, info.size);
		if (size == info.size && settings->Unflatten(data) == B_OK) {
			return true;
		}
	}
	return false;
}

// write settings to spool folder attribute
void
PrinterDriver::_WriteSettings(const char* attrName, BMessage* settings)
{
	if (settings == NULL || settings->what != 'okok') return;
	
	status_t status;
	BMallocIO data;
	status = settings->Flatten(&data);
	
	if (status == B_OK) {
		fSpoolFolder->WriteAttr(attrName, B_MESSAGE_TYPE, 0, data.Buffer(),
			data.BufferLength());
	}
}

// read settings from spool folder attribute and merge them to current settings
void
PrinterDriver::_MergeWithPreviousSettings(const char* attrName, BMessage* settings)
{
	if (settings == NULL) return;
	
	BMessage stored;
	if (_ReadSettings(attrName, &stored)) {
		AddFields(&stored, settings);
		*settings = stored;
	}
}

// Implementation of PrinterDriverInstance

class PrinterDriverInstance
{
public:
	PrinterDriverInstance(BNode* spoolFolder = NULL);
	~PrinterDriverInstance();
	PrinterDriver* GetPrinterDriver() { return fInstance; }

private:
	PrinterDriver* fInstance;
};

PrinterDriverInstance::PrinterDriverInstance(BNode* spoolFolder)
{
	fInstance = instantiate_printer_driver(spoolFolder);
	if (fInstance != NULL) {
		fInstance->InitPrinterDataAndCap();
	}
}

PrinterDriverInstance::~PrinterDriverInstance()
{
	delete fInstance;
	fInstance = NULL;
}


// printer driver add-on functions

char *add_printer(char *printerName)
{
	BPath path;
	BNode folder;
	BNode* spoolFolder = NULL;
	// get spool folder
	if (find_directory(B_USER_PRINTERS_DIRECTORY, &path) == B_OK &&
		path.Append(printerName) == B_OK &&
		folder.SetTo(path.Path()) == B_OK) {
		spoolFolder = &folder;
	}

	PrinterDriverInstance instance(spoolFolder);
	return instance.GetPrinterDriver()->AddPrinter(printerName);
}

BMessage *config_page(BNode *spoolFolder, BMessage *settings)
{
	PrinterDriverInstance instance(spoolFolder);
	return instance.GetPrinterDriver()->ConfigPage(settings);
}

BMessage *config_job(BNode *spoolFolder, BMessage *settings)
{
	PrinterDriverInstance instance(spoolFolder);
	return instance.GetPrinterDriver()->ConfigJob(settings);
}

BMessage *take_job(BFile *printJob, BNode *spoolFolder, BMessage *settings)
{
	PrinterDriverInstance instance(spoolFolder);
	return instance.GetPrinterDriver()->TakeJob(printJob, settings);
}

// main entry if printer driver is launched directly

int main(int argc, char* argv[])
{
	PrinterDriverInstance instance;
	instance.GetPrinterDriver()->About();
	return 0;
}
