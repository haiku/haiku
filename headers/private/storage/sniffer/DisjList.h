//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/DisjList.h
	Mime Sniffer Disjunction List declarations
*/
#ifndef _SNIFFER_DISJ_LIST_H
#define _SNIFFER_DISJ_LIST_H

class BPositionIO;

namespace Sniffer {

//! Abstract class defining methods acting on a list of ORed patterns
class DisjList {
public:
	DisjList();
	virtual ~DisjList();
	virtual bool Sniff(BPositionIO *data) const = 0;
	
	void SetCaseInsensitive(bool how);
	bool IsCaseInsensitive();
protected:
	bool fCaseInsensitive;
};

}

#endif	// _SNIFFER_DISJ_LIST_H