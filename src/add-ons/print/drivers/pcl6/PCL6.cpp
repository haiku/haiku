/*
 * PCL6.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include <Alert.h>
#include <Bitmap.h>
#include <File.h>
#include <memory>
#include "PCL6.h"
#include "UIDriver.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PCL6Cap.h"
#include "PackBits.h"
#include "Halftone.h"
#include "ValidRect.h"
#include "DbgMsg.h"
#include "DeltaRowCompression.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

// Compression method configuration
// Set to 0 to disable compression and to 1 to enable compression
// Note compression takes place only if it takes less space than uncompressed data!

// Run-Length-Encoding Compression
// DO NOT ENABLE HP_M2TIFF_Compress seems to be broken!!!
#define ENABLE_RLE_COMPRESSION       0 

// Delta Row Compression
#define ENABLE_DELTA_ROW_COMPRESSION 0

// DeltaRowStreamCompressor writes the delta row directly to the 
// in the contructor specified stream.
class DeltaRowStreamCompressor : public AbstractDeltaRowCompressor
{
public:
	DeltaRowStreamCompressor(int rowSize, uchar initialSeed, PCL6Writer *writer)
		: AbstractDeltaRowCompressor(rowSize, initialSeed)
		, fWriter(writer)
	{
		// nothing to do
	}
	
protected:
	void AppendByteToDeltaRow(uchar byte) {
		fWriter->Append(byte);
	}
	
private:
	PCL6Writer *fWriter;	
};

PCL6Driver::PCL6Driver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: GraphicsDriver(msg, printer_data, printer_cap)
{
	fHalftone = NULL;
	fWriter = NULL;
}

void PCL6Driver::writeData(const uint8 *data, uint32 size)
{
	writeSpoolData(data, size);
}

bool PCL6Driver::startDoc()
{
	try {
		jobStart();
		fHalftone = new Halftone(getJobData()->getSurfaceType(), getJobData()->getGamma(), getJobData()->getInkDensity(), getJobData()->getDitherType());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PCL6Driver::endDoc(bool)
{
	try {
		if (fHalftone) {
			delete fHalftone;
		}
		jobEnd();
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

bool PCL6Driver::nextBand(BBitmap *bitmap, BPoint *offset)
{
	DBGMSG(("> nextBand\n"));

	try {
		BRect bounds = bitmap->Bounds();

		RECT rc;
		rc.left   = (int)bounds.left;
		rc.top    = (int)bounds.top;
		rc.right  = (int)bounds.right;
		rc.bottom = (int)bounds.bottom;

		int height = rc.bottom - rc.top + 1;

		int x = (int)offset->x;
		int y = (int)offset->y;

		int page_height = getPageHeight();

		if (y + height > page_height) {
			height = page_height - y;
		}

		rc.bottom = height - 1;

		DBGMSG(("height = %d\n", height));
		DBGMSG(("x = %d\n", x));
		DBGMSG(("y = %d\n", y));

		if (get_valid_rect(bitmap, &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;

			bool color;
			int width;
			int widthByte;
			int padBytes;
			int out_row_length;
			int height;
			int out_size;
			int delta;

			color = getJobData()->getColor() == JobData::kColor;

			width = rc.right - rc.left + 1;
			height = rc.bottom - rc.top + 1;
			delta = bitmap->BytesPerRow();

			if (color) {
				widthByte = 3 * width;
			} else {
				widthByte = (width + 7) / 8;	/* byte boundary */
			}

			out_row_length = 4*((widthByte+3)/4);
			padBytes = out_row_length - widthByte; /* line length is a multiple of 4 bytes */
			out_size  = out_row_length * height;


			DBGMSG(("width = %d\n", width));
			DBGMSG(("widthByte = %d\n", widthByte));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("out_size = %d\n", out_size));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->get_pixel_depth() = %d\n", fHalftone->getPixelDepth()));

			uchar *ptr = (uchar *)bitmap->Bits()
						+ rc.top * delta
						+ (rc.left * fHalftone->getPixelDepth()) / 8;

			const uchar *buffer;

			uchar *out_buffer = new uchar[out_size]; 

			uchar *out_ptr = out_buffer;

			auto_ptr<uchar> _out_buffer(out_buffer);

			DBGMSG(("move\n"));

			buffer = out_buffer;

			int xPage = x;
			int yPage = y;
			
			DeltaRowCompressor deltaRowCompressor(out_row_length, 0);
			if (deltaRowCompressor.InitCheck() != B_OK) {
				return false;
			}
			
			int deltaRowSize = 0;
			
			// dither entire band into out_buffer
			for (int i = rc.top; i <= rc.bottom; i++) {
				uchar* out = out_ptr;
				if (color) {
					uchar* in = ptr;
					for (int w = width; w > 0; w --) {
						*out++ = in[2];
						*out++ = in[1];
						*out++ = in[0];
						in += 4;
					}
				} else {
					fHalftone->dither(out_ptr, ptr, x, y, width);
					// invert pixels
					for (int w = widthByte; w > 0; w --, out ++) {
						*out = ~*out;
					}
				}
				// pad with 0s
				for (int w = padBytes; w > 0; w --, out ++) {
					*out = 0;
				}

				{
					int size = deltaRowCompressor.CalculateSize(out_ptr, true);
					deltaRowSize += size + 2; // two bytes for the row byte count
				}

				ptr  += delta;
				out_ptr += out_row_length;
				y++;
			}

			writeBitmap(buffer, out_size, out_row_length, xPage, yPage, width, height, deltaRowSize);

		} else {
			DBGMSG(("band bitmap is clean.\n"));
		}

		if (y >= page_height) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else {
			offset->y += height;
		}

		DBGMSG(("< nextBand\n"));
		return true;
	}
	catch (TransportException &err) {
		BAlert *alert = new BAlert("", err.what(), "OK");
		alert->Go();
		return false;
	} 
}

void PCL6Driver::writeBitmap(const uchar* buffer, int outSize, int rowSize, int x, int y, int width, int height, int deltaRowSize)
{
	// choose the best compression method
	PCL6Writer::Compression compressionMethod = PCL6Writer::kNoCompression;
	int dataSize = outSize;

#if ENABLE_DELTA_ROW_COMPRESSION
	if (deltaRowSize < dataSize) {
		compressionMethod = PCL6Writer::kDeltaRowCompression;
		dataSize = deltaRowSize;
	}
#endif

#if ENABLE_RLE_COMPRESSION
	HP_UInt32 compressedSize = 0;
	HP_M2TIFF_CalcCompression((HP_pCharHuge)buffer, outSize, &compressedSize);
	if (compressedSize < (uint32)dataSize) {
		compressionMethod = PCL6Writer::kRLECompression;
		dataSize = compressedSize;
	}
#endif
	
	// write bitmap
	move(x, y);
	
	startRasterGraphics(x, y, width, height, compressionMethod);

	rasterGraphics(buffer, outSize, dataSize, rowSize, height, compressionMethod);

	endRasterGraphics();
	
#if 1
	fprintf(stderr, "Out Size       %d %2.2f\n", (int)outSize, 100.0);
#if ENABLE_RLE_COMPRESSION
	fprintf(stderr, "RLE Size       %d %2.2f\n", (int)compressedSize, 100.0 * compressedSize / outSize);
#endif
#if ENABLE_DELTA_ROW_COMPRESSION
	fprintf(stderr, "Delta Row Size %d %2.2f\n", (int)deltaRowSize, 100.0 * deltaRowSize / outSize);
#endif
	fprintf(stderr, "Data Size      %d %2.2f\n", (int)dataSize, 100.0 * dataSize / outSize);
#endif
}


void PCL6Driver::jobStart()
{
	// PJL header
	writeSpoolString("\033%%-12345X@PJL JOB\n"
					 "@PJL SET RESOLUTION=%d\n"
	                 "@PJL ENTER LANGUAGE=PCLXL\n"
	                 ") HP-PCL XL;1;1;"
	                 "Comment Copyright (c) 2003, 2004 Haiku\n",
	                 getJobData()->getXres());
	// PCL6 begin
	fWriter = new PCL6Writer(this);
	fWriter->BeginSession(getJobData()->getXres(), getJobData()->getYres(), PCL6Writer::kInch, PCL6Writer::kBackChAndErrPage);
	fWriter->OpenDataSource();
	fMediaSide = PCL6Writer::kFrontMediaSide;
}

bool PCL6Driver::startPage(int)
{
	PCL6Writer::Orientation orientation = PCL6Writer::kPortrait;
	if (getJobData()->getOrientation() == JobData::kLandscape) {
		orientation = PCL6Writer::kLandscape;
	}
	
	PCL6Writer::MediaSize mediaSize = PCL6Driver::mediaSize(getJobData()->getPaper());
	PCL6Writer::MediaSource mediaSource = PCL6Driver::mediaSource(getJobData()->getPaperSource());
	if (getJobData()->getPrintStyle() == JobData::kSimplex) {
		fWriter->BeginPage(orientation, mediaSize, mediaSource);
	} else if (getJobData()->getPrintStyle() == JobData::kDuplex) {
		fWriter->BeginPage(orientation, mediaSize, mediaSource, 
			PCL6Writer::kDuplexHorizontalBinding, fMediaSide);

		if (fMediaSide == PCL6Writer::kFrontMediaSide) {
			fMediaSide = PCL6Writer::kBackMediaSide;
		} else {
			fMediaSide = PCL6Writer::kFrontMediaSide;
		}
	} else {
		return false;
	}
	
	// PageOrigin from Windows NT printer driver
	int x = 142 * getJobData()->getXres() / 600;
	int y = 100 * getJobData()->getYres() / 600;
	bool color = getJobData()->getColor() == JobData::kColor;
	fWriter->SetPageOrigin(x, y);
	fWriter->SetColorSpace(color ? PCL6Writer::kRGB : PCL6Writer::kGray);
	fWriter->SetPaintTxMode(PCL6Writer::kOpaque);
	fWriter->SetSourceTxMode(PCL6Writer::kOpaque);
	fWriter->SetROP(204);
	return true;
}

void PCL6Driver::startRasterGraphics(int x, int y, int width, int height, PCL6Writer::Compression compressionMethod)
{
	bool color = getJobData()->getColor() == JobData::kColor;
	fWriter->BeginImage(PCL6Writer::kDirectPixel, color ? PCL6Writer::k8Bit : PCL6Writer::k1Bit, width, height, width, height);
	fWriter->ReadImage(compressionMethod, 0, height);
}

void PCL6Driver::endRasterGraphics()
{
	fWriter->EndImage();
}

void PCL6Driver::rasterGraphics(
	const uchar *buffer,
	int bufferSize,
	int dataSize,
	int rowSize,
	int height,
	int compressionMethod
)
{
	// write bitmap byte size
	fWriter->EmbeddedDataPrefix32(dataSize);
	
	// write data
	if (compressionMethod == PCL6Writer::kRLECompression) {
		// use RLE compression
		// uint32 compressedSize = 0;
		// HP_M2TIFF_Compress(fStream, 0, (HP_pCharHuge)buffer, bufferSize, &compressedSize);
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

bool PCL6Driver::endPage(int)
{
	try {
		fWriter->EndPage(getJobData()->getCopies());
		return true;
	}
	catch (TransportException &err) {
		return false;
	} 
}

void PCL6Driver::jobEnd()
{
	fWriter->CloseDataSource();
	fWriter->EndSession();
	fWriter->Flush();
	delete fWriter;
	fWriter = NULL;
	// PJL footer
	writeSpoolString("\033%%-12345X@PJL EOJ\n"
	                 "\033%%-12345X");
}

void PCL6Driver::move(int x, int y)
{
	fWriter->SetCursor(x, y);
}

PCL6Writer::MediaSize PCL6Driver::mediaSize(JobData::Paper paper)
{
	switch (paper) {
		case JobData::kLetter:    return PCL6Writer::kLetterPaper;
		case JobData::kLegal:     return PCL6Writer::kLegalPaper;
		case JobData::kA4:        return PCL6Writer::kA4Paper;
		case JobData::kExecutive: return PCL6Writer::kExecPaper;
		case JobData::kLedger:    return PCL6Writer::kLedgerPaper;
		case JobData::kA3:        return PCL6Writer::kA3Paper;
/*
		case : return PCL6Writer::kCOM10Envelope;
		case : return PCL6Writer::kMonarchEnvelope;
		case : return PCL6Writer::kC5Envelope;
		case : return PCL6Writer::kDLEnvelope;
		case : return PCL6Writer::kJB4Paper;
		case : return PCL6Writer::kJB5Paper;
		case : return PCL6Writer::kB5Envelope;
		case : return PCL6Writer::kB5Paper;
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

PCL6Writer::MediaSource PCL6Driver::mediaSource(JobData::PaperSource source)
{
	switch (source) {
		case JobData::kAuto:       return PCL6Writer::kAutoSelect;
		case JobData::kCassette1:  return PCL6Writer::kDefaultSource;
		case JobData::kCassette2:  return PCL6Writer::kEnvelopeTray;
		case JobData::kLower:      return PCL6Writer::kLowerCassette;
		case JobData::kUpper:      return PCL6Writer::kUpperCassette;
		case JobData::kMiddle:     return PCL6Writer::kThirdCassette;
		case JobData::kManual:     return PCL6Writer::kManualFeed;
		case JobData::kCassette3:  return PCL6Writer::kMultiPurposeTray;
		
		default:
			return PCL6Writer::kAutoSelect;
	}
}

