#ifndef FONT_FAMILY_H_
#define FONT_FAMILY_H_

#include <String.h>
#include <Rect.h>
#include <Font.h>
#include <List.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "SharedObject.h"

class FontFamily;
class ServerFont;

enum font_format
{
	FONT_TRUETYPE=0,
	FONT_TYPE_1,
	FONT_OPENTYPE,
	FONT_BDF,
	FONT_CFF,
	FONT_CID,
	FONT_PCF,
	FONT_PFR,
	FONT_SFNT,
	FONT_TYPE_42,
	FONT_WINFONT,
};


// data structure used by the FreeType cache manager
typedef struct CachedFaceRec_
{
	BString file_path; 
	int face_index; 
} CachedFaceRec, *CachedFace;

class FontStyle : public SharedObject
{
public:
	FontStyle(const char *filepath, FT_Face face);
	~FontStyle(void);
	ServerFont *Instantiate(float size, float rotation=0.0, float shear=90.0);
	bool IsFixedWidth(void) { return is_fixedwidth; }
	bool IsScalable(void) { return is_scalable; }
	bool HasKerning(void) { return has_kerning; }
	bool HasTuned(void) { return has_bitmaps; }
	uint16 GlyphCount(void) { return glyphcount; }
	uint16 CharMapCount(void) { return charmapcount; }
	const char *Style(void);
	FontFamily *Family(void) { return family; }
	FT_Face GetFace(void);
	const char *GetPath(void);
	int16 ConvertToUnicode(uint16 c);
protected:
	friend FontFamily;
	FontFamily *family;
	uint16 glyphcount, charmapcount;
	BString *name, *path;
	BList *instances;
	bool is_fixedwidth, is_scalable, has_kerning, has_bitmaps;
	CachedFace cachedface;
	uint8 format;
	BRect fbounds;
};

class FontFamily : public SharedObject
{
public:
	FontFamily(const char *namestr);
	~FontFamily(void);
	const char *GetName(void);
	void AddStyle(const char *path, FT_Face face);
	void RemoveStyle(const char *style);
	bool HasStyle(const char *style);
	int32 CountStyles(void);
	const char *GetStyle(int32 index);
	FontStyle *GetStyle(const char *style);
protected:
	BString *name;
	BList *styles;
};

#endif