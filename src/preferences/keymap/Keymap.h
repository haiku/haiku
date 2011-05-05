/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <Keymap.h>

#include <Entry.h>
#include <Messenger.h>
#include <String.h>


enum dead_key_index {
	kDeadKeyAcute = 1,
	kDeadKeyGrave,
	kDeadKeyCircumflex,
	kDeadKeyDiaeresis,
	kDeadKeyTilde
};


class Keymap : public BKeymap {
public:
								Keymap();
								~Keymap();

			void				SetTarget(BMessenger target,
									BMessage* modificationMessage);

			status_t			Load(const entry_ref& ref);
			status_t			Save(const entry_ref& ref);

			void				DumpKeymap();

			status_t			SetModifier(uint32 keyCode, uint32 modifier);

			void				SetDeadKeyEnabled(uint32 keyCode,
									uint32 modifiers, bool enabled);
			void				GetDeadKeyTrigger(dead_key_index deadKeyIndex,
									BString& outTrigger);
			void				SetDeadKeyTrigger(dead_key_index deadKeyIndex,
									const BString& trigger);

			status_t			Use();

			void				SetKey(uint32 keyCode, uint32 modifiers,
									int8 deadKey, const char* bytes,
									int32 numBytes = -1);

			void				SetName(const char* name);

			const key_map&		Map() const { return fKeys; }
			key_map&			Map() { return fKeys; }

			Keymap&				operator=(const Keymap& other);

private:
			bool				_SetChars(int32 offset, const char* bytes,
									int32 numBytes);

private:
			char				fName[B_FILE_NAME_LENGTH];

			BMessenger			fTarget;
			BMessage*			fModificationMessage;
};


#endif	// KEYMAP_H
