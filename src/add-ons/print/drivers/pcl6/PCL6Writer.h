/* 
** PCL6Writer.h
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _PCL6_WRITER_H
#define _PCL6_WRITER_H


#include <SupportDefs.h>


class PCL6Driver;


class PCL6WriterStream {
public:
	virtual			~PCL6WriterStream() {}
	virtual	void	Write(const uint8* data, uint32 size) = 0;
};


class PCL6Writer {
public:
	// DO NOT change this enumerations the order is important!!!
			enum Orientation {
				kPortrait,
				kLandscape,
				kReversePortrait,
				kReverseLandscape
			};
	
			enum MediaSize {
				kLetterPaper,
				kLegalPaper,
				kA4Paper,
				kExecPaper,
				kLedgerPaper,
				kA3Paper,
				kCOM10Envelope,
				kMonarchEnvelope,
				kC5Envelope,
				kDLEnvelope,
				kJB4Paper,
				kJB5Paper,
				kB5Envelope,
				kB5Paper,
				kJPostcard,
				kJDoublePostcard,
				kA5Paper,
				kA6Paper,
				kJB6Paper,
				kJIS8KPaper,
				kJIS16KPaper,
				kJISExecPaper
			};
	
			enum MediaSource {
				kDefaultSource,
				kAutoSelect,
				kManualFeed,
				kMultiPurposeTray,
				kUpperCassette,
				kLowerCassette,
				kEnvelopeTray,
				kThirdCassette
			};
	
			enum Compression {
				kNoCompression,
				kRLECompression,
				kJPEGCompression,
				kDeltaRowCompression
			};
	
			enum ColorSpace {
				kBiLevel,
				kGray,
				kRGB,
				kCMY,
				kCIELab,
				kCRGB,
				kSRGB
			};
	
			enum ColorDepth {
				k1Bit,
				k4Bit,
				k8Bit
			};

			enum ColorMapping {
				kDirectPixel,
				kIndexedPixel,
				kDirectPlane
			};
	
			enum Transparency {
				kOpaque,
				kTransparent
			};

			enum DuplexPageMode {
				kDuplexHorizontalBinding,
				kDuplexVerticalBinding
			};
	
			enum MediaSide {
				kFrontMediaSide,
				kBackMediaSide
			};
	
			enum SimplexPageMode {
				kSimplexFrontSide
			};
	
			enum UnitOfMeasure {
				kInch,
				kMillimeter,
				kTenthsOfAMillimeter
			};
	
			enum ErrorReporting {
				kNoReporting,
				kBackChannel,
				kErrorPage,
				kBackChAndErrPage,
				kNWBackChannel,
				kNWErrorPage,
				kNWBackChAndErrPage
			};
	
			enum Enable {
				kOn,
				kOff
			};
	
			enum Boolean {
				kFalse,
				kTrue
			};
	
			enum ProtocolClass {
				kProtocolClass1_1,
				kProtocolClass2_0,
				kProtocolClass2_1,
				kProtocolClass3_0,
			};
	
					PCL6Writer(PCL6WriterStream* stream,
						uint32 bufferSize = 16 * 1024);
	virtual 		~PCL6Writer();
	
	// these methods throw TransportException if data could not
	// be written
			void	Flush();
	
			void	PJLHeader(ProtocolClass protocolClass, int dpi,
						const char* comment = NULL);
			void	PJLFooter();

			void	BeginSession(uint16 xres, uint16 yres,
						UnitOfMeasure unitOfMeasure,
						ErrorReporting errorReporting);
			void	EndSession();
	
			void	OpenDataSource();
			void	CloseDataSource();
	
			void	BeginPage(Orientation orientation, MediaSize mediaSize,
						MediaSource mediaSource);
			void	BeginPage(Orientation orientation, MediaSize mediaSize,
						MediaSource mediaSource, DuplexPageMode duplexPageMode,
						MediaSide mediaSide);
			void	EndPage(uint16 copies);

			void	SetPageOrigin(int16 x, int16 y);
			void	SetColorSpace(ColorSpace colorSpace);
			void	SetPaintTxMode(Transparency transparency);
			void	SetSourceTxMode(Transparency transparency);
			void	SetROP(uint8 rop);
			void	SetCursor(int16 x, int16 y);	
	
			void	BeginImage(ColorMapping colorMapping, ColorDepth colorDepth,
						uint16 sourceWidth, uint16 sourceHeight,
						uint16 destWidth, uint16 destHeight);
			void	ReadImage(Compression compression, uint16 startLine,
						uint16 blockHeight, uint8 padBytes = 4);	
			void	EndImage();
			void	EmbeddedDataPrefix(uint32 size);
			void	EmbeddedDataPrefix32(uint32 size);

			void	Append(uint8 value);
			void	Append(int16 value);
			void	Append(uint16 value);
			void	Append(int32 value);
			void	Append(uint32 value);
			void	Append(float value);
			void	Append(const uint8* data, uint32 size);
	
			void	AppendData(uint8 value);
			void	AppendData(int16 value);
			void	AppendData(uint16 value);
			void	AppendData(int32 value);
			void	AppendData(uint32 value);
			void	AppendData(float value);
	
			void	AppendDataXY(uint8 x, uint8 y);
			void	AppendDataXY(int16 x, int16 y);
			void	AppendDataXY(uint16 x, uint16 y);
			void	AppendDataXY(int32 x, int32 y);
			void	AppendDataXY(uint32 x, uint32 y);
			void	AppendDataXY(float x, float y);
	
private:
			enum Operator {
				kBeginSession = 0x41,
				kEndSession,
				kBeginPage,
				kEndPage,

				kVendorUnique = 0x46,
				kComment,
				kOpenDataSource,
				kCloseDataSource,
				kEchoComment,
				kQuery,
				kDiagnostic3,
		
				kBeginStream = 0x5b,
				kReadStream,
				kEndStream,
		
				kSetColorSpace = 0x6a,
				kSetCursor,
		
				kSetPageOrigin = 0x75,
				kSetPageRotation,
				kSetPageScale,
				kSetPaintTxMode,
				kSetPenSource,
				kSetPenWidth,
				kSetROP,
				kSetSourceTxMode,

				kBeginImage = 0xb0,
				kReadImage,
				kEndImage,
			};
	
			enum DataTag {
				kUByteData = 0xc0,
				kUInt16Data,
				kUInt32Data,
				kSInt16Data,
				kSInt32Data,
				kReal32Data,
		
				kString = 0xc7,
				kUByteArray,
				kUInt16Array,
				kUInt32Array,
				kSInt16Array,
				kSInt32Array,
				kReal32Array,
		
				kUByteXY = 0xd0,
				kUInt16XY,
				kUInt32XY,
				kSInt16XY,
				kSInt32XY,
				kReal32XY,
		
				kUByteBox = 0xe0,
				kUInt16Box,
				kUInt32Box,
				kSInt16Box,
				kSInt32Box,
				kReal32Box,
		
				k8BitAttrId = 0xf8,
		
				kEmbeddedData = 0xfa,
				kEmbeddedDataByte,
			};

			enum DataType {
				kUByte,
				kSByte,
				kUInt16,
				kSInt16,
				kReal32
			};

			enum Attribute {
				kCMYColor = 1,
				kPaletteDepth,
				kColorSpace,
		
				kRGBColor = 11,
		
				kMediaDest = 36,
				kMediaSize,
				kMediaSource,
				kMediaType,
				kOrientation,
				kPageAngle,
				kPageOrigin,
				kPageScale,
				kROP3,
				kTxMode,
		
				kCustomMediaSize = 47,
				kCustomMediaSizeUnits,
				kPageCopies,
				kDitherMatrixSize,
				kDitherMatrixDepth,
				kSimplexPageMode,
				kDuplexPageMode,
				kDuplexPageSide,
		
				kPoint = 76,
		
				kColorDepth = 98,
				kBlockHeight,
				kColorMapping,
				kCompressMode,
				kDestinationBox,
				kDestinationSize,
		
				kSourceHeight = 107,
				kSourceWidth,
				kStartLine,
				kPadBytesMultiple,
				kBlockByteLength,
		
				kNumberOfScanLines = 115,
		
				kDataOrg = 130,
				kMeasure = 134,
		
				kSourceType = 136,
				kUnitsPerMeasure,
				kQueryKey,
				kStreamName,
				kStreamDataLength,
		
				kErrorReport = 143,
				kIOReadTimeOut,
		
				kWritingMode = 173
			};

			enum DataSource {
				kDefaultDataSource
			};

			enum DataOrganization {
				kBinaryHighByteFirst,
				kBinaryLowByteFirst
			};

			void	AppendString(const char* string);
			void	AppendOperator(Operator op);
			void	AppendAttribute(Attribute attr);
			void	AppendDataTag(DataTag tag);
	
			PCL6WriterStream* fStream;
				// the stream used for writing the generated PCL6 data
			uint8*	fBuffer;	// the buffer
			uint32	fSize;		// the size of the buffer
			uint32	fIndex;		// the index of the next byte to be written
};

#endif // _PCL6_WRITER_H
