//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_REPORT_DEFS__H
#define _K_PPP_REPORT_DEFS__H

#include <PPPReportDefs.h>


typedef struct ppp_report_request {
	thread_id thread;
	int32 type;
	int32 flags;
} ppp_report_request;


#endif
