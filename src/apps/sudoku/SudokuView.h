/*
 * Copyright 2007-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_VIEW_H
#define SUDOKU_VIEW_H


#include <View.h>
#include <ObjectList.h>


class BDataIO;
class SudokuField;
struct entry_ref;


enum {
	kMarkValidHints = 0x01,
	kMarkInvalid = 0x02,
};


enum {
	kExportAsText,
	kExportAsHTML,
	kExportAsBitmap,
	kExportAsPicture
};


class SudokuView : public BView {
public:
								SudokuView(BRect frame, const char* name,
									const BMessage& settings,
									uint32 resizingMode);
								SudokuView(BMessage* archive);
	virtual						~SudokuView();

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);
			void				InitObject(const BMessage* archive);

			status_t			SaveState(BMessage& state) const;

			status_t			SetTo(entry_ref& ref);
			status_t			SetTo(const char* data);
			status_t			SetTo(SudokuField* field);

			status_t			SaveTo(entry_ref& ref,
									uint32 as = kExportAsText);
			status_t			SaveTo(BDataIO &to, uint32 as = kExportAsText);

			status_t			CopyToClipboard();

			void				ClearChanged();
			void				ClearAll();

			void				SetHintFlags(uint32 flags);
			uint32				HintFlags() const { return fHintFlags; }

			SudokuField*		Field() { return fField; }

			void				SetEditable(bool editable);
			bool				Editable() const { return fEditable; }

			bool				CanUndo() { return !fUndos.IsEmpty(); }
			bool				CanRedo() { return !fRedos.IsEmpty(); }
			void				Undo();
			void				Redo();

protected:
	virtual	void				AttachedToWindow();

	virtual void				FrameResized(float width, float height);
	virtual void				MouseDown(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				KeyDown(const char *bytes, int32 numBytes);

	virtual void				MessageReceived(BMessage* message);

	virtual void				Draw(BRect updateRect);

private:
			status_t			_FilterString(const char* data,
									size_t dataLength, char* buffer,
									uint32& out, bool& ignore);
			void				_SetText(char* text, uint32 value);
			char				_BaseCharacter();
			bool				_ValidCharacter(char c);
			BPoint				_LeftTop(uint32 x, uint32 y);
			BRect				_Frame(uint32, uint32 y);
			void				_InvalidateHintField(uint32 x, uint32 y,
									uint32 hintX, uint32 hintY);
			void				_InvalidateField(uint32 x, uint32 y);
			void				_InvalidateKeyboardFocus(uint32 x, uint32 y);
			void				_InsertKey(char rawKey, int32 modifiers);
			void				_RemoveHint();
			bool				_GetHintFieldFor(BPoint where, uint32 x,
									uint32 y, uint32& hintX, uint32& hintY);
			bool				_GetFieldFor(BPoint where, uint32& x,
									uint32& y);
			void				_FitFont(BFont& font, float width,
									float height);
			void				_DrawKeyboardFocus();
			void				_DrawHints(uint32 x, uint32 y);
			void				_UndoRedo(BObjectList<BMessage>& undos,
									BObjectList<BMessage>& redos);
			void				_PushUndo();

private:
			rgb_color			fBackgroundColor;
			SudokuField*		fField;
			BObjectList<BMessage> fUndos;
			BObjectList<BMessage> fRedos;
			uint32				fBlockSize;
			float				fWidth;
			float				fHeight;
			float				fBaseline;
			BFont				fFieldFont;
			BFont				fHintFont;
			float				fHintHeight;
			float				fHintWidth;
			float				fHintBaseline;
			uint32				fShowHintX;
			uint32				fShowHintY;
			uint32				fLastHintValue;
			bool				fLastHintValueSet;
			uint32				fLastField;
			uint32				fKeyboardX;
			uint32				fKeyboardY;
			uint32				fHintFlags;
			bool				fShowKeyboardFocus;
			bool				fShowCursor;
			bool				fEditable;
};


static const uint32 kMsgSudokuSolved = 'susl';
static const uint32 kMsgSolveSudoku = 'slvs';
static const uint32 kMsgSolveSingle = 'slsg';

// you can observe these:
static const int32 kUndoRedoChanged = 'unre';


#endif	// SUDOKU_VIEW_H
