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

enum {
	kUnknownPrinter = 0
};

struct BaseCap {
	string label;
	bool is_default;
	BaseCap(const string &n, bool d) : label(n), is_default(d) {}
};

struct PaperCap : public BaseCap {
	JobData::Paper paper;
	BRect paper_rect;
	BRect printable_rect;
	PaperCap(const string &n, bool d, JobData::Paper p, const BRect &r1, const BRect &r2)
		: BaseCap(n, d), paper(p), paper_rect(r1), printable_rect(r2) {}
};

struct PaperSourceCap : public BaseCap {
	JobData::PaperSource paper_source;
	PaperSourceCap(const string &n, bool d, JobData::PaperSource f)
		: BaseCap(n, d), paper_source(f) {}
};

struct ResolutionCap : public BaseCap {
	int xres;
	int yres;
	ResolutionCap(const string &n, bool d, int x, int y)
		: BaseCap(n, d), xres(x), yres(y) {}
};

struct OrientationCap : public BaseCap {
	JobData::Orientation orientation;
	OrientationCap(const string &n, bool d, JobData::Orientation o)
		: BaseCap(n, d), orientation(o) {}
};

struct PrintStyleCap : public BaseCap {
	JobData::PrintStyle print_style;
	PrintStyleCap(const string &n, bool d, JobData::PrintStyle x)
		: BaseCap(n, d), print_style(x) {}
};

struct BindingLocationCap : public BaseCap {
	JobData::BindingLocation binding_location;
	BindingLocationCap(const string &n, bool d, JobData::BindingLocation b)
		: BaseCap(n, d), binding_location(b) {}
};

struct ColorCap : public BaseCap {
	JobData::Color color;
	ColorCap(const string &n, bool d, JobData::Color c)
		: BaseCap(n, d), color(c) {}
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
	enum CapID {
		kPaper,
		kPaperSource,
		kResolution,
		kOrientation,
		kPrintStyle,
		kBindingLocation,
		kColor
	};

	virtual int countCap(CapID) const = 0;
	virtual bool isSupport(CapID) const = 0;
	virtual const BaseCap **enumCap(CapID) const = 0;
	const BaseCap *getDefaultCap(CapID) const;
	int getPrinterId() const;

protected:
	PrinterCap(const PrinterCap &);
	PrinterCap &operator = (const PrinterCap &);
	const PrinterData *getPrinterData() const;
	void setPrinterId(int id);

private:
	const PrinterData *fPrinterData;
	int fPrinterID;
};

inline const PrinterData *PrinterCap::getPrinterData() const
{
	return fPrinterData;
}

inline int PrinterCap::getPrinterId() const
{
	return fPrinterID;
}

inline void PrinterCap::setPrinterId(int id)
{
	fPrinterID = id;
}

#endif	/* __PRINTERCAP_H */
