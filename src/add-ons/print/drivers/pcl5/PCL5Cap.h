/*
 * PCL5Cap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PCL5CAP_H
#define __PCL5CAP_H

#include "PrinterCap.h"

class PCL5Cap : public PrinterCap {
public:
	PCL5Cap(const PrinterData *printer_data);
	virtual int countCap(CapID) const;
	virtual bool isSupport(CapID) const;
	virtual const BaseCap **enumCap(CapID) const;
};

#endif	/* __PCL5CAP_H */
