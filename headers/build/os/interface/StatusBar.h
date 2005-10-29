//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		StatusBar.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BStatusBar displays a "percentage-of-completion" gauge.
//------------------------------------------------------------------------------

#ifndef _STATUS_BAR_H
#define _STATUS_BAR_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <View.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BStatusBar class ------------------------------------------------------------
class BStatusBar : public BView
{
public:
	BStatusBar(BRect frame, const char *name, const char *label = NULL,
		const char *trailingLabel = NULL);
	BStatusBar(BMessage *archive);

	virtual ~BStatusBar();
	
	static BArchivable *Instantiate(BMessage *archive);
	virtual	status_t Archive(BMessage *archive, bool deep = true) const;

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void Draw(BRect updateRect);

	virtual void SetBarColor(rgb_color color);
	virtual void SetBarHeight(float height);
	virtual void SetText(const char *string);
	virtual void SetTrailingText(const char *string);
	virtual void SetMaxValue(float max);

	void Update(float delta, const char *text = NULL, const char *trailingText = NULL);
	virtual void Reset(const char *label = NULL, const char *trailingLabel = NULL);

	float CurrentValue() const;
	float MaxValue() const;
	rgb_color BarColor() const;
	float BarHeight() const;
	const char *Text() const;
	const char *TrailingText() const;
	const char *Label() const;
	const char *TrailingLabel() const;

	virtual	void MouseDown(BPoint point);
	virtual	void MouseUp(BPoint point);
	virtual	void WindowActivated(bool state);
	virtual	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual	void DetachedFromWindow();
	virtual	void FrameMoved(BPoint new_position);
	virtual	void FrameResized(float new_width, float new_height);

	virtual BHandler *ResolveSpecifier(BMessage *message, int32 index, BMessage *specifier,
		int32 what, const char *property);

	virtual void ResizeToPreferred();
	virtual void GetPreferredSize(float *width, float *height);
	virtual void MakeFocus(bool state = true);
	virtual void AllAttached();
	virtual void AllDetached();
	virtual status_t GetSupportedSuites(BMessage *data);

	virtual status_t Perform(perform_code d, void *arg);

private:
	virtual	void _ReservedStatusBar1();
	virtual	void _ReservedStatusBar2();
	virtual	void _ReservedStatusBar3();
	virtual	void _ReservedStatusBar4();

	BStatusBar &operator=(const BStatusBar &);

	void InitObject(const char *l, const char *aux_l);
	void SetTextData(char **pp, const char *str);
	void FillBar(BRect r);
	void Resize();
	void _Draw(BRect updateRect, bool bar_only);

	char *fLabel;
	char *fTrailingLabel;
	char *fText;
	char *fTrailingText;
	float fMax;
	float fCurrent;
	float fBarHeight;
	float fTrailingWidth;
	rgb_color fBarColor;
	float fEraseText;
	float fEraseTrailingText;
	bool fCustomBarHeight;
	bool _pad1;
	bool _pad2;
	bool _pad3;
	uint32 _reserved[3];
};

//------------------------------------------------------------------------------

#endif // _STATUS_BAR_H

/*
 * $Log $
 *
 * $Id  $
 *
 */
