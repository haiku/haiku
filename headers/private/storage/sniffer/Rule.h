//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/Rule.h
	Mime sniffer rule declarations
*/
#ifndef _SNIFFER_RULE_H
#define _SNIFFER_RULE_H

#include <SupportDefs.h>

#include <sys/types.h>
#include <vector>

class BPositionIO;

namespace BPrivate {
namespace Storage {
namespace Sniffer {

class DisjList;

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
	ssize_t BytesNeeded() const;
private:
	friend class Parser;

	void Unset();
	void SetTo(double priority, std::vector<DisjList*>* list);

	double fPriority;
	std::vector<DisjList*> *fConjList;	// A list of DisjLists to be ANDed
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_RULE_H


