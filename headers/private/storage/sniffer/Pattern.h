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

//! Abstract class definining an interface for sniffing BPositionIO objects
class Pattern {
public:
	Pattern(const char *string, const char *mask = NULL);
	~Pattern();
	
	status_t InitCheck() const;
	Err* GetErr() const;
	
	bool Sniff(Range range, BPositionIO *data) const;
	
	status_t SetTo(const char *string, const char *mask = NULL);
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