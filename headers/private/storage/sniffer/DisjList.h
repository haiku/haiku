//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/DisjList.h
	Mime Sniffer Disjunction List declarations
*/
#ifndef _SNIFFER_DISJ_LIST_H
#define _SNIFFER_DISJ_LIST_H

#include <sys/types.h>

class BPositionIO;

namespace BPrivate {
namespace Storage {
namespace Sniffer {

//! Abstract class defining methods acting on a list of ORed patterns
class DisjList {
public:
	DisjList();
	virtual ~DisjList();

	virtual bool Sniff(BPositionIO *data) const = 0;
	virtual ssize_t BytesNeeded() const = 0;
	
	void SetCaseInsensitive(bool how);
	bool IsCaseInsensitive();
protected:
	bool fCaseInsensitive;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_DISJ_LIST_H


