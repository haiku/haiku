#ifndef SERVERFONT_H_
#define SERVERFONT_H_

#include <Rect.h>
#include <Font.h>

class FontStyle;

class ServerFont
{
public:
	ServerFont(FontStyle *fstyle, float fsize=12.0, float frotation=0.0, float fshear=90.0,
	uint16 flags=0, uint8 spacing=B_CHAR_SPACING);
	ServerFont(const ServerFont &font);
	~ServerFont(void);
	font_direction Direction(void) { return fdirection; }
	uint8 Encoding(void) { return fencoding; }
	edge_info Edges(void) { return fedges; }
	uint32 Flags(void) { return fflags; }
	uint8 Spacing(void) { return fspacing; }
	float Shear(void) { return fshear; }
	float Rotation(void) { return frotation; }
	float Size(void) { return fsize; }
	uint16 Face(void) { return fface; }
	uint32 CountGlyphs(void);
	FontStyle *Style(void) { return fstyle; }

	void SetDirection(const font_direction &dir) { fdirection=dir; }
	void SetEdges(const edge_info &info) { fedges=info; }
	void SetEncoding(uint8 encoding) { fencoding=encoding; }
	void SetFlags(const uint32 &value) { fflags=value; }
	void SetSpacing(const uint8 &value) { fspacing=value; }
	void SetShear(const float &value) { fshear=value; }
	void SetSize(const float &value) { fsize=value; }
	void SetRotation(const float &value) { frotation=value; }
	void SetFace(const uint16 &value) { fface=value; }
	
//	BRect BoundingBox(void);
//	escapement_delta Escapements(char c);
	void Height(font_height *fh);
protected:
	friend FontStyle;
	FontStyle *fstyle;
	font_height fheight;
	escapement_delta fescapements;
	edge_info fedges;
	float fsize, frotation, fshear;
	uint32 fflags;
	uint8 fspacing;
	uint16 fface;
	font_direction fdirection;
	uint8 ftruncate;
	uint8 fencoding;
};

#endif
