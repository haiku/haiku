/*
 * PCL6Cap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PCL6CAP_H
#define __PCL6CAP_H

#include "PrinterCap.h"

enum ProtocolClass {
	kProtocolClass1_1,
	kProtocolClass2_0,
	kProtocolClass2_1,
	kProtocolClass3_0,
};

class PCL6Cap : public PrinterCap {
public:
	PCL6Cap(const PrinterData *printer_data);
	virtual int countCap(CapID) const;
	virtual bool isSupport(CapID) const;
	virtual const BaseCap **enumCap(CapID) const;
};

#endif	/* __PCL6CAP_H */
