#ifndef __WIDTHBUFFER_H
#define __WIDTHBUFFER_H

#include "TextGapBuffer.h"
#include "TextViewSupportBuffer.h"


struct _width_table_ {
#if B_BEOS_VERSION_DANO
	BFont font;				// corresponding font
#else
	int32 fontCode;			// font code
	float fontSize;			// font size
#endif
	int32 hashCount;		// number of hashed items
	int32 tableCount;		// size of table
	void *widths;			// width table	
};


class _BWidthBuffer_ : public _BTextViewSupportBuffer_<_width_table_> {
public:
	_BWidthBuffer_();
	virtual ~_BWidthBuffer_();

	float StringWidth(const char *inText, int32 fromOffset, int32 length,
		const BFont *inStyle);
	float StringWidth(_BTextGapBuffer_ &, int32 fromOffset, int32 length,
		const BFont *inStyle);

private:
	bool FindTable(const BFont *font, int32 *outIndex);
	int32 InsertTable(const BFont *font);
	
	bool GetEscapement(uint32, int32, float *);
	float HashEscapements(const char *, int32, int32, int32, const BFont *);
	
	static uint32 Hash(uint32);
};

#endif // __WIDTHBUFFER_H
