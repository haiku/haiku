/*
 * GraphicsDriver.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef	__GRAPHICSDRIVER_H
#define	__GRAPHICSDRIVER_H

#include "JobData.h"
#include "PrinterData.h"
#include "PrintProcess.h"
#include "SpoolMetaData.h"
#include "Transport.h"
#include "StatusWindow.h"

class BView;
class BBitmap;
class BMessage;
class PrinterData;
class PrinterCap;

enum {
	kGDFRotateBandBitmap = 1
};

class GraphicsDriver {
public:
	GraphicsDriver(BMessage *, PrinterData *, const PrinterCap *);
	virtual ~GraphicsDriver();
	const JobData *getJobData(BFile *spoolFile);
	BMessage *takeJob(BFile *spool, uint32 flags = 0);
	static BPoint getScale(int32 nup, BRect physicalRect, float scaling);
	static BPoint getOffset(int32 nup, int index, JobData::Orientation orientation, const BPoint *scale, 
		BRect scaledPhysicalRect, BRect scaledPrintableRect, BRect physicalRect);

protected:
	virtual bool startDoc();
	virtual bool startPage(int page_number);
	virtual bool nextBand(BBitmap *bitmap, BPoint *offset);
	virtual bool endPage(int page_number);
	virtual bool endDoc(bool success);

	void writeSpoolData(const void *buffer, size_t size) throw(TransportException);
	void writeSpoolString(const char *buffer, ...) throw(TransportException);
	void writeSpoolChar(char c) throw(TransportException);

	static void convert_to_rgb24(void* src, void* dst, int width, color_space cs);
	static void convert_to_gray(void* src, void* dst, int width, color_space cs);

	const JobData *getJobData() const;
	const PrinterData *getPrinterData() const;
	const PrinterCap *getPrinterCap() const;
	const SpoolMetaData *getSpoolMetaData() const;
	int getProtocolClass() const;

	int getPageWidth()  const;
	int getPageHeight() const;
	int getBandWidth()  const;
	int getBandHeight() const;
	int getPixelDepth() const;

	GraphicsDriver(const GraphicsDriver &);
	GraphicsDriver &operator = (const GraphicsDriver &);

private:
	bool setupData(BFile *file);
	void setupBitmap();
	void cleanupData();
	void cleanupBitmap();
	bool printPage(PageDataList *pages);
	bool collectPages(SpoolData *spool_data, PageDataList *pages);
	bool skipPages(SpoolData *spool_data);
	bool printDocument(SpoolData *spool_data);
	bool printJob(BFile *file);
	static void rgb32_to_rgb24(void* src, void* dst, int width);
	static void cmap8_to_rgb24(void* src, void* dst, int width);
	static uint8 gray(uint8 r, uint8 g, uint8 b);
	static void rgb32_to_gray(void* src, void* dst, int width);
	static void cmap8_to_gray(void* src, void* dst, int width);

	uint32           fFlags;
	BMessage         *fMsg;
	BView            *fView;
	BBitmap          *fBitmap;
	Transport        *fTransport;
	JobData          *fOrgJobData;
	JobData          *fRealJobData;
	PrinterData      *fPrinterData;
	const PrinterCap *fPrinterCap;
	SpoolMetaData    *fSpoolMetaData;

	int fPageWidth;
	int fPageHeight;
	int fBandWidth;
	int fBandHeight;
	int fPixelDepth;
	int fBandCount;
	int fInternalCopies;
	
	uint32 fPageCount;
	StatusWindow *fStatusWindow;
};

inline const JobData *GraphicsDriver::getJobData() const
{
	return fRealJobData;
}

inline const PrinterData *GraphicsDriver::getPrinterData() const
{
	return fPrinterData;
}

inline const PrinterCap *GraphicsDriver::getPrinterCap() const
{
	return fPrinterCap;
}

inline const SpoolMetaData *GraphicsDriver::getSpoolMetaData() const
{
	return fSpoolMetaData;
}

inline int GraphicsDriver::getProtocolClass() const
{
	return fPrinterData->getProtocolClass();
}

inline int GraphicsDriver::getPageWidth() const
{
	return fPageWidth;
}

inline int GraphicsDriver::getPageHeight() const
{
	return fPageHeight;
}

inline int GraphicsDriver::getBandWidth() const
{
	return fBandWidth;
}

inline int GraphicsDriver::getBandHeight() const
{
	return fBandHeight;
}

#endif	/* __GRAPHICSDRIVER_H */
