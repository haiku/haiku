#ifndef _PRINTER_DRIVER_H
#define _PRINTER_DRIVER_H

#include <SupportDefs.h>
 
class BFile;
class BMessage;
class BNode;
class PrinterCap;
class PrinterData;
class GraphicsDriver;

#define kAttrPageSettings "libprint/page_settings"
#define kAttrJobSettings  "libprint/job_settings"

class PrinterDriver {
public:
	PrinterDriver(BNode* spoolFolder);
	virtual ~PrinterDriver();

	virtual const char* GetSignature() const = 0;
	virtual const char* GetDriverName() const = 0;
	virtual const char* GetVersion() const = 0;
	virtual const char* GetCopyright() const = 0;

	virtual PrinterCap* InstantiatePrinterCap(PrinterData* printerData) = 0;
	virtual GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings, PrinterData* printerData, PrinterCap* printerCap) = 0;
	
	virtual void About();
	virtual char* AddPrinter(char* printerName);
	BMessage* ConfigPage(BMessage* settings);
	BMessage* ConfigJob(BMessage* settings);
	BMessage* TakeJob(BFile* printJob, BMessage* settings);

private:
	bool ReadSettings(const char* attrName, BMessage* settings);
	void WriteSettings(const char* attrName, BMessage* settings);
	void MergeWithPreviousSettings(const char* attrName, BMessage* settings);

	BNode*          fSpoolFolder;
	PrinterCap*     fPrinterCap;
	GraphicsDriver* fGraphicsDriver;
};

PrinterDriver* instantiate_printer_driver(BNode* printerFolder = NULL);

#endif
