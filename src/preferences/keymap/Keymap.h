/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <InterfaceDefs.h>
#include <Entry.h>


class Keymap {
public:
			status_t		Load(entry_ref& ref);
			status_t		Save(entry_ref& ref);
			void			DumpKeymap();
			bool			IsModifierKey(uint32 keyCode);
			uint8			IsDeadKey(uint32 keyCode, uint32 modifiers);
			bool			IsDeadSecondKey(uint32 keyCode, uint32 modifiers,
								uint8 activeDeadKey);
			void			GetChars(uint32 keyCode, uint32 modifiers,
								uint8 activeDeadKey, char** chars,
								int32* numBytes);
			status_t		Use();
			bool			Equals(const Keymap& map) const;

			const key_map&	Map() const { return fKeys; }

private:
			char*			fChars;
			key_map			fKeys;
			uint32			fCharsSize;
			char			fName[B_FILE_NAME_LENGTH];
};


#endif //KEYMAP_H
