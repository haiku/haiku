//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Expr.h
	Mime Sniffer Expression declarations
*/
#ifndef _sk_sniffer_expr_h_
#define _sk_sniffer_expr_h_

namespace Sniffer {

class BFile;

//! Abstract class definining an interface for sniffing BFile objects
class Expr {
public:
	virtual ~Expr() {}
	virtual bool Sniff(BFile *file) = 0;
};

}

#endif	// _sk_sniffer_expr_h_