//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/PatternList.h
	MIME sniffer pattern list declarations
*/
#ifndef _sk_sniffer_pattern_list_h_
#define _sk_sniffer_pattern_list_h_

#include <sniffer/Expr.h>
#include <sniffer/Range.h>
#include <vector>

class BPositionIO;

namespace Sniffer {

class Err;
class Pattern;

class PatternList : public Expr {
public:
	PatternList(Range range);
	virtual ~PatternList();

	status_t InitCheck() const;
	Err* GetErr() const;
	
	virtual bool Sniff(BPositionIO *data) const;
	void Add(Pattern *pattern);
private:
	std::vector<Pattern*> fList;
	Range fRange;
};

}

#endif	// _sk_sniffer_pattern_list_h_