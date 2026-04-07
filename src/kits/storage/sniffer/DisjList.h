/*
 * Copyright 2002, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 */

/*!
	\file sniffer/DisjList.h
	Mime Sniffer Disjunction List declarations
*/

#ifndef _SNIFFER_DISJ_LIST_H
#define _SNIFFER_DISJ_LIST_H

#include <sys/types.h>

namespace BPrivate {
namespace Storage {
namespace Sniffer {

struct Data;

//! Abstract class defining methods acting on a list of ORed patterns
class DisjList {
public:
	DisjList();
	virtual ~DisjList();

	virtual bool Sniff(const Data& data) const = 0;
	virtual ssize_t BytesNeeded() const = 0;
	
	void SetCaseInsensitive(bool how);
	bool IsCaseInsensitive();
protected:
	bool fCaseInsensitive;
};


}; // namespace Sniffer
}; // namespace Storage
}; // namespace BPrivate


#endif // _SNIFFER_DISJ_LIST_H
