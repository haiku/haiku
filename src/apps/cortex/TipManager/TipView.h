// TipView.h
// * PURPOSE
//   Provide a basic, extensible 'ToolTip' view, designed
//   to be hosted by a floating window (TipWindow).
// * HISTORY
//   e.moon		20oct99		multi-line support
//   e.moon		17oct99		Begun.

#ifndef __TipView_H__
#define __TipView_H__

#include <Font.h>
#include <String.h>
#include <View.h>

#include <vector>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TipWindow;

class TipView :
	public	BView {
	typedef	BView _inherited;
	
public:											// *** dtor/ctors
	virtual ~TipView();
	TipView();

public:											// *** operations

	// if attached to a BWindow, the window must be locked
	virtual void setText(
		const char*							text);
		
public:											// *** BView

	virtual void Draw(
		BRect										updateRect);

	virtual void FrameResized(
		float										width,
		float										height);
		
	virtual void GetPreferredSize(
		float*									outWidth,
		float*									outHeight);

private:										// implementation
	BString										m_text;
	BPoint										m_offset;

	BFont											m_font;
	font_height								m_fontHeight;
	
	rgb_color									m_textColor;
	rgb_color									m_borderLoColor;
	rgb_color									m_borderHiColor;
	rgb_color									m_viewColor;


	typedef vector<int32> line_set;
	line_set									m_lineSet;

private:
	static const float				s_xPad;
	static const float				s_yPad;

	void _initColors();
	void _initFont();
	void _updateLayout(
		float										width,
		float										height);
	
	void _setText(
		const char*							text);
		
	float _maxTextWidth();
	float _textHeight();
};

__END_CORTEX_NAMESPACE
#endif /*__TipView_H__*/