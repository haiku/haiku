/*
	This class is the shadow class to BFont. The only problem is that
	FreeType is going to need to be, um, "customized" to do everything
	we need it to do, such as the file format, other encodings, etc.
*/

#include "FontFamily.h"
#include "ServerFont.h"


ServerFont::ServerFont(FontStyle *style, float size=12.0, float rotation=0.0, float shear=90.0,
	uint16 flags=0, uint8 spacing=B_CHAR_SPACING)
{
	fstyle=style;
	fsize=size;
	frotation=rotation;
	fshear=shear;
	fflags=flags;
	fspacing=spacing;
	fdirection=B_FONT_LEFT_TO_RIGHT;
	fface=B_REGULAR_FACE;
	ftruncate=B_TRUNCATE_END;
	fencoding=B_UNICODE_UTF8;
	fbounds.Set(0,0,0,0);
	if(fstyle)
		fstyle->AddDependent();
}

ServerFont::ServerFont(const ServerFont &font)
{
	fstyle=font.fstyle;
	fsize=font.fsize;
	frotation=font.frotation;
	fshear=font.fshear;
	fflags=font.fflags;
	fspacing=font.fspacing;
	fdirection=font.fdirection;
	fface=font.fface;
	ftruncate=font.ftruncate;
	fencoding=font.fencoding;
	fbounds.Set(0,0,0,0);
	if(fstyle)
		fstyle->AddDependent();
}

ServerFont::~ServerFont(void)
{
	if(fstyle)
		fstyle->RemoveDependent();
}

/*	TODO: implement these functions
BRect BoundingBox(void)
{
}

escapement_delta Escapements(char c)
{
}
*/

const BRect &ServerFont::BoundingBox(void)
{
	return fbounds;
}

BRect ServerFont::StringBounds(const char *string)
{
	return BRect(0,0,0,0);
}

void ServerFont::Height(font_height *fh)
{
	fh->ascent=fheight.ascent;
	fh->descent=fheight.descent;
	fh->leading=fheight.leading;
}

