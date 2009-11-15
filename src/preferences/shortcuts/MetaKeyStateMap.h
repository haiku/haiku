/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef MetaKeyStateMap_h
#define MetaKeyStateMap_h


#include <List.h>
#include <SupportDefs.h>


class BitFieldTester;


// This class defines a set of possible chording states (e.g. "Left only", 
// "Right only", "Both", "Either") for a meta-key (e.g. Shift), and the 
// description strings and qualifier bit-chords that go with them.
class MetaKeyStateMap {
public:
	
			// Note: You MUST call SetInfo() directly after using this ctor!
							MetaKeyStateMap();

			// Creates a MetaKeyStateMap with the give name 
			// (e.g. "Shift" or "Ctrl")
							MetaKeyStateMap(const char* keyName);


							~MetaKeyStateMap();

			// For when you have to use the default ctor
			void			SetInfo(const char* keyName);

			// (tester) becomes property of this map!
			void			AddState(const char* desc,
								const BitFieldTester* tester);

			// Returns the name of the meta-key (e.g. "Ctrl")
	const	char*			GetName() const;

			// Returns the number of possible states contained in this 
			// MetaKeyStateMap.
			int				GetNumStates() const;

			// Returns a BitFieldTester that tests for the nth state's 
			// presence.
	const	BitFieldTester*	GetNthStateTester(int stateNum) const;

			// Returns a textual description of the nth state (e.g. "Left")
	const	char*			GetNthStateDesc(int stateNum) const;

private:
			// e.g. "Alt" or "Ctrl"
			char*			fKeyName;

			// list of strings e.g. "Left" or "Both"
			BList			fStateDescs;

			// list of BitFieldTesters for testing bits of modifiers in state
			BList			fStateTesters;
};


#endif

