/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RAW_H
#define RAW_H


#include "ReadHelper.h"


struct jhead;
struct tiff_tag;


struct image_meta_info {
	char	manufacturer[64];
	char	model[128];
	char	software[64];
	float	flash_used;
	float	iso_speed;
	float	shutter;
	float	aperture;
	float	focal_length;
	double	pixel_aspect;
	uint32	raw_width;
	uint32	raw_height;
	int		flip;
	uint32	dng_version;
	uint32	shot_order;
	int32	black;
	int32	maximum;
	float	camera_multipliers[4];
	float	pre_multipliers[4];
	float	rgb_camera[3][4];	/* RGB from camera color */
	time_t	timestamp;
};

struct image_data_info {
	uint32	width;
	uint32	height;
	uint32	output_width;
	uint32	output_height;
	uint32	bits_per_sample;
	uint32	compression;
	uint32	photometric_interpretation;
	uint32	flip;
	uint32	samples;
	uint32	bytes;
	off_t	data_offset;
	bool	is_raw;
};

#define COMPRESSION_NONE		1
#define COMPRESSION_OLD_JPEG	6		// Old JPEG (before 6.0)
#define COMPRESSION_PACKBITS	32773	// Macintosh RLE


typedef void (*monitor_hook)(const char* message, float percentage, void* data);

class DCRaw {
	public:
		DCRaw(BPositionIO& stream);
		~DCRaw();

		status_t Identify();
		status_t ReadImageAt(uint32 index, uint8*& outputBuffer,
			size_t& bufferSize);

		void GetMetaInfo(image_meta_info& metaInfo) const;
		uint32 CountImages() const;
		status_t ImageAt(uint32 index, image_data_info& info) const;

		status_t GetEXIFTag(off_t& offset, size_t& length,
			bool& bigEndian) const;
		void SetProgressMonitor(monitor_hook hook, void* data);

		void SetHalfSize(bool half);

	private:
		int32 _AllocateImage();
		image_data_info& _Raw();
		image_data_info& _Thumb();
		void _CorrectIndex(uint32& index) const;
		uint16& _Bayer(int32 column, int32 row);
		int32 _FilterCoefficient(int32 column, int32 row);
		int32 _FlipIndex(uint32 row, uint32 col, uint32 flip);
		bool _SupportsCompression(image_data_info& info) const;
		bool _IsCanon() const;
		bool _IsKodak() const;
		bool _IsNikon() const;
		bool _IsOlympus() const;
		bool _IsPentax() const;
		bool _IsSamsung() const;

		// image manipulation and conversion
		void _ScaleColors();
		void _WaveletDenoise();
		void _PreInterpolate();
		void _CameraToCIELab(ushort cam[4], float lab[3]);
		void _CameraXYZCoefficients(double cam_xyz[4][3]);
		void _AdobeCoefficients(const char *manufacturer, const char *model);
		void _BorderInterpolate(uint32 border);
		void _AHDInterpolate();
		void _PseudoInverse(double (*in)[3], double (*out)[3], uint32 size);
		void _ConvertToRGB();
		void _GammaLookUpTable(uchar* lut);

		void _ParseThumbTag(off_t baseOffset, uint32 offsetTag, uint32 lengthTag);
		void _ParseManufacturerTag(off_t baseOffset);
		void _ParseEXIF(off_t baseOffset);
		void _ParseLinearTable(uint32 length);
		void _FixupValues();

		// Lossless JPEG
		void _InitDecoder();
		uchar *_MakeDecoder(const uchar* source, int level);
		void _InitDecodeBits();
		uint32 _GetDecodeBits(uint32 numBits);
		status_t _LosslessJPEGInit(struct jhead* jh, bool infoOnly);
		int _LosslessJPEGDiff(struct decode *dindex);
		void _LosslessJPEGRow(struct jhead *jh, int jrow);

		// RAW Loader
		void _LoadRAWUnpacked(const image_data_info& image);
		void _LoadRAWPacked12(const image_data_info& info);
		void _MakeCanonDecoder(uint32 table);
		bool _CanonHasLowBits();
		void _LoadRAWCanonCompressed(const image_data_info& info);
		void _LoadRAWLosslessJPEG(const image_data_info& image);
		void _LoadRAW(const image_data_info& info);

		// Image writers
		void _WriteJPEG(image_data_info& image, uint8* outputBuffer);
		void _WriteRGB32(image_data_info& image, uint8* outputBuffer);

		// TIFF
		time_t _ParseTIFFTimestamp(bool reversed);
		void _ParseTIFFTag(off_t baseOffset, tiff_tag& tag, off_t& offset);
		status_t _ParseTIFFImageFileDirectory(off_t baseOffset, uint32 offset);
		status_t _ParseTIFFImageFileDirectory(off_t baseOffset);
		status_t _ParseTIFF(off_t baseOffset);

		TReadHelper	fRead;
		image_meta_info fMeta;
		image_data_info* fImages;
		uint32		fNumImages;

		int32		fRawIndex;
		int32		fThumbIndex;
		uint32		fDNGVersion;
		bool		fIsTIFF;

		uint16		(*fImageData)[4];
			// output image data
		float		fThreshold;
		int32		fShrink;
		bool		fHalfSize;
		bool		fUseCameraWhiteBalance;
		bool		fUseAutoWhiteBalance;
		bool		fRawColor;
		bool		fUseGamma;
		float		fBrightness;
		int32		fOutputColor;
		int32		fHighlight;
		int32		fDocumentMode;
		uint32		fOutputWidth;
		uint32		fOutputHeight;
		uint32		fInputWidth;
		uint32		fInputHeight;
		int32		fTopMargin;
		int32		fLeftMargin;
		uint32		fUniqueID;
		uint32		fColors;
		uint16		fWhite[8][8];
		float		fUserMultipliers[4];
		uint16*		fCurve;
		uint16		fCR2Slice[3];
		uint32*		fOutputProfile;
		uint32		fOutputBitsPerSample;
		int32		(*fHistogram)[4];
		float*		cbrt;
		float		xyz_cam[3][4];
			// TODO: find better names for these...

		// Lossless JPEG
		decode*		fDecodeBuffer;
		decode*		fSecondDecode;
		decode*		fFreeDecode;
		int			fDecodeLeaf;
		uint32		fDecodeBits;
		uint32		fDecodeBitsRead;
		bool		fDecodeBitsReset;
		bool		fDecodeBitsZeroAfterMax;

		uint32		fFilters;
		uint32		fEXIFFilters;
		off_t		fEXIFOffset;
		uint32		fEXIFLength;

		off_t		fCurveOffset;

		monitor_hook fProgressMonitor;
		void*		fProgressData;
};

#endif	// RAW_H
