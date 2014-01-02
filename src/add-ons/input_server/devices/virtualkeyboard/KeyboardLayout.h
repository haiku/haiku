/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H


#include <map>

#include <Entry.h>
#include <ObjectList.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>


enum key_shape {
	kRectangleKeyShape,
	kCircleKeyShape,
	kEnterKeyShape
};

struct Key {
	int32		code;
	int32		alternate_code[3];
	int32		alternate_modifier[3];
	key_shape	shape;
	BRect		frame;
	float		second_row;
		// this is the width of the second row of a kEnterKeyShape key
	bool		dark;
};

struct Indicator {
	int32		modifier;
	BRect		frame;
};

typedef std::map<BString, BString> VariableMap;

class KeyboardLayout {
public:
							KeyboardLayout();
							~KeyboardLayout();

			const char*		Name();

			int32			CountKeys();
			Key*			KeyAt(int32 index);

			int32			CountIndicators();
			Indicator*		IndicatorAt(int32 index);

			BRect			Bounds();
			BSize			DefaultKeySize();
			int32			IndexForModifier(int32 modifier);

			status_t		Load(const char* path);
			status_t		Load(entry_ref& ref);

			void			SetDefault();
			bool			IsDefault() const { return fIsDefault; }

private:
	enum parse_mode {
		kPairs,
		kSize,
		kRowStart,
		kKeyShape,
		kKeyCodes
	};
	struct parse_state {
		parse_mode	mode;
		int32		line;
	};

			void			_FreeKeys();
			void			_Error(const parse_state& state,
								const char* reason, ...);
			void			_AddAlternateKeyCode(Key* key, int32 modifier,
								int32 code);
			bool			_AddKey(const Key& key);
			void			_SkipCommentsAndSpace(parse_state& state,
								const char*& data);
			void			_Trim(BString& string, bool stripComments);
			bool			_GetPair(const parse_state& state,
								const char*& data, BString& name,
								BString& value);
			bool			_AddKeyCodes(const parse_state& state,
								BPoint& rowLeftTop, Key& key, const char* data,
								int32& lastCount);
			bool			_GetSize(const parse_state& state, const char* data,
								float& x, float& y, float* _secondRow = NULL);
			bool			_GetShape(const parse_state& state,
								const char* data, Key& key);
			const char*		_Delimiter(parse_mode mode);
			bool			_GetTerm(const char*& data, const char* delimiter,
								BString& term, bool closingBracketAllowed);
			bool			_SubstituteVariables(BString& term,
								VariableMap& variables, BString& unknown);
			bool			_ParseTerm(const parse_state& state,
								const char*& data, BString& term,
								VariableMap& variables);

			status_t		_InitFrom(const char* data);

			BString			fName;
			Key*			fKeys;
			int32			fKeyCount;
			int32			fKeyCapacity;
			BRect			fBounds;
			BSize			fDefaultKeySize;
			int32			fAlternateIndex[3];
			BObjectList<Indicator> fIndicators;
			bool			fIsDefault;
};

#endif	// KEYBOARD_LAYOUT_H
