/*
 * UIDriver.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef	__UIDRIVER_H
#define	__UIDRIVER_H

class BMessage;
class PrinterData;
class PrinterCap;
class JobData;

class UIDriver {
public:
	UIDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap);
	virtual ~UIDriver();
	BMessage *configPage();
	BMessage *configJob();

protected:
	virtual long doPageSetup(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	virtual long doJobSetup(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);

	UIDriver(const UIDriver &);
	UIDriver &operator = (const UIDriver &);

private:
	BMessage         *__msg;
	PrinterData      *__printer_data;
	const PrinterCap *__printer_cap;
};

#endif	/* __UIDRIVER_H */
