// RosterTestAppDefs.h
#ifndef ROSTER_TEST_APP_DEF_H
#define ROSTER_TEST_APP_DEF_H

// message codes
enum {
	MSG_STARTED				= 'strt',	// "path": B_STRING_TYPE
	MSG_TERMINATED			= 'term',	//
	MSG_MAIN_ARGS			= 'args',	// "argc": B_INT32_TYPE,
										// "argv": B_STRING_TYPE[]
	MSG_ARGV_RECEIVED		= 'argv',	// "argc": B_INT32_TYPE,
										// "argv": B_STRING_TYPE[]
	MSG_REFS_RECEIVED		= 'refs',	// "refs": B_REF_TYPE[]
	MSG_MESSAGE_RECEIVED	= 'mesg',	// "message": B_MESSAGE_TYPE
	MSG_QUIT_REQUESTED		= 'quit',	//
	MSG_READY_TO_RUN		= 'redy',	//
};

#endif	// ROSTER_TEST_APP_DEF_H
