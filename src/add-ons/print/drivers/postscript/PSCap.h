/*
 * PSCap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */
#ifndef __PSCAP_H
#define __PSCAP_H


#include "PrinterCap.h"


class PSCap : public PrinterCap {
public:
					PSCap(const PrinterData* printer_data);
	virtual	int		CountCap(CapID) const;
	virtual bool	Supports(CapID) const;
	virtual const	BaseCap **GetCaps(CapID) const;
};

#endif // __PSCAP_H
