// rock.h

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// Altername name field flags.
enum
{
	NM_CONTINUE = 1,
	NM_CURRENT = 2,
	NM_PARENT = 4,
	NM_HOST = 32
} NMFLAGS;

// Symbolic link field flags
enum
{
	SL_CONTINUE = 1
} SLFLAGS;

// Symbolic link field component flags
enum
{
	SLCP_CONTINUE = 1,
	SLCP_CURRENT = 2, 
	SLCP_PARENT = 4, 
	SLCP_ROOT = 8,
	SLCP_VOLROOT = 16, 
	SLCP_HOST = 32
} SLCPFLAGS;
