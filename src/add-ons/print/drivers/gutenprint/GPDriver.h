/*
 * GP.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */
#ifndef __GP_H
#define __GP_H


#include "GPBinding.h"
#include "GPCapabilities.h"
#include "GraphicsDriver.h"
#include "OutputStream.h"

class Halftone;


class GPDriver : public GraphicsDriver, public OutputStream
{
public:
				GPDriver(BMessage* msg, PrinterData* printer_data,
					const PrinterCap* printer_cap);

	void		Write(const void *buffer, size_t size)
					/* throw(TransportException) */;

protected:
	bool		StartDocument();
	void		SetParameter(BString& parameter, PrinterCap::CapID category,
					int value);
	void		SetDriverSpecificSettings();
	void		AddDriverSpecificSetting(PrinterCap::CapID category,
					const char* key);
	void		AddDriverSpecificBooleanSetting(PrinterCap::CapID category,
					const char* key);
	void		AddDriverSpecificIntSetting(PrinterCap::CapID category,
					const char* key);
	void		AddDriverSpecificDimensionSetting(PrinterCap::CapID category,
					const char* key);
	void		AddDriverSpecificDoubleSetting(PrinterCap::CapID category,
					const char* key);
	bool		StartPage(int page);
	bool		NextBand(BBitmap* bitmap, BPoint* offset);
	bool		EndPage(int page);
	bool		EndDocument(bool success);
	void		ShowError(const char* message);

private:
	GPBinding		fBinding;
	GPJobConfiguration	fConfiguration;
};

#endif // __GP_H
