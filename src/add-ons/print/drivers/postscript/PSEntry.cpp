/*
 * PSEntry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include "PS.h"
#include "PSCap.h"
#include "PrinterDriver.h"

class PSPrinterDriver : public PrinterDriver
{
public:
	PSPrinterDriver(BNode* printerFolder) : PrinterDriver(printerFolder) {}
	
	const char* GetSignature() const  
	{
		return "application/x-vnd.PS-compatible"; 
	}
	
	const char* GetDriverName() const 
	{ 
		return "PS Compatible"; 
	}
	
	const char* GetVersion() const    
	{ 
		return "0.1"; 
	}
	
	const char* GetCopyright() const  
	{ 
		return "PS driver Copyright © 2003,04 Michael Pfeiffer.\n"; 
	}

	PrinterCap* InstantiatePrinterCap(PrinterData* printerData) 
	{
		return new PSCap(printerData);
	}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings, PrinterData* printerData, PrinterCap* printerCap)
	{
		return new PSDriver(settings, printerData, printerCap);
	}
};

PrinterDriver* instantiate_printer_driver(BNode* printerFolder)
{
	return new PSPrinterDriver(printerFolder);
}
