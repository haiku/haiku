/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef PDFWRITER_H
#define PDFWRITER_H

#include <AppKit.h>
#include <InterfaceKit.h>
#include <String.h>
#include <List.h>

#include "math.h"

#include "PrinterDriver.h"
#include "PictureIterator.h"
#include "Fonts.h"
#include "SubPath.h"
#include "Utils.h"
#include "Link.h"
#include "ImageCache.h"
#include "PDFSystem.h"

#include "pdflib.h"

#define USE_IMAGE_CACHE 1

#define RAD2DEGREE(r) (180.0 * r / PI)
#define DEGREE2RAD(d) (PI * d / 180.0)

class DrawShape;
class WebLink;
class Bookmark;
class XRefDefs;
class XRefDests;

class PDFWriter : public PrinterDriver, public PictureIterator
{
	
	friend class DrawShape;
	friend class PDFLinePathBuilder;
	friend class WebLink;
	friend class Link;
	friend class Bookmark;
	friend class LocalLink;
	friend class TextLine;
	
	public:
		// constructors / destructor
		PDFWriter();
		~PDFWriter();
		
		// public methods
		status_t	BeginJob();
		status_t 	PrintPage(int32 pageNumber, int32 pageCount);
		status_t	EndJob();
		status_t	InitWriter();
		status_t	BeginPage(BRect paperRect, BRect printRect);
		status_t	EndPage();

		void        SetAttribute(const char* name, const char* value);
		bool        LoadBookmarkDefinitions(const char* name);
		bool        LoadXRefsDefinitions(const char* name);
		void        RecordDests(const char* s);
		
		// PDFLib callbacks
		size_t		WriteData(void *data, size_t size);
		void		ErrorHandler(int type, const char *msg);
		
		// Image support
		int32       BytesPerPixel(int32 pixelFormat);

		bool        HasAlphaChannel(int32 pixelFormat);
		bool        NeedsBPC1Mask(int32 pixelFormat);

		inline bool IsTransparentRGB32(uint8* in);
		inline bool IsTransparentRGBA32(uint8* in);
		inline bool IsTransparentRGB32_BIG(uint8* in);
		inline bool IsTransparentRGBA32_BIG(uint8* in);
		//inline bool IsTransparentRGB24(uint8* in);
		//inline bool IsTransparentRGB24_BIG(uint8* in);
		//inline bool IsTransparentRGB16(uint8* in);
		//inline bool IsTransparentRGB16_BIG(uint8* in);
		inline bool IsTransparentRGB15(uint8* in);
		inline bool IsTransparentRGB15_BIG(uint8* in);
		inline bool IsTransparentRGBA15(uint8* in);
		inline bool IsTransparentRGBA15_BIG(uint8* in);
		inline bool IsTransparentCMAP8(uint8* in);
		//inline bool IsTransparentGRAY8(uint8* in);
		//inline bool IsTransparentGRAY1(uint8* in);

		inline uint8 AlphaFromRGBA32(uint8* in);
		inline uint8 AlphaFromRGBA32_BIG(uint8* in);

		inline void ConvertFromRGB32(uint8* in, uint8* out);
		inline void ConvertFromRGB32_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGBA32(uint8* in, uint8* out);
		inline void ConvertFromRGBA32_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGB24(uint8* in, uint8* out);
		inline void ConvertFromRGB24_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGB16(uint8* in, uint8* out);
		inline void ConvertFromRGB16_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGB15(uint8* in, uint8* out);
		inline void ConvertFromRGBA15_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGB15_BIG(uint8* in, uint8* out);
		inline void ConvertFromRGBA15(uint8* in, uint8* out);
		inline void ConvertFromCMAP8(uint8* in, uint8* out);
		inline void ConvertFromGRAY8(uint8* in, uint8* out);
		inline void ConvertFromGRAY1(uint8* in, uint8* out, int8 bit);

		uint8		*CreateMask(BRect src, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data);
		uint8		*CreateSoftMask(BRect src, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data);
		BBitmap		*ConvertBitmap(BRect src, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data);
		bool		GetImages(BRect src, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data, int* mask, int* image);

		// String handling
		bool		BeginsChar(char byte) { return BEGINS_CHAR(byte); }
		void		ToUtf8(uint32 encoding, const char *string, BString &utf8);
		void		ToUnicode(const char *string, BString &unicode);
		void		ToPDFUnicode(const char *string, BString &unicode);
		uint16		CodePointSize(const char *s);
		void		DrawChar(uint16 unicode, const char *utf8, int16 size);
		void		ClipChar(BFont* font, const char* unicode, const char *utf8, int16 size, float width);
		bool   		EmbedFont(const char* n);
		status_t	DeclareFonts();
		void        RecordFont(const char* family, const char* style, float size);

		// BPicture playback handlers
		void		Op(int number);
		void		MovePenBy(BPoint delta);
		void		StrokeLine(BPoint start, BPoint end);
		void		StrokeRect(BRect rect);
		void		FillRect(BRect rect);
		void		StrokeRoundRect(BRect rect, BPoint radii);
		void		FillRoundRect(BRect rect, BPoint radii);
		void		StrokeBezier(BPoint *control);
		void		FillBezier(BPoint *control);
		void		StrokeArc(BPoint center, BPoint radii, float startTheta, float arcTheta);
		void		FillArc(BPoint center, BPoint radii, float startTheta, float arcTheta);
		void		StrokeEllipse(BPoint center, BPoint radii);
		void		FillEllipse(BPoint center, BPoint radii);
		void		StrokePolygon(int32 numPoints, BPoint *points, bool isClosed);
		void		FillPolygon(int32 numPoints, BPoint *points, bool isClosed);
		void        StrokeShape(BShape *shape);
		void        FillShape(BShape *shape);
		void		DrawString(char *string, float deltax, float deltay);
		void		DrawPixels(BRect src, BRect dest, int32 width, int32 height, int32 bytesPerRow, int32 pixelFormat, int32 flags, void *data);
		void		SetClippingRects(BRect *rects, uint32 numRects);
		void    	ClipToPicture(BPicture *picture, BPoint point, bool clip_to_inverse_picture);
		void		PushState();
		void		PopState();
		void		EnterStateChange();
		void		ExitStateChange();
		void		EnterFontState();
		void		ExitFontState();
		void		SetOrigin(BPoint pt);
		void		SetPenLocation(BPoint pt);
		void		SetDrawingMode(drawing_mode mode);
		void		SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit);
		void		SetPenSize(float size);
		void		SetForeColor(rgb_color color);
		void		SetBackColor(rgb_color color);
		void		SetStipplePattern(pattern p);
		void		SetScale(float scale);
		void		SetFontFamily(char *family);
		void		SetFontStyle(char *style);
		void		SetFontSpacing(int32 spacing);
		void		SetFontSize(float size);
		void		SetFontRotate(float rotation);
		void		SetFontEncoding(int32 encoding);
		void		SetFontFlags(int32 flags);
		void		SetFontShear(float shear);
		void		SetFontFace(int32 flags);
		
		static inline bool IsSame(const pattern &p1, const pattern &p2);
		static inline bool IsSame(const rgb_color &c1, const rgb_color &c2);
		
	private:
	
		enum PDFVersion {
			kPDF13,
			kPDF14,
			kPDF15
		};
	
		class State 
		{
		public:
			State			*prev;
			BFont           beFont;
			int				font;
			PDFSystem       pdfSystem;
			float			penX;
			float			penY;
			drawing_mode 	drawingMode;
			rgb_color		foregroundColor;
			rgb_color		backgroundColor;
			rgb_color       currentColor;
			cap_mode 		capMode;
			join_mode 		joinMode;
			float 			miterLimit;
			float           penSize;
			pattern         pattern0;
			int32           fontSpacing;
			
			// initialize with defalt values
			State(float h = a4_height, float x = 0, float y = 0) 
			{
				static rgb_color white    = {255, 255, 255, 255};
				static rgb_color black    = {0, 0, 0, 255};
				prev = NULL;
				font             = 0;
				pdfSystem.SetHeight(h);
				pdfSystem.SetOrigin(x, y);
				penX             = 0;
				penY             = 0;
				drawingMode      = B_OP_COPY; 
				foregroundColor  = white;
				backgroundColor  = black;
				currentColor     = black;
				capMode          = B_BUTT_CAP; 
				joinMode         = B_MITER_JOIN; 
				miterLimit       = B_DEFAULT_MITER_LIMIT; 
				penSize          = 1; 
				pattern0         = B_SOLID_HIGH; 
				fontSpacing      = B_STRING_SPACING; 
			}

			State(State *prev) 
			{
				*this = *prev;
				this->prev = prev;
			}
		};

		class Font 
		{
		public:
			Font(char *n, int f, font_encoding e) : name(n), font(f), encoding(e) { }
			BString name;
			int     font;
			font_encoding encoding;
		};

		class Pattern
		{
		public:
			pattern     pat;
			rgb_color   lowColor, highColor;
			int         patternId;
			
			Pattern(const pattern &p, rgb_color low, rgb_color high, int id)
				: pat(p)
				, lowColor(low)
				, highColor(high)
				, patternId(id)
			{};
			
			inline bool Matches(const pattern &p, rgb_color low, rgb_color high) const {
				return IsSame(pat, p) && IsSame(lowColor, low) && IsSame(highColor, high);
			};
		};

		class Transparency
		{
			uint8	alpha;
			int		handle;
		public:			
			Transparency(uint8 alpha, int handle)
				: alpha(alpha)
				, handle(handle)
			{};
			
			inline bool Matches(uint8 alpha) const {
				return this->alpha == alpha;
			};
			
			inline int Handle() const { return handle; }
		};
	
		PDFVersion      fPDFVersion;
		FILE			*fLog;
		PDF				*fPdf;
		int32           fPage;
		State			*fState;
		int32           fStateDepth;
		TList<Font>     fFontCache;
		TList<Pattern>  fPatterns;
		TList<Transparency> fTransparencyCache;
		TList<Transparency> fTransparencyStack;
		ImageCache      fImageCache;
		int64           fEmbedMaxFontSize;
		BScreen         *fScreen;
		Fonts           *fFonts;
		bool            fCreateWebLinks;
		bool            fCreateBookmarks;
		Bookmark        *fBookmark;
		bool            fCreateXRefs;
		XRefDefs        *fXRefs;
		XRefDests       *fXRefDests;
		font_encoding   fFontSearchOrder[no_of_cjk_encodings];
		TextLine        fTextLine;
		TList<UsedFont> fUsedFonts;
		UserDefinedEncodings fUserDefinedEncodings;
		
		enum 
		{
			kDrawingMode,
			kClippingMode
		}				fMode;

		inline float tx(float x)    { return fState->pdfSystem.tx(x); }
		inline float ty(float y)    { return fState->pdfSystem.ty(y); }
		inline float scale(float f) { return fState->pdfSystem.scale(f); }

		PDFSystem* pdfSystem() const { return &fState->pdfSystem; }

		enum
		{
			kStroke = true,
			kFill   = false
		};

		inline bool MakesPattern()  { return Pass() == 0; }
		inline bool MakesPDF()      { return Pass() == 1; }

		inline bool IsDrawing() const  { return fMode == kDrawingMode; }
		inline bool IsClipping() const { return fMode == kClippingMode; }

		// PDF features depending on PDF version:
		inline bool SupportsSoftMask() const { return fPDFVersion >= kPDF14; }
		inline bool SupportsOpacity() const { return fPDFVersion >= kPDF14; }

		inline float     PenSize() const        { return fState->penSize; }
		inline cap_mode  LineCapMode() const    { return fState->capMode; }
		inline join_mode LineJoinMode() const   { return fState->joinMode; }
		inline float     LineMiterLimit() const { return fState->miterLimit; }
		
		bool StoreTranslatorBitmap(BBitmap *bitmap, const char *filename, uint32 type);

		void GetFontName(BFont *font, char *fontname);
		void GetFontName(BFont *font, char *fontname, bool &embed, font_encoding encoding);
		int FindFont(char *fontname, bool embed, font_encoding encoding);
		void MakeUserDefinedEncoding(uint16 unicode, uint8 &enc, uint8 &index);

		// alpha transparency
		Transparency* FindTransparency(uint8 alpha);
		void BeginTransparency();
		void EndTransparency();

		void PushInternalState();
		bool PopInternalState();

		void SetColor(rgb_color toSet);
		void SetColor();
		void CreatePattern();
		int  FindPattern();
		void SetPattern();
		
		void StrokeOrClip();
		void FillOrClip();
		void Paint(bool stroke);
		void PaintShape(BShape *shape, bool stroke);
		void PaintRoundRect(BRect rect, BPoint radii, bool stroke);
		void PaintArc(BPoint center, BPoint radii, float startTheta, float arcTheta, int stroke);
		void PaintEllipse(BPoint center, BPoint radii, bool stroke);
};



// --------------------------------------------------
inline bool 
PDFWriter::IsSame(const pattern &p1, const pattern &p2)
{
	char *a = (char*)p1.data;
	char *b = (char*)p2.data;
	return memcmp(a, b, 8) == 0;
}


// --------------------------------------------------
inline bool 
PDFWriter::IsSame(const rgb_color &c1, const rgb_color &c2)
{
	char *a = (char*)&c1;
	char *b = (char*)&c1;
	return memcmp(a, b, sizeof(rgb_color)) == 0;
}


// PDFLib C callbacks class instance redirectors
size_t	_WriteData(PDF *p, void *data, size_t size);
void	_ErrorHandler(PDF *p, int type, const char *msg);

#endif	// #if PDFWRITER_H
