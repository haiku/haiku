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


class GraphicsDriver {
public:
				GraphicsDriver(BMessage* message, PrinterData* printerData,
					const PrinterCap *printerCap);
	virtual		~GraphicsDriver();

	const JobData*	GetJobData(BFile* spoolFile);
	BMessage*		TakeJob(BFile* spoolFile);
	static BPoint	GetScale(int32 nup, BRect physicalRect, float scaling);
	static BPoint	GetOffset(int32 nup, int index,
						JobData::Orientation orientation, const BPoint* scale,
						BRect scaledPhysicalRect, BRect scaledPrintableRect,
						BRect physicalRect);

protected:
					GraphicsDriver(const GraphicsDriver &);

			GraphicsDriver&	operator = (const GraphicsDriver &);

	virtual	bool	StartDocument();
	virtual	bool	StartPage(int page_number);
	virtual	bool	NextBand(BBitmap* bitmap, BPoint* offset);
	virtual	bool	EndPage(int page_number);
	virtual	bool	EndDocument(bool success);

			void	WriteSpoolData(const void* buffer, size_t size)
						/* throw (TransportException) */;
			void	WriteSpoolString(const char* buffer, ...)
						/* throw (TransportException) */;
			void	WriteSpoolChar(char c)
						/* throw (TransportException) */;

			void	ReadSpoolData(void* buffer, size_t size)
						/* throw (TransportException) */;
			int		ReadSpoolChar()
						/* throw (TransportException) */;

	static	void	ConvertToRGB24(const void* src, void* dst, int width,
						color_space cs);
	static	void	ConvertToGray(const void* src, void* dst, int width,
						color_space cs);

			const JobData*			GetJobData() const;
			const PrinterData*		GetPrinterData() const;
			const PrinterCap*		GetPrinterCap() const;
			const SpoolMetaData*	GetSpoolMetaData() const;
			int						GetProtocolClass() const;

			int		GetPageHeight() const;

private:
			bool	_SetupData(BFile* file);
			void	_SetupBitmap();
			void	_CleanupData();
			void	_CleanupBitmap();
			bool	_PrintPage(PageDataList* pages);
			bool	_PrintBand(BBitmap* band, BPoint* offset);
			void	_RotateInto(BBitmap* target, const BBitmap* source);
			bool	_CollectPages(SpoolData* spoolData, PageDataList* pages);
			bool	_SkipPages(SpoolData* spoolData);
			bool	_PrintDocument(SpoolData* spoolData);
			bool	_PrintJob(BFile* file);

			bool	_NeedRotateBitmapBand() const;

	static	void	_ConvertRGB32ToRGB24(const void* src, void* dst, int width);
	static	void	_ConvertCMAP8ToRGB24(const void* src, void* dst, int width);
	static	uint8	_ConvertToGray(uint8 r, uint8 g, uint8 b);
	static	void	_ConvertRGB32ToGray(const void* src, void* dst, int width);
	static	void	_ConvertCMAP8ToGray(const void* src, void* dst, int width);

	BMessage*			fMessage;
	BView*				fView;
	BBitmap*			fBitmap;
	BBitmap*			fRotatedBitmap;
	Transport*			fTransport;
	JobData*			fOrgJobData;
	JobData*			fRealJobData;
	PrinterData*		fPrinterData;
	const PrinterCap*	fPrinterCap;
	SpoolMetaData*		fSpoolMetaData;

	int	fPageWidth;
	int	fPageHeight;
	int	fBandWidth;
	int	fBandHeight;
	int	fPixelDepth;
	int	fBandCount;
	int	fInternalCopies;

	uint32	fPageCount;
	
	StatusWindow*	fStatusWindow;
};


inline const JobData*
GraphicsDriver::GetJobData() const
{
	return fRealJobData;
}


inline const PrinterData*
GraphicsDriver::GetPrinterData() const
{
	return fPrinterData;
}


inline const PrinterCap*
GraphicsDriver::GetPrinterCap() const
{
	return fPrinterCap;
}


inline const SpoolMetaData*
GraphicsDriver::GetSpoolMetaData() const
{
	return fSpoolMetaData;
}


inline int
GraphicsDriver::GetProtocolClass() const
{
	return fPrinterData->GetProtocolClass();
}


inline int
GraphicsDriver::GetPageHeight() const
{
	if (!_NeedRotateBitmapBand())
		return fPageHeight;
	return fPageWidth;
}

#endif	/* __GRAPHICSDRIVER_H */
