//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Rule.h
	Mime sniffer rule declarations
*/
#ifndef _sk_sniffer_rule_h_
#define _sk_sniffer_rule_h_

#include <List.h>

namespace Sniffer {

class Expr;

class Rule {
public:
	Rule();
	Rule(const char *rule);
	status_t SetTo(const char *rule);
private:
	float fPriority;
	BList fExprList;
};

}

#endif	// _sk_sniffer_rule_h_