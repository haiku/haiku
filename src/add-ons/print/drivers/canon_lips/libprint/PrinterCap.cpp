/*
 * PrinterCap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "PrinterCap.h"

PrinterCap::PrinterCap(const PrinterData *printer_data)
	: __printer_data(printer_data), __printer_id(UNKNOWN_PRINTER)
{
}

PrinterCap::~PrinterCap()
{
}

/*
PrinterCap::PrinterCap(const PrinterCap &printer_cap)
{
	__printer_data = printer_cap.__printer_data;
	__printer_id   = printer_cap.__printer_id;
}

PrinterCap::PrinterCap &operator = (const PrinterCap &printer_cap)
{
	__printer_data = printer_cap.__printer_data;
	__printer_id   = printer_cap.__printer_id;
	return *this;
}
*/

const BaseCap *PrinterCap::getDefaultCap(CAPID id) const
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
