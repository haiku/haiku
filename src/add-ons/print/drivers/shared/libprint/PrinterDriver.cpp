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
#include "DbgMsg.h"
#include "Exports.h"
#include "GraphicsDriver.h"
#include "PrinterCap.h"
#include "PrinterData.h"
#include "UIDriver.h"


// Implementation of PrinterDriver

PrinterDriver::PrinterDriver(BNode* spoolFolder)
	: fSpoolFolder(spoolFolder)
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
}

void PrinterDriver::About() 
{
	BString copyright;
	copyright = "libprint Copyright © 1999-2000 Y.Takagi\n";
	copyright << GetCopyright();
	copyright << "All Rights Reserved.";
	
	AboutBox app(GetSignature(), GetDriverName(), GetVersion(), copyright.String());
	app.Run();
}

char* PrinterDriver::AddPrinter(char* printerName)
{
	DBGMSG((">%s: add_printer\n", GetDriverName()));
	DBGMSG(("\tprinter_name: %s\n", printerName));
	DBGMSG(("<%s: add_printer\n", GetDriverName()));
	return printerName;
}

BMessage* PrinterDriver::ConfigPage(BMessage* settings)
{
	DBGMSG((">%s: config_page\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);
	
	BMessage pageSettings(*settings);
	MergeWithPreviousSettings(kAttrPageSettings, &pageSettings);
	PrinterData printerData(fSpoolFolder);
	fPrinterCap = InstantiatePrinterCap(&printerData);
	UIDriver drv(&pageSettings, &printerData, fPrinterCap);
	BMessage *result = drv.configPage();
	WriteSettings(kAttrPageSettings, result);

	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: config_page\n", GetDriverName()));
	return result;
}

BMessage* PrinterDriver::ConfigJob(BMessage* settings)
{
	DBGMSG((">%s: config_job\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);
	
	BMessage jobSettings(*settings);
	MergeWithPreviousSettings(kAttrJobSettings, &jobSettings);
	PrinterData printerData(fSpoolFolder);
	fPrinterCap = InstantiatePrinterCap(&printerData);
	UIDriver drv(&jobSettings, &printerData, fPrinterCap);
	BMessage *result = drv.configJob();
	WriteSettings(kAttrJobSettings, result);
	
	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: config_job\n", GetDriverName()));
	return result;
}

BMessage* PrinterDriver::TakeJob(BFile* printJob, BMessage* settings)
{
	DBGMSG((">%s: take_job\n", GetDriverName()));
	DUMP_BMESSAGE(settings);
	DUMP_BNODE(fSpoolFolder);

	PrinterData printerData(fSpoolFolder);
	fPrinterCap = InstantiatePrinterCap(&printerData);
	fGraphicsDriver = InstantiateGraphicsDriver(settings, &printerData, fPrinterCap);
	BMessage *result = fGraphicsDriver->takeJob(printJob);

	DUMP_BMESSAGE(result);
	DBGMSG(("<%s: take_job\n", GetDriverName()));
	return result;
}

// read settings from spool folder attribute
bool PrinterDriver::ReadSettings(const char* attrName, BMessage* settings)
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
void PrinterDriver::WriteSettings(const char* attrName, BMessage* settings)
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
void PrinterDriver::MergeWithPreviousSettings(const char* attrName, BMessage* settings)
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
}

PrinterDriverInstance::~PrinterDriverInstance()
{
	delete fInstance;
	fInstance = NULL;
}


// printer driver add-on functions

char *add_printer(char *printerName)
{
	PrinterDriverInstance instance;
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
