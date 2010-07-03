/*
 * PSEntry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 * Copyright 2010 Ithamar Adema.
 */


#include <Entry.h>
#include <Path.h>

#include "PrinterDriver.h"
#include "PS.h"
#include "PSCap.h"
#include "PSData.h"
#include "SelectPPDDlg.h"


class PSPrinterDriver : public PrinterDriver {
public:
	PSPrinterDriver(BNode* printerFolder)
	:
	PrinterDriver(printerFolder)
	{
	}

	const char*	GetSignature() const
				{
					return "application/x-vnd.PS-compatible";
				}

	const char*	GetDriverName() const
				{ 
					return "PS compatible";
				}

	const char*	GetVersion() const
				{
					return "0.1";
				}

	const char*	GetCopyright() const
				{
					return "PS driver Copyright © 2003,04 Michael Pfeiffer.\n";
				}

	char*		AddPrinter(char *printerName)
				{
					BPath path;
					if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK
						&& path.Append("ppd") == B_OK
						&& BEntry(path.Path()).Exists()) {
						SelectPPDDlg* dialog =
							new SelectPPDDlg(dynamic_cast<PSData*>
								(GetPrinterData()));
						if (dialog->Go() != B_OK)
							return NULL;
					}
					return printerName;
				}

	PrinterData*	InstantiatePrinterData(BNode* node)
					{
						return new PSData(node);
					}

	PrinterCap*		InstantiatePrinterCap(PrinterData* printerData) 
					{
						return new PSCap(printerData);
					}

	GraphicsDriver*	InstantiateGraphicsDriver(BMessage* settings,
						PrinterData* printerData, PrinterCap* printerCap)
					{
						return new PSDriver(settings, printerData, printerCap);
					}
};


PrinterDriver *
instantiate_printer_driver(BNode* printerFolder)
{
	return new PSPrinterDriver(printerFolder);
}
