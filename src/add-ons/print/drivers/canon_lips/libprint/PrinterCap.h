/*
 * PrinterCap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PRINTERCAP_H
#define __PRINTERCAP_H

#include <string>
#include <Rect.h>
#include "JobData.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

#define UNKNOWN_PRINTER	0

struct BaseCap {
	string label;
	bool is_default;
	BaseCap(const string &n, bool d) : label(n), is_default(d) {}
};

struct PaperCap : public BaseCap {
	JobData::PAPER paper;
	BRect paper_rect;
	BRect printable_rect;
	PaperCap(const string &n, bool d, JobData::PAPER p, const BRect &r1, const BRect &r2)
		: BaseCap(n, d), paper(p), paper_rect(r1), printable_rect(r2) {}
};

struct PaperSourceCap : public BaseCap {
	JobData::PAPERSOURCE paper_source;
	PaperSourceCap(const string &n, bool d, JobData::PAPERSOURCE f)
		: BaseCap(n, d), paper_source(f) {}
};

struct ResolutionCap : public BaseCap {
	int xres;
	int yres;
	ResolutionCap(const string &n, bool d, int x, int y)
		: BaseCap(n, d), xres(x), yres(y) {}
};

struct OrientationCap : public BaseCap {
	JobData::ORIENTATION orientation;
	OrientationCap(const string &n, bool d, JobData::ORIENTATION o)
		: BaseCap(n, d), orientation(o) {}
};

struct PrintStyleCap : public BaseCap {
	JobData::PRINTSTYLE print_style;
	PrintStyleCap(const string &n, bool d, JobData::PRINTSTYLE x)
		: BaseCap(n, d), print_style(x) {}
};

struct BindingLocationCap : public BaseCap {
	JobData::BINDINGLOCATION binding_location;
	BindingLocationCap(const string &n, bool d, JobData::BINDINGLOCATION b)
		: BaseCap(n, d), binding_location(b) {}
};

class PrinterData;

class PrinterCap {
public:
	PrinterCap(const PrinterData *printer_data);
	virtual ~PrinterCap();
/*
	PrinterCap(const PrinterCap &printer_cap);
	PrinterCap &operator = (const PrinterCap &printer_cap);
*/
	enum CAPID {
		PAPER,
		PAPERSOURCE,
		RESOLUTION,
		ORIENTATION,
		PRINTSTYLE,
		BINDINGLOCATION
	};

	virtual int countCap(CAPID) const = 0;
	virtual bool isSupport(CAPID) const = 0;
	virtual const BaseCap **enumCap(CAPID) const = 0;
	const BaseCap *getDefaultCap(CAPID) const;
	int getPrinterId() const;

protected:
	PrinterCap(const PrinterCap &);
	PrinterCap &operator = (const PrinterCap &);
	const PrinterData *getPrinterData() const;
	void setPrinterId(int id);

private:
	const PrinterData *__printer_data;
	int __printer_id;
};

inline const PrinterData *PrinterCap::getPrinterData() const
{
	return __printer_data;
}

inline int PrinterCap::getPrinterId() const
{
	return __printer_id;
}

inline void PrinterCap::setPrinterId(int id)
{
	__printer_id = id;
}

#endif	/* __PRINTERCAP_H */
