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

class BPositionIO;

namespace Sniffer {

//! Base expression class
class Expr {
public:
	virtual ~Expr() {}
	virtual bool Sniff(BPositionIO *data) const = 0;
};

}

#endif	// _sk_sniffer_expr_h_