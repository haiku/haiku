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

#include <SupportDefs.h>
#include <vector>

class BPositionIO;

namespace Sniffer {

class Expr;

/*! \brief A priority and a list of expressions to be used for sniffing out the
	type of an untyped file.
*/
class Rule {
public:
	Rule();
	~Rule();
	
	status_t InitCheck() const;
	
	double Priority() const;	
	bool Sniff(BPositionIO *data) const;	
private:
	friend class Parser;

	void Unset();
	void SetTo(double priority, std::vector<Expr*>* list);

	double fPriority;
	std::vector<Expr*> *fExprList;
};

}

#endif	// _sk_sniffer_rule_h_