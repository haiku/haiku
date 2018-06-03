/*
* Copyright 2017, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#include "Lpstyl.h"
#include "LpstylCap.h"
#include "LpstylData.h"
#include "PrinterDriver.h"


class LpstylPrinterDriver : public PrinterDriver
{
	public:
		LpstylPrinterDriver(BNode* printerFolder)
			: PrinterDriver(printerFolder)
		{}

		const char* GetSignature() const
		{
			return "application/x-vnd.lpstyl";
		}

		const char* GetDriverName() const
		{
			return "Apple StyleWriter";
		}

		const char* GetVersion() const
		{
			return "1.0.0";
		}

		const char* GetCopyright() const
		{
			return "Copyright 1996-2000 Monroe Williams, 2017 Adrien Destugues.\n";
		}

		PrinterData* InstantiatePrinterData(BNode* node)
		{
			return new LpstylData(node);
		}

		PrinterCap* InstantiatePrinterCap(PrinterData* printerData)
		{
			return new LpstylCap(printerData);
		}

		GraphicsDriver* InstantiateGraphicsDriver(BMessage* settings,
			PrinterData* printerData, PrinterCap* printerCap)
		{
			return new LpstylDriver(settings, printerData, printerCap);
		}
};

PrinterDriver*
instantiate_printer_driver(BNode* printerFolder)
{
	return new LpstylPrinterDriver(printerFolder);
}
