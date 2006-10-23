/*
 * PrinterCap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "PrinterCap.h"
#include "PrinterData.h"

PrinterCap::PrinterCap(const PrinterData *printer_data)
	: fPrinterData(printer_data), 
	fPrinterID(kUnknownPrinter)
{
}

PrinterCap::~PrinterCap()
{
}

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

int PrinterCap::getProtocolClass() const {
	return fPrinterData->getProtocolClass();
}
