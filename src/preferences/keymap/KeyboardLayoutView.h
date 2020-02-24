/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 */
#ifndef KEYBOARD_LAYOUT_VIEW_H
#define KEYBOARD_LAYOUT_VIEW_H


#include <Messenger.h>
#include <View.h>

#include "KeyboardLayout.h"


class BMenuItem;
class Keymap;


class KeyboardLayoutView : public BView {
public:
								KeyboardLayoutView(const char* name);
								~KeyboardLayoutView();

			void				SetKeyboardLayout(KeyboardLayout* layout);
			void				SetKeymap(Keymap* keymap);
			void				SetTarget(BMessenger target);

			KeyboardLayout*		GetKeyboardLayout() { return fLayout; }

			void				SetBaseFont(const BFont& font);

			void				SetEditable(bool editable);

protected:
	virtual void				AttachedToWindow();
	virtual void				FrameResized(float width, float height);
	virtual BSize				MinSize();

	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				KeyUp(const char* bytes, int32 numBytes);
	virtual void				MouseDown(BPoint point);
	virtual void				MouseUp(BPoint point);
	virtual void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);

	virtual void				Draw(BRect updateRect);
	virtual void				MessageReceived(BMessage* message);
	virtual void				WindowActivated(bool active);

private:
			enum key_kind {
				kNormalKey,
				kSpecialKey,
				kSymbolKey,
				kIndicator
			};

			void				_LayoutKeyboard();
			void				_DrawKeyButton(BView* view, BRect& rect,
									BRect updateRect, rgb_color base,
									rgb_color background, bool pressed);
			void				_DrawKey(BView* view, BRect updateRect,
									const Key* key, BRect frame, bool pressed);
			void				_DrawIndicator(BView* view, BRect updateRect,
									const Indicator* indicator, BRect rect,
									bool lit);
			const char*			_SpecialKeyLabel(const key_map& map,
									uint32 code, bool abbreviated = false);
			const char*			_SpecialMappedKeySymbol(const char* bytes,
									size_t numBytes);
			const char*			_SpecialMappedKeyLabel(const char* bytes,
									size_t numBytes, bool abbreviated = false);
			bool				_FunctionKeyLabel(uint32 code, char* text,
									size_t textSize);
			void				_GetAbbreviatedKeyLabelIfNeeded(BView* view,
									BRect rect, const Key* key, char* text,
									size_t textSize);
			void				_GetKeyLabel(const Key* key, char* text,
									size_t textSize, key_kind& keyKind);
			bool				_IsKeyPressed(uint32 code);
			bool				_KeyState(uint32 code) const;
			void				_SetKeyState(uint32 code, bool pressed);
			Key*				_KeyForCode(uint32 code);
			void				_InvalidateKey(uint32 code);
			void				_InvalidateKey(const Key* key);
			bool				_HandleDeadKey(uint32 key, int32 modifiers);
			void				_KeyChanged(const BMessage* message);
			Key*				_KeyAt(BPoint point);
			BRect				_FrameFor(BRect keyFrame);
			BRect				_FrameFor(const Key* key);
			void				_SetFontSize(BView* view, key_kind keyKind);
			void				_EvaluateDropTarget(BPoint point);
			void				_SendFakeKeyDown(const Key* key);

			BMenuItem*			_CreateSwapModifiersMenuItem(uint32 modifier,
									uint32 displayModifier, uint32 oldCode,
									uint32 newCode);
			const char*			_NameForModifier(uint32 modifier, bool pretty);

			KeyboardLayout*		fLayout;
			Keymap*				fKeymap;
			BMessenger			fTarget;
			bool				fEditable;

			uint8				fKeyState[16];
			int32				fModifiers;
			int32				fDeadKey;
			int32				fButtons;

			BPoint				fClickPoint;
			Key*				fDragKey;
			int32				fDragModifiers;
			Key*				fDropTarget;
			BPoint				fDropPoint;

			BSize				fOldSize;
			BFont				fBaseFont;
			BFont				fSpecialFont;
			float				fBaseFontHeight;
			float				fBaseFontSize;
			BPoint				fOffset;
			float				fFactor;
			float				fGap;
};


#endif	// KEYBOARD_LAYOUT_VIEW_H
