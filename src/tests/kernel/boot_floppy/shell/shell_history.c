#include <stdio.h>
#include <string.h>

#include "shell_history.h"



static char Table[MAX_HISTORY_DEPTH][120]; // holds MAX_HISTORY_DEPTH slots
                                           // each slot up to 120 chars long

static int  NumEntries = 0;   // current number of filled entries in Table
static int  Slot  = 0;        // index of next slot in Table to look at




void store_history_command(char *cmd_string)
{
	// store the command string in the next available slot

	char *next = Table[0];
	int   i;
	
	// find the next empty slot (if there is one)
	//
	for (i = 0; i < MAX_HISTORY_DEPTH; ++i) {
	
		if (Slot == MAX_HISTORY_DEPTH)
			// wrap around
			Slot = 0;
		
		next = Table[Slot++];
		if (*next == '\0')
			// found empty slot
			break;
	}
	
	// copy in the command string
	strcpy (next, cmd_string);
	
	
	// update count
	++NumEntries;
	if (NumEntries > MAX_HISTORY_DEPTH)
		NumEntries = MAX_HISTORY_DEPTH;
}


char *fetch_history_command(void)
{
	// fetch the previous command string
	
	char *prev = "";
	int   i;
	
	for (i = 0; i < NumEntries; ++i) {
		
		if (Slot == 0)
			// wrap around
			Slot = NumEntries;
		
		prev = Table[--Slot];
		if (*prev)
			// found non-blank entry
			break;
	}
	
	return prev;
}
