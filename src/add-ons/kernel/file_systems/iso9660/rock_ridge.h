/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef ROCK_RIDGE_H
#define ROCK_RIDGE_H


// Altername name field flags.
enum NMFLAGS {
	NM_CONTINUE = 1,
	NM_CURRENT = 2,
	NM_PARENT = 4,
	NM_HOST = 32
};

// Symbolic link field flags
enum SLFLAGS {
	SL_CONTINUE = 1
};

// Symbolic link field component flags
enum SLCPFLAGS {
	SLCP_CONTINUE = 1,
	SLCP_CURRENT = 2,
	SLCP_PARENT = 4,
	SLCP_ROOT = 8,
	SLCP_VOLROOT = 16,
	SLCP_HOST = 32
};

#endif	// ROCK_RIDGE_H
