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
						UIDriver(BMessage* message, PrinterData* printerData,
							const PrinterCap* printerCap);
	virtual				~UIDriver();
			BMessage*	ConfigPage();
			BMessage*	ConfigJob();

protected:
						UIDriver(const UIDriver &);

			UIDriver&	operator=(const UIDriver &);

	virtual	status_t	PageSetup(JobData* jobData, PrinterData* printerData,
							const PrinterCap* printerCap);
	virtual	status_t	JobSetup(JobData* jobData, PrinterData* printerData,
							const PrinterCap* printerCap);

private:
	BMessage*			fMsg;
	PrinterData*		fPrinterData;
	const PrinterCap*	fPrinterCap;
};

#endif	/* __UIDRIVER_H */
