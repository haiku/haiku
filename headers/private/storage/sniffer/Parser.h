//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Parser.h
	MIME sniffer rule parser declarations
*/
#ifndef _sk_sniffer_parser_h_
#define _sk_sniffer_parser_h_

#include <SupportDefs.h>

class BString;

namespace Sniffer {
class Rule;
status_t parse(const char *rule, Rule *result, BString *parseError = NULL);
}

#endif	// _sk_sniffer_parser_h_

