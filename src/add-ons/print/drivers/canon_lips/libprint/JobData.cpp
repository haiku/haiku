/*
 * JobData.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <InterfaceDefs.h>
#include <Message.h>
#include "JobData.h"
#include "PrinterCap.h"
#include "DbgMsg.h"

const char *JD_XRES             = "xres";
const char *JD_YRES             = "yres";
const char *JD_COPIES           = "copies";
const char *JD_ORIENTATION      = "orientation";
const char *JD_SCALING          = "scaling";
const char *JD_PAPER_RECT       = "paper_rect";
const char *JD_FIRST_PAGE       = "first_page";
const char *JD_LAST_PAGE        = "last_page";
const char *JD_PRINTABLE_RECT   = "printable_rect";

const char *JD_PAPER            = "JJJJ_paper";
const char *JD_NUP              = "JJJJ_nup";
const char *JD_SURFACE_TYPE     = "JJJJ_surface_type";
const char *JD_GAMMA            = "JJJJ_gamma";
const char *JD_PAPER_SOURCE     = "JJJJ_paper_source";
const char *JD_COLLATE          = "JJJJ_collate";
const char *JD_REVERSE          = "JJJJ_reverse";
const char *JD_PRINT_STYLE      = "JJJJ_print_style";
const char *JD_BINDING_LOCATION = "JJJJ_binding_location";
const char *JD_PAGE_ORDER       = "JJJJ_page_order";

JobData::JobData(BMessage *msg, const PrinterCap *cap)
{
	load(msg, cap);
}

JobData::~JobData()
{
}

JobData::JobData(const JobData &job_data)
{
	__paper            = job_data.__paper;
	__xres             = job_data.__xres;
	__yres             = job_data.__yres;
	__orientation      = job_data.__orientation;
	__scaling          = job_data.__scaling;
	__paper_rect       = job_data.__paper_rect;
	__printable_rect   = job_data.__printable_rect;
	__nup              = job_data.__nup;
	__first_page       = job_data.__first_page;
	__last_page        = job_data.__last_page;
	__surface_type     = job_data.__surface_type;
	__gamma            = job_data.__gamma;
	__paper_source     = job_data.__paper_source;
	__copies           = job_data.__copies;
	__collate          = job_data.__collate;
	__reverse          = job_data.__reverse;
	__print_style      = job_data.__print_style;
	__binding_location = job_data.__binding_location;
	__page_order       = job_data.__page_order;
	__msg              = job_data.__msg;
}

JobData &JobData::operator = (const JobData &job_data)
{
	__paper            = job_data.__paper;
	__xres             = job_data.__xres;
	__yres             = job_data.__yres;
	__orientation      = job_data.__orientation;
	__scaling          = job_data.__scaling;
	__paper_rect       = job_data.__paper_rect;
	__printable_rect   = job_data.__printable_rect;
	__nup              = job_data.__nup;
	__first_page       = job_data.__first_page;
	__last_page        = job_data.__last_page;
	__surface_type     = job_data.__surface_type;
	__gamma            = job_data.__gamma;
	__paper_source     = job_data.__paper_source;
	__copies           = job_data.__copies;
	__collate          = job_data.__collate;
	__reverse          = job_data.__reverse;
	__print_style      = job_data.__print_style;
	__binding_location = job_data.__binding_location;
	__page_order       = job_data.__page_order;
	__msg              = job_data.__msg;
	return *this;
}

void JobData::load(BMessage *msg, const PrinterCap *cap)
{
	__msg = msg;

	if (msg->HasInt32(JD_PAPER))
		__paper = (PAPER)msg->FindInt32(JD_PAPER);
	else if (cap->isSupport(PrinterCap::PAPER))
		__paper = ((const PaperCap *)cap->getDefaultCap(PrinterCap::PAPER))->paper;
	else
		__paper = A4;

	if (msg->HasInt64(JD_XRES)) {
		int64 xres64; 
		msg->FindInt64(JD_XRES, &xres64);
		__xres = xres64; 
	} else if (cap->isSupport(PrinterCap::RESOLUTION)) {
		__xres = ((const ResolutionCap *)cap->getDefaultCap(PrinterCap::RESOLUTION))->xres;
	} else {
		__xres = 300; 
	}

	if (msg->HasInt64(JD_YRES)) {
		int64 yres64;
		msg->FindInt64(JD_YRES, &yres64);
		__yres = yres64;
	} else if (cap->isSupport(PrinterCap::RESOLUTION)) {
		__yres = ((const ResolutionCap *)cap->getDefaultCap(PrinterCap::RESOLUTION))->yres;
	} else {
		__yres = 300;
	}

	if (msg->HasInt32(JD_ORIENTATION))
		__orientation = (ORIENTATION)msg->FindInt32(JD_ORIENTATION);
	else if (cap->isSupport(PrinterCap::ORIENTATION))
		__orientation = ((const OrientationCap *)cap->getDefaultCap(PrinterCap::ORIENTATION))->orientation;
	else
		__orientation = PORTRAIT;

	if (msg->HasFloat(JD_SCALING))
		__scaling = msg->FindFloat(JD_SCALING);
	else
		__scaling = 100.0f;

	if (msg->HasRect(JD_PAPER_RECT)) {
		__paper_rect = msg->FindRect(JD_PAPER_RECT);
	}

	if (msg->HasRect(JD_PRINTABLE_RECT)) {
		__printable_rect = msg->FindRect(JD_PRINTABLE_RECT);
	}

	if (msg->HasInt32(JD_FIRST_PAGE))
		__first_page = msg->FindInt32(JD_FIRST_PAGE);
	else
		__first_page = 1;

	if (msg->HasInt32(JD_LAST_PAGE))
		__last_page = msg->FindInt32(JD_LAST_PAGE);
	else
		__last_page = -1;

	if (msg->HasInt32(JD_NUP))
		__nup = msg->FindInt32(JD_NUP);
	else
		__nup = 1;

	if (msg->HasInt32(JD_SURFACE_TYPE))
		__surface_type = (color_space)msg->FindInt32(JD_SURFACE_TYPE);
	else
		__surface_type = B_CMAP8;

	if (msg->HasFloat(JD_GAMMA))
		__gamma = __msg->FindFloat(JD_GAMMA);
	else
		__gamma = 1.4f;

	if (msg->HasInt32(JD_PAPER_SOURCE))
		__paper_source = (PAPERSOURCE)__msg->FindInt32(JD_PAPER_SOURCE);
	else if (cap->isSupport(PrinterCap::PAPERSOURCE))
		__paper_source = ((const PaperSourceCap *)cap->getDefaultCap(PrinterCap::PAPERSOURCE))->paper_source;
	else
		__paper_source = AUTO;

	if (msg->HasInt32(JD_COPIES))
		__copies = msg->FindInt32(JD_COPIES);
	else
		__copies = 1;

	if (msg->HasBool(JD_COLLATE))
		__collate = msg->FindBool(JD_COLLATE);
	else
		__collate = false;

	if (msg->HasBool(JD_REVERSE))
		__reverse = msg->FindBool(JD_REVERSE);
	else
		__reverse = false;

	if (msg->HasInt32(JD_PRINT_STYLE))
		__print_style = (PRINTSTYLE)msg->FindInt32(JD_PRINT_STYLE);
	else if (cap->isSupport(PrinterCap::PRINTSTYLE))
		__print_style = ((const PrintStyleCap *)cap->getDefaultCap(PrinterCap::PRINTSTYLE))->print_style;
	else
		__print_style = SIMPLEX;

	if (msg->HasInt32(JD_BINDING_LOCATION))
		__binding_location = (BINDINGLOCATION)msg->FindInt32(JD_BINDING_LOCATION);
	else if (cap->isSupport(PrinterCap::BINDINGLOCATION))
		__binding_location = ((const BindingLocationCap *)cap->getDefaultCap(PrinterCap::BINDINGLOCATION))->binding_location;
	else
		__binding_location = LONG_EDGE_LEFT;

	if (msg->HasInt32(JD_PAGE_ORDER))
		__page_order = (PAGEORDER)msg->FindInt32(JD_PAGE_ORDER);
	else
		__page_order = ACROSS_FROM_LEFT;
}

void JobData::save(BMessage *msg)
{
	if (msg == NULL) {
		msg = __msg;
	}

	if (msg->HasInt32(JD_PAPER))
		msg->ReplaceInt32(JD_PAPER, __paper);
	else
		msg->AddInt32(JD_PAPER, __paper);

	if (msg->HasInt64(JD_XRES))
		msg->ReplaceInt64(JD_XRES, __xres);
	else
		msg->AddInt64(JD_XRES, __xres);
	
	if (msg->HasInt64(JD_YRES))
		msg->ReplaceInt64(JD_YRES, __yres);
	else
		msg->AddInt64(JD_YRES, __yres);

	if (msg->HasInt32(JD_ORIENTATION))
		msg->ReplaceInt32(JD_ORIENTATION, __orientation);
	else
		msg->AddInt32(JD_ORIENTATION, __orientation);

	if (msg->HasFloat(JD_SCALING))
		msg->ReplaceFloat(JD_SCALING, __scaling);
	else
		msg->AddFloat(JD_SCALING, __scaling);

	if (msg->HasRect(JD_PAPER_RECT))
		msg->ReplaceRect(JD_PAPER_RECT, __paper_rect);
	else
		msg->AddRect(JD_PAPER_RECT, __paper_rect);

	if (msg->HasRect(JD_PRINTABLE_RECT))
		msg->ReplaceRect(JD_PRINTABLE_RECT, __printable_rect);
	else
		msg->AddRect(JD_PRINTABLE_RECT, __printable_rect);

	if (msg->HasInt32(JD_NUP))
		msg->ReplaceInt32(JD_NUP, __nup);
	else
		msg->AddInt32(JD_NUP, __nup);

	if (msg->HasInt32(JD_FIRST_PAGE))
		msg->ReplaceInt32(JD_FIRST_PAGE, __first_page);
	else
		msg->AddInt32(JD_FIRST_PAGE, __first_page);

	if (msg->HasInt32(JD_LAST_PAGE))
		msg->ReplaceInt32(JD_LAST_PAGE, __last_page);
	else
		msg->AddInt32(JD_LAST_PAGE, __last_page);

	if (msg->HasInt32(JD_SURFACE_TYPE))
		msg->ReplaceInt32(JD_SURFACE_TYPE, __surface_type);
	else
		msg->AddInt32(JD_SURFACE_TYPE, __surface_type);

	if (msg->HasFloat(JD_GAMMA))
		msg->ReplaceFloat(JD_GAMMA, __gamma);
	else
		msg->AddFloat(JD_GAMMA, __gamma);

	if (msg->HasInt32(JD_PAPER_SOURCE))
		msg->ReplaceInt32(JD_PAPER_SOURCE, __paper_source);
	else
		msg->AddInt32(JD_PAPER_SOURCE, __paper_source);

	if (msg->HasInt32(JD_COPIES))
		msg->ReplaceInt32(JD_COPIES, __copies);
	else
		msg->AddInt32(JD_COPIES, __copies);

	if (msg->HasBool(JD_COLLATE))
		msg->ReplaceBool(JD_COLLATE, __collate);
	else
		msg->AddBool(JD_COLLATE, __collate);

	if (msg->HasBool(JD_REVERSE))
		msg->ReplaceBool(JD_REVERSE, __reverse);
	else
		msg->AddBool(JD_REVERSE, __reverse);

	if (msg->HasInt32(JD_PRINT_STYLE))
		msg->ReplaceInt32(JD_PRINT_STYLE, __print_style);
	else
		msg->AddInt32(JD_PRINT_STYLE, __print_style);

	if (msg->HasInt32(JD_BINDING_LOCATION))
		msg->ReplaceInt32(JD_BINDING_LOCATION, __binding_location);
	else
		msg->AddInt32(JD_BINDING_LOCATION, __binding_location);

	if (msg->HasInt32(JD_PAGE_ORDER))
		msg->ReplaceInt32(JD_PAGE_ORDER, __page_order);
	else
		msg->AddInt32(JD_PAGE_ORDER, __page_order);
}
