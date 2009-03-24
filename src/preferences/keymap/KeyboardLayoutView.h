/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYBOARD_LAYOUT_VIEW_H
#define KEYBOARD_LAYOUT_VIEW_H


#include <View.h>

#include "KeyboardLayout.h"

class Keymap;


class KeyboardLayoutView : public BView {
public:
							KeyboardLayoutView(const char* name);
							~KeyboardLayoutView();

			void			SetKeyboardLayout(KeyboardLayout* layout);
			void			SetKeymap(Keymap* keymap);

protected:
	virtual	void			AttachedToWindow();
	virtual void			FrameResized(float width, float height);
//	virtual	BSize			MinSize();

	virtual	void			KeyDown(const char* bytes, int32 numBytes);
	virtual	void			KeyUp(const char* bytes, int32 numBytes);
	virtual	void			MouseDown(BPoint point);
	virtual	void			MouseUp(BPoint point);
	virtual	void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);

	virtual	void			Draw(BRect updateRect);
	virtual	void			MessageReceived(BMessage* message);

private:
	enum key_kind {
		kNormalKey,
		kSpecialKey,
		kSymbolKey
	};

			const char*		_SpecialKeyLabel(const key_map& map, uint32 code);
			const char*		_SpecialMappedKeySymbol(const char* bytes,
								size_t numBytes);
			const char*		_SpecialMappedKeyLabel(const char* bytes,
								size_t numBytes);
			bool			_FunctionKeyLabel(uint32 code, char* text,
								size_t textSize);
			void			_GetKeyLabel(Key* key, char* text, size_t textSize,
								key_kind& keyKind);
			bool			_IsKeyPressed(int32 code);
			Key*			_KeyForCode(int32 code);
			void			_InvalidateKey(int32 code);
			void			_KeyChanged(BMessage* message);
			Key*			_KeyAt(BPoint point);
			BRect			_FrameFor(Key* key);
			float			_FontSizeFor(BRect frame, const char* text);

			KeyboardLayout*	fLayout;
			Keymap*			fKeymap;

			uint8			fKeyState[16];
			int32			fModifiers;
			int32			fDeadKey;

			BPoint			fClickPoint;

			BFont			fFont;
			BFont			fSpecialFont;
			float			fFontHeight;
			float			fMaxFontSize;
			BPoint			fOffset;
			float			fFactor;
			float			fGap;
};

#endif	// KEYBOARD_LAYOUT_VIEW_H
