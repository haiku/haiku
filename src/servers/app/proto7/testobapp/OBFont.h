#ifndef OBFONT_H_
#define OBFONT_H_

#include <BeBuild.h>
#include <SupportDefs.h>
#include <InterfaceDefs.h>

#include <Font.h>	// for various font structure definitions

class BShape;
class BString;
class OBFontPrivate;

class OBFont {
public:
							OBFont(void);
							OBFont(const OBFont &font);	
							OBFont(const OBFont *font);			
							~OBFont(void);

		status_t			SetFamilyAndStyle(const font_family family, 
											  const font_style style);
		void				SetFamilyAndStyle(uint32 code);
		status_t			SetFamilyAndFace(const font_family family, uint16 face);
		
		void				SetSize(float size);
		void				SetShear(float shear);
		void				SetRotation(float rotation);
		void				SetSpacing(uint8 spacing);
		void				SetEncoding(uint8 encoding);
		void				SetFace(uint16 face);
		void				SetFlags(uint32 flags);

		void				GetFamilyAndStyle(font_family *family,
											  font_style *style) const;
		uint32				FamilyAndStyle(void) const;
		float				Size(void) const;
		float				Shear(void) const;
		float				Rotation(void) const;
		uint8				Spacing(void) const;
		uint8				Encoding(void) const;
		uint16				Face(void) const;
		uint32				Flags(void) const;
		
		font_direction		Direction(void) const; 
		bool				IsFixed(void) const;	
		bool				IsFullAndHalfFixed(void) const;	
		BRect				BoundingBox(void) const;
		unicode_block		Blocks(void) const;
		font_file_format	FileFormat(void) const;

		int32				CountTuned(void) const;
		void				GetTunedInfo(int32 index, tuned_font_info *info) const;

		void				TruncateString(BString* in_out,
										   uint32 mode,
										   float width) const;
		void            	GetTruncatedStrings(const char *stringArray[], 
												int32 numStrings, 
												uint32 mode,
												float width, 
												BString resultArray[]) const;
		void            	GetTruncatedStrings(const char *stringArray[], 
												int32 numStrings, 
												uint32 mode,
												float width, 
												char *resultArray[]) const;

		float				StringWidth(const char *string) const;
		float				StringWidth(const char *string, int32 length) const;
		void				GetStringWidths(const char *stringArray[], 
											const int32 lengthArray[], 
											int32 numStrings, 
											float widthArray[]) const;

		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   float escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   float escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   BPoint escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   BPoint escapementArray[],
										   BPoint offsetArray[]) const;
									   
		void				GetEdges(const char charArray[], 
									 int32 numBytes,
									 edge_info edgeArray[]) const;
		void				GetHeight(font_height *height) const;
		
		void				GetBoundingBoxesAsGlyphs(const char charArray[], 
													 int32 numChars, 
													 font_metric_mode mode,
													 BRect boundingBoxArray[]) const;
		void				GetBoundingBoxesAsString(const char charArray[], 
													 int32 numChars, 
													 font_metric_mode mode,
													 escapement_delta *delta,
													 BRect boundingBoxArray[]) const;
		void				GetBoundingBoxesForStrings(const char *stringArray[],
													   int32 numStrings,
													   font_metric_mode mode,
													   escapement_delta deltas[],
													   BRect boundingBoxArray[]) const;
		
		void				GetGlyphShapes(const char charArray[],
										   int32 numChars,
										   BShape *glyphShapeArray[]) const;							   

		void				GetHasGlyphs(const char charArray[],
										 int32 numChars,
										 bool hasArray[]) const;
									 
		OBFont&				operator=(const OBFont &font); 
		bool				operator==(const OBFont &font) const;
		bool				operator!=(const OBFont &font) const; 

		void				PrintToStream(void) const;
	
private:
		uint16				fFamilyID;
		uint16				fStyleID;
		float				fSize;
		float				fShear;
		float				fRotation;
		uint8				fSpacing;
		uint8				fEncoding;
		uint16				fFace;
		uint32				fFlags;
		font_height			fHeight;
		OBFontPrivate		*private_data;
		uint32				_reserved[2];

		void           		SetPacket(void *packet) const;
		void           		GetTruncatedStrings64(const char *stringArray[], 
												  int32 numStrings, 
												  uint32 mode,
												  float width, 
												  char *resultArray[]) const;
		void           		GetTruncatedStrings64(const char *stringArray[], 
												  int32 numStrings, 
												  uint32 mode,
												  float width, 
												  BString resultArray[]) const;
		void				_GetEscapements_(const char charArray[],
											 int32 numChars, 
											 escapement_delta *delta,
											 uint8 mode,
											 float *escapements,
											 float *offsets = NULL) const;
		void				_GetBoundingBoxes_(const char charArray[], 
											   int32 numChars, 
											   font_metric_mode mode,
											   bool string_escapement,
											   escapement_delta *delta,
											   BRect boundingBoxArray[]) const;
};

/*----------------------------------------------------------------*/
/*----- OBFont related declarations -------------------------------*/

//extern const OBFont *obe_plain_font;
//extern const OBFont *obe_bold_font;
//extern const OBFont *obe_fixed_font;
extern OBFont *obe_plain_font;
extern OBFont *obe_bold_font;
extern OBFont *obe_fixed_font;

int32 ob_count_font_families(void);
status_t ob_get_font_family(int32 index, font_family *name, uint32 *flags=NULL);

int32 ob_count_font_styles(font_family name);
status_t ob_get_font_style(font_family family, int32 index, font_style *name,
		uint32 *flags=NULL);
status_t ob_get_font_style(font_family family, int32 index, font_style *name, 
		uint16 *face, uint32 *flags=NULL);
bool ob_update_font_families(bool check_only);

status_t ob_get_font_cache_info(uint32 id, void *set);
status_t ob_set_font_cache_info(uint32 id, void *set);

#endif
