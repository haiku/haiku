//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Rule.cpp
	MIME sniffer rule implementation
*/

#include <sniffer/Expr.h>
#include <sniffer/Rule.h>
#include <DataIO.h>

using namespace Sniffer;

Rule::Rule()
	: fPriority(0.0)
	, fExprList(NULL)
{
}

Rule::~Rule() {
	Unset();	
}

status_t
Rule::InitCheck() const {
	return fExprList ? B_OK : B_NO_INIT;
}


double
Rule::Priority() const {
	return fPriority;
}

bool
Rule::Sniff(BPositionIO *data) const {
	if (InitCheck() != B_OK)
		return false;
	else {
		bool result = true;
		std::vector<Expr*>::const_iterator i;
		for (i = fExprList->begin(); i != fExprList->end(); i++) {
			if (*i)
				result &= (*i)->Sniff(data);		
		}
	}
}

void
Rule::Unset() {
 	if (fExprList){
		delete fExprList;
		fExprList = NULL;
	}
}

void
Rule::SetTo(double priority, std::vector<Expr*>* list) {
	Unset();
	fPriority = priority;
	fExprList = list;
}



