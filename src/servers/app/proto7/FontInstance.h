#ifndef FONTINSTANCE_H_
#define FONTINSTANCE_H_

class FontStyle;

#include <Font.h>

class FontInstance
{
public:
	FontInstance(void);
	FontInstance(float size, float rotation, float shear);
	~FontInstance(void);

	void SetProperties(float size=12.0, float rotation=0.0, float shear=90.0);
	void SetSpacing(uint8 spacing);
	void SetFlags(int32 flags);
	void SetFace(int32 face);
	void SetSize(float size);
	void SetRotation(float rotation);
	void SetShear(float shear);
	void SetEncoding(uint8 encoding);

	unicode_block Blocks(void);
	BRect BoundingBox(void);
	float Rotation(void) { return fRotation; }
	float Shear(void) { return fShear; }
	float Size(void) { return fSize; }
	uint8 Spacing(void) { return fSpacing; }
	uint8 Encoding(void) { return fEncoding; }
	font_direction Direction(void) { return fDirection; }
	FontStyle *Style(void) { return fStyle; }

	void Height(font_height *height) { *height=fHeight; }

	float fSize,
		  fRotation,
		  fShear;

	int32 fToken,
		  fFlags,
		  fFace;

	uint8 fEncoding,
		  fSpacing;

	font_height fHeight;
	font_direction fDirection;
	
	FontStyle *fStyle;
};

#endif