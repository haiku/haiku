//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Err.h
	Mime Sniffer Error class declarations
*/
#ifndef _sk_sniffer_err_h_
#define _sk_sniffer_err_h_

#include <SupportDefs.h>
#include <string>

namespace Sniffer {

//! Exception class used by the MIME Sniffer
class Err {
public:
	Err(const char *msg, const ssize_t pos);
	Err(const std::string &msg, const ssize_t pos);
	Err(const Err &ref);
	virtual ~Err();	
	Err& operator=(const Err &ref);
	
	status_t SetTo(const char *msg, const ssize_t pos);
	status_t SetTo(const std::string &msg, const ssize_t pos);
	void Unset();
	
	const char* Msg() const;
	ssize_t Pos() const;
	
private:
	friend class Parser;	// So it can update the pos of its out of mem object
	void SetMsg(const char *msg);
	void SetPos(ssize_t pos);
	char *fMsg;
	ssize_t fPos;
};

}

#endif	// _sk_sniffer_err_h_