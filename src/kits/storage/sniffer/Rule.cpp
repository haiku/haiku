//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Rule.cpp
	MIME sniffer rule implementation
*/

#include <sniffer/Err.h>
#include <sniffer/DisjList.h>
#include <sniffer/Rule.h>
#include <DataIO.h>

using namespace BPrivate::Storage::Sniffer;

Rule::Rule()
	: fPriority(0.0)
	, fConjList(NULL)
{
}

Rule::~Rule() {
	Unset();	
}

status_t
Rule::InitCheck() const {
	return fConjList ? B_OK : B_NO_INIT;
}

//! Returns the priority of the rule. 0.0 <= priority <= 1.0.
double
Rule::Priority() const {
	return fPriority;
}

//! Sniffs the given data stream. Returns true if the rule matches, false if not.
bool
Rule::Sniff(BPositionIO *data) const {
	if (InitCheck() != B_OK)
		return false;
	else {
		bool result = true;
		std::vector<DisjList*>::const_iterator i;
		for (i = fConjList->begin(); i != fConjList->end(); i++) {
			if (*i)
				result &= (*i)->Sniff(data);		
		}
		return result;
	}
}

void
Rule::Unset() {
 	if (fConjList){
		delete fConjList;
		fConjList = NULL;
	}
}

//! Called by Parser::Parse() after successfully parsing a sniffer rule.
void
Rule::SetTo(double priority, std::vector<DisjList*>* list) {
	Unset();
	if (0.0 <= priority && priority <= 1.0)
		fPriority = priority;
	else
		throw new Err("Sniffer pattern error: invalid priority", -1);
	fConjList = list;
}






