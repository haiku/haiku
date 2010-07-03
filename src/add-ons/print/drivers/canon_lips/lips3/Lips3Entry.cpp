/*
 * Lips3Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips3.h"
#include "Lips3Cap.h"
#include "PrinterDriver.h"


class Lips3PrinterDriver : public PrinterDriver
{
public:
	Lips3PrinterDriver(BNode* printerFolder) : PrinterDriver(printerFolder) {}
	
	const char* GetSignature() const  
	{
		return "application/x-vnd.lips3-compatible"; 
	}
	
	const char* GetDriverName() const 
	{ 
		return "Canon LIPS3 compatible"; 
	}
	
	const char* GetVersion() const
	{ 
		return "0.9.4"; 
	}
	
	const char* GetCopyright() const  
	{ 
		return "Copyright © 1999-2000 Y.Takagi.\n"; 
	}

	PrinterCap* InstantiatePrinterCap(PrinterData* printerData) 
	{
		return new Lips3Cap(printerData);
	}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
		PrinterData* printerData, PrinterCap* printerCap)
	{
		return new LIPS3Driver(settings, printerData, printerCap);
	}
};


PrinterDriver* instantiate_printer_driver(BNode* printerFolder)
{
	return new Lips3PrinterDriver(printerFolder);
}
