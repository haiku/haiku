// BMPTranslator.h

#ifndef BMP_TRANSLATOR_H
#define BMP_TRANSLATOR_H

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <ByteOrder.h>

#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

#define BMP_TRANSLATOR_VERSION 100
#define BMP_QUALITY 1.0
#define BMP_CAPABILITY 1.0

#define BBT_QUALITY 1.0
#define BBT_CAPABILITY 1.0

struct BMPFileHeader {
	uint16 magic;			// = 'BM'
	uint32 fileSize;		// file size in bytes
	uint32 reserved;		// equals 0
	uint32 dataOffset;		// file offset to actual image
};

struct BMPInfoHeader {
	uint32 size;			// size of this struct (40)
	uint32 width;			// bitmap width
	uint32 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
	uint32 compression;		// type of compression (0 none, 1 8 bit RLE, 2 4 bit RLE)
	uint32 imagesize;		// compressed size of image? set to zero if compression == 0?
	uint32 xpixperm;		// horizontal pixels per meter
	uint32 ypixperm;		// vertical pixels per meter
	uint32 colorsused;		// number of actually used colors
	uint32 colorsimportant;	// number of important colors, zero = all
};

struct BMPColorTable {
	uint8 red;				// red intensity
	uint8 green;			// green intensity
	uint8 blue;				// blue intensity
	uint8 reserved;			// unused (=0)
};

class BMPTranslator : public BTranslator {
public:
	BMPTranslator();
	
	virtual const char *TranslatorName() const;
		// returns the short name of the translator
		
	virtual const char *TranslatorInfo() const;
		// returns a verbose name/description for the translator
	
	virtual int32 TranslatorVersion() const;
		// returns the version of the translator

	virtual const translation_format *InputFormats(int32 *out_count)
		const;
		// returns the input formats and the count of input formats
		// that this translator supports
		
	virtual const translation_format *OutputFormats(int32 *out_count)
		const;
		// returns the output formats and the count of output formats
		// that this translator supports

	virtual status_t Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		// determines whether or not this translator can convert the
		// data in inSource to the type outType

	virtual status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination);
		// this function is the whole point of the Translation Kit,
		// it translates the data in inSource to outDestination
		// using the format outType

protected:
	virtual ~BMPTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	char fName[30];
	char fInfo[100];
};

#endif // #ifndef BMP_TRANSLATOR_H
