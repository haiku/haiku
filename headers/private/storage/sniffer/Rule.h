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

#include <vector>

namespace Sniffer {

class Expr;
typedef std::vector<Expr*> ExprList;

class Rule {
public:
	Rule();
	~Rule();
private:
	friend class Parser;

	void Unset();
	void SetTo(double priority, ExprList* list);

	double fPriority;
	ExprList *fExprList;
};

}

#endif	// _sk_sniffer_rule_h_