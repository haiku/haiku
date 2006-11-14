#ifndef _SVGVIEW_VIEW_H
#define _SVGVIEW_VIEW_H

// Standard Includes -----------------------------------------------------------
#include <expat.h>

// System Includes -------------------------------------------------------------
#include <InterfaceKit.h>
#include <SupportKit.h>
#include <TranslationUtils.h>

// Project Includes ------------------------------------------------------------
#include "Matrix.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
struct named_color {
	const char  *name;
	rgb_color   color;
};

struct named_gradient {
	const char  *name;
	rgb_color   color;
	bool        started;
};

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
		fCurrentColor.red = 0; fCurrentColor.green = 0;
		fCurrentColor.blue = 0; fCurrentColor.alpha = 255;
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
	rgb_color	fCurrentColor;
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

// Globals ---------------------------------------------------------------------

// Svg2PictureView class -------------------------------------------------------
class Svg2PictureView : public BView {

public:
				Svg2PictureView(BRect frame, const char *fileName);
				~Svg2PictureView();

	virtual void		AttachedToWindow();
        virtual void    	Draw(BRect updateRect);

private:
        bool    HasAttribute(const XML_Char **attributes, const char *name);
        float   GetFloatAttribute(const XML_Char **attributes, const char *name);
        const char  *GetStringAttribute(const XML_Char **attributes, const char *name);
        rgb_color   GetColorAttribute(const XML_Char **attributes, const char *name, uint8 alpha);
        void    GetPolygonAttribute(const XML_Char **attributes, const char *name, BShape &shape);
        void    GetMatrixAttribute(const XML_Char **attributes, const char *name, BMatrix *matrix);
        void    GetShapeAttribute(const XML_Char **attributes, const char *name, BShape &shape);
        void    CheckAttributes(const XML_Char **attributes);
        void    StartElement(const XML_Char *name, const XML_Char **attributes);
        void    EndElement(const XML_Char *name);
        void    CharacterDataHandler(const XML_Char *s, int len);

        void    Push();
        void    Pop();

static  void    _StartElement(Svg2PictureView *view, const XML_Char *name, const XML_Char **attributes);
static  void    _EndElement(Svg2PictureView *view, const XML_Char *name);
static  void    _CharacterDataHandler(Svg2PictureView *view, const XML_Char *s, int len);

        _state_ fState;
        BList   fStack;
        named_gradient *fGradient;
        BList   fGradients;
        BPoint  fTextPosition;
        BString fText;

	BString fFileName;
	BPicture *fPicture;
};
//------------------------------------------------------------------------------

#endif // _SVGVIEW_VIEW_H
