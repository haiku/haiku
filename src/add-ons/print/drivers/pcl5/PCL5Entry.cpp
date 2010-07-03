/*
 * PCL5Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */


#include "PCL5.h"
#include "PCL5Cap.h"
#include "PrinterDriver.h"


class PCL5PrinterDriver : public PrinterDriver
{
public:
	PCL5PrinterDriver(BNode* printerFolder) : PrinterDriver(printerFolder) {}
	
	const char* GetSignature() const  
	{
		return "application/x-vnd.PCL5-compatible"; 
	}
	
	const char* GetDriverName() const 
	{ 
		return "PCL5 compatible"; 
	}
	
	const char* GetVersion() const
	{ 
		return "1.0"; 
	}
	
	const char* GetCopyright() const  
	{ 
		return "PCL5 driver Copyright © 2003,04 Michael Pfeiffer.\n"; 
	}

	PrinterCap* InstantiatePrinterCap(PrinterData* printerData) 
	{
		return new PCL5Cap(printerData);
	}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
		PrinterData* printerData, PrinterCap* printerCap)
	{
		return new PCL5Driver(settings, printerData, printerCap);
	}
};


PrinterDriver* instantiate_printer_driver(BNode* printerFolder)
{
	return new PCL5PrinterDriver(printerFolder);
}
