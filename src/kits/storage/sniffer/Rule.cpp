//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Rule.cpp
	MIME sniffer rule implementation
*/

#include <sniffer/Rule.h>

using namespace Sniffer;

Rule::Rule()
	: fPriority(0.0)
	, fExprList(NULL)
{
}

Rule::~Rule() {
	Unset();	
}

void
Rule::Unset() {
 	if (fExprList){
		delete fExprList;
		fExprList = NULL;
	}
}

void
Rule::SetTo(double priority, ExprList* list) {
	Unset();
	fPriority = priority;
	fExprList = list;
}



