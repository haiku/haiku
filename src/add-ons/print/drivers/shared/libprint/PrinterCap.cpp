/*
 * PrinterCap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "PrinterCap.h"

PrinterCap::PrinterCap(const PrinterData *printer_data)
	: fPrinterData(printer_data), fPrinterID(kUnknownPrinter)
{
}

PrinterCap::~PrinterCap()
{
}

/*
PrinterCap::PrinterCap(const PrinterCap &printer_cap)
{
	fPrinterData = printer_cap.fPrinterData;
	fPrinterID   = printer_cap.fPrinterID;
}

PrinterCap::PrinterCap &operator = (const PrinterCap &printer_cap)
{
	fPrinterData = printer_cap.fPrinterData;
	fPrinterID   = printer_cap.fPrinterID;
	return *this;
}
*/

const BaseCap *PrinterCap::getDefaultCap(CapID id) const
{
	int count = countCap(id);
	if (count > 0) {
		const BaseCap **base_cap = enumCap(id);
		while (count--) {
			if ((*base_cap)->is_default) {
				return *base_cap;
			}
			base_cap++;
		}
	}
	return NULL;
}
