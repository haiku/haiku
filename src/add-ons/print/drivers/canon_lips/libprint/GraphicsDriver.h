/*
 * GraphicsDriver.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef	__GRAPHICSDRIVER_H
#define	__GRAPHICSDRIVER_H

#include "JobData.h"
#include "PrintProcess.h"
#include "SpoolMetaData.h"
#include "Transport.h"

class BView;
class BBitmap;
class BMessage;
class PrinterData;
class PrinterCap;

#define GDF_ROTATE_BAND_BITMAP	0x01

class GraphicsDriver {
public:
	GraphicsDriver(BMessage *, PrinterData *, const PrinterCap *);
	virtual ~GraphicsDriver();
	BMessage *takeJob(BFile *spool, uint32 flags = 0);

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

	int getPageWidth()  const;
	int getPageHeight() const;
	int getBandWidth()  const;
	int getBandHeight() const;
	int getPixelDepth() const;

	GraphicsDriver(const GraphicsDriver &);
	GraphicsDriver &operator = (const GraphicsDriver &);

private:
	void setupData(BFile *file, long page_count);
	void setupBitmap();
	void cleanupData();
	void cleanupBitmap();
	bool printPage(PageDataList *pages);
	bool printDocument(SpoolData *spool_data);
	bool printJob(BFile *file);
	static void rgb32_to_rgb24(void* src, void* dst, int width);
	static void cmap8_to_rgb24(void* src, void* dst, int width);
	static uint8 gray(uint8 r, uint8 g, uint8 b);
	static void rgb32_to_gray(void* src, void* dst, int width);
	static void cmap8_to_gray(void* src, void* dst, int width);

	uint32           __flags;
	BMessage         *__msg;
	BView            *__view;
	BBitmap          *__bitmap;
	Transport        *__transport;
	JobData          *__org_job_data;
	JobData          *__real_job_data;
	PrinterData      *__printer_data;
	const PrinterCap *__printer_cap;
	SpoolMetaData    *__spool_meta_data;

	int __page_width;
	int __page_height;
	int __band_width;
	int __band_height;
	int __pixel_depth;
	int __band_count;
	int __internal_copies;
};

inline const JobData *GraphicsDriver::getJobData() const
{
	return __real_job_data;
}

inline const PrinterData *GraphicsDriver::getPrinterData() const
{
	return __printer_data;
}

inline const PrinterCap *GraphicsDriver::getPrinterCap() const
{
	return __printer_cap;
}

inline const SpoolMetaData *GraphicsDriver::getSpoolMetaData() const
{
	return __spool_meta_data;
}

inline int GraphicsDriver::getPageWidth() const
{
	return __page_width;
}

inline int GraphicsDriver::getPageHeight() const
{
	return __page_height;
}

inline int GraphicsDriver::getBandWidth() const
{
	return __band_width;
}

inline int GraphicsDriver::getBandHeight() const
{
	return __band_height;
}

#endif	/* __GRAPHICSDRIVER_H */
