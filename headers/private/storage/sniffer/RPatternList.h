//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/RPatternList.h
	MIME sniffer rpattern list declarations
*/
#ifndef _sk_sniffer_r_pattern_list_h_
#define _sk_sniffer_r_pattern_list_h_

#include <sniffer/Expr.h>
#include <vector>

class BPositionIO;

namespace Sniffer {

class Err;	
class RPattern;

class RPatternList : public Expr {
public:
	RPatternList();
	virtual ~RPatternList();
	
	status_t InitCheck() const;
	Err* GetErr() const;
	
	virtual bool Sniff(BPositionIO *data) const;
	void Add(RPattern *rpattern);
private:
	std::vector<RPattern*> fList;
};

}

#endif	// _sk_sniffer_r_pattern_list_h_