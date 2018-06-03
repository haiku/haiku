/*
* Copyright 2017, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#ifndef LPSTYLCAP_H
#define LPSTYLCAP_H


#include "PrinterCap.h"


class LpstylCap : public PrinterCap {
public:
					LpstylCap(const PrinterData* printer_data)
						: PrinterCap(printer_data) {}
	virtual	int		CountCap(CapID) const;
	virtual	bool	Supports(CapID) const;
	virtual	const	BaseCap** GetCaps(CapID) const;
};

#endif // __LIPS4CAP_H
