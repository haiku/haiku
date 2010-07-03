/*
 * Lips4Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips4.h"
#include "Lips4Cap.h"
#include "PrinterDriver.h"


class Lips4PrinterDriver : public PrinterDriver
{
public:
	Lips4PrinterDriver(BNode* printerFolder) : PrinterDriver(printerFolder) {}
	
	const char* GetSignature() const  
	{
		return "application/x-vnd.lips4-compatible"; 
	}
	
	const char* GetDriverName() const 
	{ 
		return "Canon LIPS4 compatible"; 
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
		return new Lips4Cap(printerData);
	}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
		PrinterData* printerData, PrinterCap* printerCap)
	{
		return new LIPS4Driver(settings, printerData, printerCap);
	}
};


PrinterDriver*
instantiate_printer_driver(BNode* printerFolder)
{
	return new Lips4PrinterDriver(printerFolder);
}
