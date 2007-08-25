/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_VIEW_H
#define SUDOKU_VIEW_H


#include <View.h>

class SudokuField;


enum {
	kMarkValidHints = 0x01,
	kMarkInvalid = 0x02,
};


class SudokuView : public BView {
public:
	SudokuView(BRect frame, const char* name, const BMessage& settings,
		uint32 resizingMode);
	virtual ~SudokuView();

	status_t SaveState(BMessage& state);

	status_t SetTo(entry_ref& ref);
	status_t SetTo(const char* data);
	status_t SetTo(SudokuField* field);

	status_t SaveTo(entry_ref& ref, bool asText);

	void ClearChanged();
	void ClearAll();

	void SetHintFlags(uint32 flags);
	uint32 HintFlags() const { return fHintFlags; }

	SudokuField* Field() { return fField; }

	void SetEditable(bool editable);
	bool Editable() const { return fEditable; }

protected:
	virtual void AttachedToWindow();

	virtual void FrameResized(float width, float height);
	virtual void MouseDown(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage);
	virtual void KeyDown(const char *bytes, int32 numBytes);

	virtual void MessageReceived(BMessage* message);

	virtual void Draw(BRect updateRect);

private:
	status_t _FilterString(const char* data, size_t dataLength, char* buffer,
		uint32& out, bool& ignore);
	void _SetText(char* text, uint32 value);
	char _BaseCharacter();
	bool _ValidCharacter(char c);
	BPoint _LeftTop(uint32 x, uint32 y);
	BRect _Frame(uint32, uint32 y);
	void _InvalidateHintField(uint32 x, uint32 y, uint32 hintX, uint32 hintY);
	void _InvalidateField(uint32 x, uint32 y);
	void _InvalidateKeyboardFocus(uint32 x, uint32 y);
	void _InsertKey(char rawKey, int32 modifiers);
	void _RemoveHint();
	bool _GetHintFieldFor(BPoint where, uint32 x, uint32 y,
		uint32& hintX, uint32& hintY);
	bool _GetFieldFor(BPoint where, uint32& x, uint32& y);
	void _FitFont(BFont& font, float width, float height);
	void _DrawKeyboardFocus();
	void _DrawHints(uint32 x, uint32 y);

	SudokuField*	fField;
	uint32			fBlockSize;
	float			fWidth, fHeight, fBaseline;
	BFont			fFieldFont;
	BFont			fHintFont;
	float			fHintHeight, fHintWidth, fHintBaseline;
	uint32			fShowHintX, fShowHintY;
	uint32			fLastHintValue;
	uint32			fLastField;
	uint32			fKeyboardX, fKeyboardY;
	uint32			fHintFlags;
	bool			fShowKeyboardFocus;
	bool			fShowCursor;
	bool			fEditable;
};

static const uint32 kMsgSudokuSolved = 'susl';
static const uint32 kMsgSolveSudoku = 'slvs';
static const uint32 kMsgSolveSingle = 'slsg';

#endif	// SUDOKU_VIEW_H
