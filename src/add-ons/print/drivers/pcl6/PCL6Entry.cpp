/*
 * PCL6Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */


#include "PCL6.h"
#include "PCL6Cap.h"
#include "PrinterDriver.h"


class PCL6PrinterDriver : public PrinterDriver
{
public:
	PCL6PrinterDriver(BNode* printerFolder)
	:
	PrinterDriver(printerFolder)
	{}
	
	const char* GetSignature() const  
	{
		return "application/x-vnd.PCL6-compatible"; 
	}
	
	const char* GetDriverName() const 
	{ 
		return "PCL6 compatible"; 
	}
	
	const char* GetVersion() const
	{ 
		return "0.2"; 
	}
	
	const char* GetCopyright() const  
	{ 
		return "PCL6 driver Copyright © 2003,04 Michael Pfeiffer.\n"; 
	}

	PrinterCap* InstantiatePrinterCap(PrinterData* printerData) 
	{
		return new PCL6Cap(printerData);
	}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
		PrinterData* printerData, PrinterCap* printerCap)
	{
		return new PCL6Driver(settings, printerData, printerCap);
	}
};


PrinterDriver*
instantiate_printer_driver(BNode* printerFolder)
{
	return new PCL6PrinterDriver(printerFolder);
}
