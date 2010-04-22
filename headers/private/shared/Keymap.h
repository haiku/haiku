/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef _KEYMAP_H
#define _KEYMAP_H


#include <DataIO.h>
#include <InterfaceDefs.h>


class BKeymap {
public:
								BKeymap();
	virtual						~BKeymap();

			status_t			SetTo(const char* path);
			status_t			SetTo(BDataIO& stream);
			status_t			SetToCurrent();
			status_t			SetToDefault();
			void				Unset();

			bool				IsModifierKey(uint32 keyCode) const;
			uint32				Modifier(uint32 keyCode) const;
			uint32				KeyForModifier(uint32 modifier) const;
			uint8				ActiveDeadKey(uint32 keyCode,
									uint32 modifiers) const;
			uint8				DeadKey(uint32 keyCode, uint32 modifiers,
									bool* isEnabled = NULL) const;
			bool				IsDeadSecondKey(uint32 keyCode,
									uint32 modifiers,
									uint8 activeDeadKey) const;
			void				GetChars(uint32 keyCode, uint32 modifiers,
									uint8 activeDeadKey, char** chars,
									int32* numBytes) const;

			const key_map&		Map() const { return fKeys; }

			bool				operator==(const BKeymap& other) const;
			bool				operator!=(const BKeymap& other) const;

			BKeymap&			operator=(const BKeymap& other);

protected:
			int32				Offset(uint32 keyCode, uint32 modifiers,
									uint32* _table = NULL) const;
			uint8				DeadKeyIndex(int32 offset) const;

protected:
			char*				fChars;
			key_map				fKeys;
			uint32				fCharsSize;
};


#endif	// KEYMAP_H
