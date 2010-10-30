/*
 * GPEntry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2010 Michael Pfeiffer.
 */


#include <Debug.h>

#include "GPDriver.h"
#include "GPCapabilities.h"
#include "GPData.h"
#include "PrinterDriver.h"
#include "SelectPrinterDialog.h"


class GPPrinterDriver : public PrinterDriver
{
public:
	GPPrinterDriver(BNode* printerFolder)
		:
		PrinterDriver(printerFolder)
		{
		}

	const char*	GetSignature() const
				{
					return "application/x-vnd.gutenprint";
				}
	
	const char*	GetDriverName() const
				{
					return "Gutenprint";
				}
	
	const char*	GetVersion() const
				{
					return "1.0";
				}
	
	const char*	GetCopyright() const
				{
					return "Gutenprint driver "
						"Copyright © 2010 Michael Pfeiffer.\n";
				}

	char* 		AddPrinter(char *printerName)
				{
					GPData* data = dynamic_cast<GPData*>(GetPrinterData());
					ASSERT(data != NULL);

					SelectPrinterDialog* dialog =
						new SelectPrinterDialog(data);

					if (dialog->Go() != B_OK)
						return NULL;

					return printerName;
				}

	PrinterData*	InstantiatePrinterData(BNode* node)
				{
					return new GPData(node);
				}

	PrinterCap* InstantiatePrinterCap(PrinterData* printerData) 
				{
					return new GPCapabilities(printerData);
				}
	
	GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
		PrinterData* printerData, PrinterCap* printerCap)
				{
					return new GPDriver(settings, printerData, printerCap);
				}
};


PrinterDriver* instantiate_printer_driver(BNode* printerFolder)
{
	return new GPPrinterDriver(printerFolder);
}
