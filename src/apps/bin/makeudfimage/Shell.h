//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Shell.h
*/

#ifndef _SHELL_H
#define _SHELL_H

#include <string>

#include "SupportDefs.h"

class Shell {
public:
	Shell();
	void Run(int argc, char *argv[]);
private:
	status_t _ProcessArguments(int argc, char *argv[]);
	status_t _ProcessArgument(std::string arg, int argc, char *argv[]);
	void _PrintHelp();
};


#endif	// _SHELL_H
