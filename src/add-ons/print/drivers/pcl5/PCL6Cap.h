/*
 * PCL5Cap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PCL5CAP_H
#define __PCL5CAP_H

#include "PrinterCap.h"

class PCL6Cap : public PrinterCap {
public:
	PCL6Cap(const PrinterData *printer_data);
	virtual int countCap(CAPID) const;
	virtual bool isSupport(CAPID) const;
	virtual const BaseCap **enumCap(CAPID) const;
};

#endif	/* __PCL5CAP_H */
