#ifndef _SVG_VIEW_VIEW_H
#define _SVG_VIEW_VIEW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <View.h>

// Project Includes ------------------------------------------------------------
#include "tinyxml.h"
#include "Matrix.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

enum {
	STROKE_FLAG			= 0x01,
	FILL_FLAG			= 0x02,
	STROKE_WIDTH_FLAG	= 0x04,
	LINE_MODE_FLAG		= 0x08,
	FONT_SIZE_FLAG		= 0x10,
	MATRIX_FLAG			= 0x20,

};

struct _state_ {

	_state_() { set_default_values(); }
	_state_(_state_ &state) { *this = state; }
	void set_default_values()
	{
		fFlags = 0;
		fStrokeColor.red = 0; fStrokeColor.green = 0;
		fStrokeColor.blue = 0; fStrokeColor.alpha = 255;
		fStroke = false;
		fFillColor.red = 0; fFillColor.green = 0;
		fFillColor.blue = 0; fFillColor.alpha = 255;
		fFill = true;
		fStrokeWidth = 1.0f;
		fLineCap = B_BUTT_CAP;
		fLineJoin = B_MITER_JOIN;
		fLineMiterLimit = B_DEFAULT_MITER_LIMIT;
		fFontSize = 9.0f;
	}

	uint32		fFlags;
	rgb_color	fStrokeColor;
	bool		fStroke;
	rgb_color	fFillColor;
	bool		fFill;
	float		fStrokeWidth;
	cap_mode	fLineCap;
	join_mode	fLineJoin;
	float		fLineMiterLimit;
	float		fFontSize;
	BMatrix		fMatrix;
};

struct named_color {
	const char *name;
	rgb_color color;
};

// SVGViewWindow class ------------------------------------------------------
class SVGViewView : public BView {

public:
 					SVGViewView(BRect frame, const char *name,
						const char *file);
virtual				~SVGViewView();

virtual	void		Draw(BRect updateRect);

private:
		rgb_color	GetColor(const char *name, uint8 alpha);
		named_color *GetGradient(TiXmlElement *element);

		void		CheckAttributes(TiXmlElement *element);
		void		DrawNode(TiXmlNode *node);

		void		Push();
		void		Pop();

		_state_		fState;
		BList		fStack;

		BList		fGradients;
		BPicture	*fPicture;

		TiXmlDocument	fDoc;
};
//------------------------------------------------------------------------------

#endif // _SVG_VIEW_VIEW_H
