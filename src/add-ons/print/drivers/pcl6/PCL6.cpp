/*
 * PCL6.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */


#include "PCL6.h"

#include <memory.h>

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>

#include "DbgMsg.h"
#include "DeltaRowCompression.h"
#include "Halftone.h"
#include "JobData.h"
#include "PackBits.h"
#include "PCL6Cap.h"
#include "PCL6Config.h"
#include "PCL6Rasterizer.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "ValidRect.h"


// DeltaRowStreamCompressor writes the delta row directly to the 
// in the contructor specified stream.
class DeltaRowStreamCompressor : public AbstractDeltaRowCompressor
{
public:
				DeltaRowStreamCompressor(int rowSize, uchar initialSeed,
					PCL6Writer* writer)
				:
				AbstractDeltaRowCompressor(rowSize, initialSeed),
				fWriter(writer)
				{}
	
protected:
	void		AppendByteToDeltaRow(uchar byte)
				{
					fWriter->Append(byte);
				}
	
private:
	PCL6Writer*	fWriter;	
};


PCL6Driver::PCL6Driver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap),
	fWriter(NULL),
	fMediaSide(PCL6Writer::kFrontMediaSide),
	fHalftone(NULL)
{
}


void
PCL6Driver::Write(const uint8* data, uint32 size)
{
	WriteSpoolData(data, size);
}


bool
PCL6Driver::StartDocument()
{
	try {
		_JobStart();
		fHalftone = new Halftone(GetJobData()->GetSurfaceType(),
			GetJobData()->GetGamma(), GetJobData()->GetInkDensity(),
			GetJobData()->GetDitherType());
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
PCL6Driver::EndDocument(bool)
{
	try {
		if (fHalftone)
			delete fHalftone;
		_JobEnd();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
PCL6Driver::NextBand(BBitmap* bitmap, BPoint* offset)
{
	DBGMSG(("> nextBand\n"));

	try {
		int y = (int)offset->y;
	
		PCL6Rasterizer* rasterizer;
		if (_UseColorMode()) {
			#if COLOR_DEPTH == 8
				rasterizer = new ColorRGBRasterizer(fHalftone);
			#elif COLOR_DEPTH == 1
				rasterizer = new ColorRasterizer(fHalftone);
			#else
				#error COLOR_DEPTH must be either 1 or 8!		
			#endif
		} else
			rasterizer = new MonochromeRasterizer(fHalftone);

		auto_ptr<Rasterizer> _rasterizer(rasterizer);
		bool valid = rasterizer->SetBitmap((int)offset->x, (int)offset->y,
			bitmap, GetPageHeight());
		
		if (valid) {
			rasterizer->InitializeBuffer();
			
			// Use compressor to calculate delta row size
			DeltaRowCompressor* deltaRowCompressor = NULL;
			if (_SupportsDeltaRowCompression()) {
				deltaRowCompressor = 
					new DeltaRowCompressor(rasterizer->GetOutRowSize(), 0);
				if (deltaRowCompressor->InitCheck() != B_OK) {
					delete deltaRowCompressor;
					return false;
				}
			}
			auto_ptr<DeltaRowCompressor>_deltaRowCompressor(deltaRowCompressor);
			int deltaRowSize = 0;
		
			// remember position		
			int xPage = rasterizer->GetX();
			int yPage = rasterizer->GetY();
			
			while (rasterizer->HasNextLine()) {
				const uchar* rowBuffer = 
					static_cast<const uchar*>(rasterizer->RasterizeNextLine());
				
				if (deltaRowCompressor != NULL) {
					int size =
						deltaRowCompressor->CalculateSize(rowBuffer, true);
					deltaRowSize += size + 2;
						// two bytes for the row byte count
				}
			}
	
			y = rasterizer->GetY();
	
			uchar* outBuffer = rasterizer->GetOutBuffer();
			int outBufferSize = rasterizer->GetOutBufferSize();
			int outRowSize = rasterizer->GetOutRowSize();
			int width = rasterizer->GetWidth();
			int height = rasterizer->GetHeight();
			_WriteBitmap(outBuffer, outBufferSize, outRowSize, xPage, yPage,
				width, height, deltaRowSize);
		}

		if (y >= GetPageHeight()) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else {
			offset->y += bitmap->Bounds().IntegerHeight()+1;
		}

		return true;
	}
	catch (TransportException& err) {
		BAlert* alert = new BAlert("", err.What(), "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return false;
	} 
}


void
PCL6Driver::_WriteBitmap(const uchar* buffer, int outSize, int rowSize, int x,
	int y, int width, int height, int deltaRowSize)
{
	// choose the best compression method
	PCL6Writer::Compression compressionMethod = PCL6Writer::kNoCompression;
	int dataSize = outSize;

#if ENABLE_DELTA_ROW_COMPRESSION
	if (_SupportsDeltaRowCompression() && deltaRowSize < dataSize) {
		compressionMethod = PCL6Writer::kDeltaRowCompression;
		dataSize = deltaRowSize;
	}
#endif

#if ENABLE_RLE_COMPRESSION
	if (_SupportsRLECompression()) {
		int rleSize = pack_bits_size(buffer, outSize);
		if (rleSize < dataSize) {
			compressionMethod = PCL6Writer::kRLECompression;
			dataSize = rleSize;
		}
	}
#endif
	
	// write bitmap
	_Move(x, y);
	
	_StartRasterGraphics(x, y, width, height, compressionMethod);

	_RasterGraphics(buffer, outSize, dataSize, rowSize, height,
		compressionMethod);

	_EndRasterGraphics();
	
#if DISPLAY_COMPRESSION_STATISTICS
	fprintf(stderr, "Out Size       %d %2.2f\n", (int)outSize, 100.0);
#if ENABLE_RLE_COMPRESSION
	fprintf(stderr, "RLE Size       %d %2.2f\n", (int)rleSize,
		100.0 * rleSize / outSize);
#endif
#if ENABLE_DELTA_ROW_COMPRESSION
	fprintf(stderr, "Delta Row Size %d %2.2f\n", (int)deltaRowSize,
		100.0 * deltaRowSize / outSize);
#endif
	fprintf(stderr, "Data Size      %d %2.2f\n", (int)dataSize,
		100.0 * dataSize / outSize);
#endif
}


void
PCL6Driver::_JobStart()
{
	// PCL6 begin
	fWriter = new PCL6Writer(this);
	PCL6Writer::ProtocolClass pc =
		(PCL6Writer::ProtocolClass)GetProtocolClass();
	fWriter->PJLHeader(pc, GetJobData()->GetXres(),
		"Copyright (c) 2003 - 2010 Haiku");
	fWriter->BeginSession(GetJobData()->GetXres(), GetJobData()->GetYres(),
		PCL6Writer::kInch, PCL6Writer::kBackChAndErrPage);
	fWriter->OpenDataSource();
	fMediaSide = PCL6Writer::kFrontMediaSide;
}


bool
PCL6Driver::StartPage(int)
{
	PCL6Writer::Orientation orientation = PCL6Writer::kPortrait;
	if (GetJobData()->GetOrientation() == JobData::kLandscape) {
		orientation = PCL6Writer::kLandscape;
	}
	
	PCL6Writer::MediaSize mediaSize = 
		_MediaSize(GetJobData()->GetPaper());
	PCL6Writer::MediaSource mediaSource = 
		_MediaSource(GetJobData()->GetPaperSource());
	if (GetJobData()->GetPrintStyle() == JobData::kSimplex) {
		fWriter->BeginPage(orientation, mediaSize, mediaSource);
	} else if (GetJobData()->GetPrintStyle() == JobData::kDuplex) {
		// TODO move duplex binding option to UI
		fWriter->BeginPage(orientation, mediaSize, mediaSource, 
			PCL6Writer::kDuplexVerticalBinding, fMediaSide);

		if (fMediaSide == PCL6Writer::kFrontMediaSide)
			fMediaSide = PCL6Writer::kBackMediaSide;
		else
			fMediaSide = PCL6Writer::kFrontMediaSide;
	} else
		return false;
	
	// PageOrigin from Windows NT printer driver
	int x = 142 * GetJobData()->GetXres() / 600;
	int y = 100 * GetJobData()->GetYres() / 600;
	fWriter->SetPageOrigin(x, y);
	fWriter->SetColorSpace(_UseColorMode() ? PCL6Writer::kRGB
		: PCL6Writer::kGray);
	fWriter->SetPaintTxMode(PCL6Writer::kOpaque);
	fWriter->SetSourceTxMode(PCL6Writer::kOpaque);
	fWriter->SetROP(204);
	return true;
}


void
PCL6Driver::_StartRasterGraphics(int x, int y, int width, int height,
	PCL6Writer::Compression compressionMethod)
{
	PCL6Writer::ColorDepth colorDepth;
	if (_UseColorMode()) {
		#if COLOR_DEPTH == 8
			colorDepth = PCL6Writer::k8Bit;
		#elif COLOR_DEPTH == 1
			colorDepth = PCL6Writer::k1Bit;
		#else
			#error COLOR_DEPTH must be either 1 or 8!		
		#endif
	} else
		colorDepth = PCL6Writer::k1Bit;

	fWriter->BeginImage(PCL6Writer::kDirectPixel, colorDepth, width, height,
		width, height);
	fWriter->ReadImage(compressionMethod, 0, height);
}


void
PCL6Driver::_EndRasterGraphics()
{
	fWriter->EndImage();
}


void
PCL6Driver::_RasterGraphics(const uchar* buffer, int bufferSize, int dataSize,
	int rowSize, int height, int compressionMethod)
{
	// write bitmap byte size
	fWriter->EmbeddedDataPrefix32(dataSize);
	
	// write data
	if (compressionMethod == PCL6Writer::kRLECompression) {
		// use RLE compression
		uchar* outBuffer = new uchar[dataSize];
		pack_bits(outBuffer, buffer, bufferSize);
		fWriter->Append(outBuffer, dataSize);
		delete[] outBuffer;
		return;
	} else if (compressionMethod == PCL6Writer::kDeltaRowCompression) {
		// use delta row compression
		DeltaRowStreamCompressor compressor(rowSize, 0, fWriter);
		if (compressor.InitCheck() != B_OK) {
			return;
		}
		
		const uint8* row = buffer;
		for (int i = 0; i < height; i ++) {
			// write row byte count
			int32 size = compressor.CalculateSize(row);
			fWriter->Append((uint16)size);
			
			if (size > 0) {
				// write delta row
				compressor.Compress(row);
			}
			
			row += rowSize;
		}
	} else {
		// write raw data
		fWriter->Append(buffer, bufferSize);
	}
}


bool
PCL6Driver::EndPage(int)
{
	try {
		fWriter->EndPage(GetJobData()->GetCopies());
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


void
PCL6Driver::_JobEnd()
{
	fWriter->CloseDataSource();
	fWriter->EndSession();
	fWriter->PJLFooter();
	fWriter->Flush();
	delete fWriter;
	fWriter = NULL;
}


void
PCL6Driver::_Move(int x, int y)
{
	fWriter->SetCursor(x, y);
}


bool
PCL6Driver::_SupportsRLECompression()
{
	return GetJobData()->GetColor() != JobData::kColorCompressionDisabled;
}


bool
PCL6Driver::_SupportsDeltaRowCompression()
{
	return GetProtocolClass() >= PCL6Writer::kProtocolClass2_1
		&& GetJobData()->GetColor() != JobData::kColorCompressionDisabled;
}


bool
PCL6Driver::_UseColorMode()
{
	return GetJobData()->GetColor() != JobData::kMonochrome;
}


PCL6Writer::MediaSize
PCL6Driver::_MediaSize(JobData::Paper paper)
{
	switch (paper) {
		case JobData::kLetter:
			return PCL6Writer::kLetterPaper;
		case JobData::kLegal:
			return PCL6Writer::kLegalPaper;
		case JobData::kA4:
			return PCL6Writer::kA4Paper;
		case JobData::kExecutive:
			return PCL6Writer::kExecPaper;
		case JobData::kLedger:
			return PCL6Writer::kLedgerPaper;
		case JobData::kA3:
			return PCL6Writer::kA3Paper;
		case JobData::kB5:
			return PCL6Writer::kB5Paper;
		case JobData::kJapanesePostcard:
			return PCL6Writer::kJPostcard;
		case JobData::kA5:
			return PCL6Writer::kA5Paper;
		case JobData::kB4:
			return PCL6Writer::kJB4Paper;
/*
		case : return PCL6Writer::kCOM10Envelope;
		case : return PCL6Writer::kMonarchEnvelope;
		case : return PCL6Writer::kC5Envelope;
		case : return PCL6Writer::kDLEnvelope;
		case : return PCL6Writer::kJB4Paper;
		case : return PCL6Writer::kJB5Paper;
		case : return PCL6Writer::kB5Envelope;
		case : return PCL6Writer::kJPostcard;
		case : return PCL6Writer::kJDoublePostcard;
		case : return PCL6Writer::kA5Paper;
		case : return PCL6Writer::kA6Paper;
		case : return PCL6Writer::kJB6Paper;
		case : return PCL6Writer::kJIS8KPaper;
		case : return PCL6Writer::kJIS16KPaper;
		case : return PCL6Writer::kJISExecPaper;
*/
		default:
			return PCL6Writer::kLegalPaper;
	}
}


PCL6Writer::MediaSource
PCL6Driver::_MediaSource(JobData::PaperSource source)
{
	switch (source) {
		case JobData::kAuto:
			return PCL6Writer::kAutoSelect;
		case JobData::kCassette1:
			return PCL6Writer::kDefaultSource;
		case JobData::kCassette2:
			return PCL6Writer::kEnvelopeTray;
		case JobData::kLower:
			return PCL6Writer::kLowerCassette;
		case JobData::kUpper:
			return PCL6Writer::kUpperCassette;
		case JobData::kMiddle:
			return PCL6Writer::kThirdCassette;
		case JobData::kManual:
			return PCL6Writer::kManualFeed;
		case JobData::kCassette3:
			return PCL6Writer::kMultiPurposeTray;
		default:
			return PCL6Writer::kAutoSelect;
	}
}
