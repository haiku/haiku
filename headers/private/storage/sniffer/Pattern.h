//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Pattern.h
	Mime Sniffer Pattern declarations
*/
#ifndef _sk_sniffer_pattern_h_
#define _sk_sniffer_pattern_h_

#include <SupportDefs.h>
#include <string>
#include <sniffer/Range.h>

class BPositionIO;

namespace Sniffer {

class Err;

//! A byte string and optional mask to be compared against a data stream.
/*! The byte string and mask (if supplied) must be of the same length. */
class Pattern {
public:
	Pattern(const std::string &string, const std::string &mask);
	Pattern(const std::string &string);
	~Pattern();
	
	status_t InitCheck() const;
	Err* GetErr() const;
	
	bool Sniff(Range range, BPositionIO *data) const;
	
	status_t SetTo(const std::string &string, const std::string &mask);
private:
	bool Sniff(off_t start, off_t size, BPositionIO *data) const;
	
	void SetStatus(status_t status, const char *msg = NULL);
	void SetErrorMessage(const char *msg);

	std::string fString;
	std::string fMask;

	status_t fCStatus;
	Err *fErrorMessage;
};

}

#endif	// _sk_sniffer_pattern_h_