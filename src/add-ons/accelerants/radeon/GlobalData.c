/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Global data
*/


#include "GlobalData.h"

// the sample driver stores everything in global variables;
// I dislike this idea as this makes supporting multiple graphics
// card impossible; to be prepared, only the following variable is used
accelerator_info *ai;

int debug_level_flow = 2;
int debug_level_info = 4;
int debug_level_error = 4;
