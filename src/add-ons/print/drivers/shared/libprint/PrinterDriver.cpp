/*
 * PrinterDriver.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2004 Michael Pfeiffer.
 */

#include "PrinterDriver.h"

#include <fs_attr.h> // for attr_info
#include <File.h>
#include <Message.h>
#include <Node.h>
#include <String.h>
#include <memory> // for auto_ptr

#include "BeUtils.h"
#include "AboutBox.h"
#include "AddPrinterDlg.h"
#include "DbgMsg.h"
#include "Exports.h"
#include "GraphicsDriver.h"
#include "PrinterCap.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "Preview.h"


// Implementation of PrinterDriver

PrinterDriver::PrinterDriver(BNode* spoolFolder)
	: fSpoolFolder(spoolFolder)
	, fPrinterData(NULL)
	, fPrinterCap(NULL)
	, fGraphicsDriver(NULL)
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

void 
PrinterDriver::InitPrinterDataAndCap() {
	fPrinterData = new PrinterData(fSpoolFolder);
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
	
	if (fPrinterCap->isSupport(PrinterCap::kProtocolClass)) {
		if (fPrinterCap->countCap(PrinterCap::kProtocolClass) > 1) {
			AddPrinterDlg *dialog;
			dialog = new AddPrinterDlg(fPrinterData, fPrinterCap);
			if (dialog->Go() != B_OK) {
				// dialog canceled
				return NULL;
			}
		} else {
			const ProtocolClassCap* pcCap;
			pcCap = (const ProtocolClassCap*)fPrinterCap->getDefaultCap(PrinterCap::kProtocolClass);
			if (pcCap != NULL) {
				fPrinterData->setProtocolClass(pcCap->protocolClass);
				fPrinterData->save();
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
	MergeWithPreviousSettings(kAttrPageSettings, &pageSettings);
	UIDriver drv(&pageSettings, fPrinterData, fPrinterCap);
	BMessage *result = drv.configPage();
	WriteSettings(kAttrPageSettings, result);

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
	MergeWithPreviousSettings(kAttrJobSettings, &jobSettings);
	UIDriver drv(&jobSettings, fPrinterData, fPrinterCap);
	BMessage *result = drv.configJob();
	WriteSettings(kAttrJobSettings, result);
	
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
	const JobData* jobData = fGraphicsDriver->getJobData(printJob);
	if (jobData != NULL && jobData->getShowPreview()) {
		PreviewWindow *preview = new PreviewWindow(printJob, true);
		if (preview->Go() != B_OK) {
			return new BMessage('okok');
		}
	}	
	BMessage *result = fGraphicsDriver->takeJob(printJob);

	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: take_job\n", GetDriverName()));
	return result;
}

// read settings from spool folder attribute
bool 
PrinterDriver::ReadSettings(const char* attrName, BMessage* settings)
{
	attr_info info;
	char*  data;
	ssize_t size;

	settings->MakeEmpty();
	
	if (fSpoolFolder->GetAttrInfo(attrName, &info) == B_OK && info.size > 0) {
		data = new char[info.size];
		auto_ptr<char> _data(data);
		size = fSpoolFolder->ReadAttr(attrName, B_MESSAGE_TYPE, 0, data, info.size);
		if (size == info.size && settings->Unflatten(data) == B_OK) {
			return true;
		}
	}
	return false;
}

// write settings to spool folder attribute
void 
PrinterDriver::WriteSettings(const char* attrName, BMessage* settings)
{
	if (settings == NULL || settings->what != 'okok') return;
	
	size_t size;
	char* data;
	
	size = settings->FlattenedSize();
	data = new char[size];
	auto_ptr<char> _data(data);
	
	if (data != NULL && settings->Flatten(data, size) == B_OK) {
		fSpoolFolder->WriteAttr(attrName, B_MESSAGE_TYPE, 0, data, size);
	}
}

// read settings from spool folder attribute and merge them to current settings
void 
PrinterDriver::MergeWithPreviousSettings(const char* attrName, BMessage* settings)
{
	if (settings == NULL) return;
	
	BMessage stored;
	if (ReadSettings(attrName, &stored)) {
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
