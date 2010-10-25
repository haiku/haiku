/*
 * PrinterCap.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PRINTERCAP_H
#define __PRINTERCAP_H

#include <string>
#include <Rect.h>
#include "JobData.h"

using namespace std;

enum {
	kUnknownPrinter = 0
};

struct BaseCap {
							BaseCap(const string &label, bool isDefault);

			string			fLabel;
			bool			fIsDefault;
};

struct PaperCap : public BaseCap {
							PaperCap(const string &label, bool isDefault,
								JobData::Paper paper, const BRect &paperRect,
								const BRect &physicalRect);

			JobData::Paper	fPaper;
			BRect			fPaperRect;
			BRect			fPhysicalRect;
};

struct PaperSourceCap : public BaseCap {
							PaperSourceCap(const string &label, bool isDefault,
								JobData::PaperSource paperSource);

			JobData::PaperSource	fPaperSource;
};

struct ResolutionCap : public BaseCap {
							ResolutionCap(const string &label, bool isDefault,
								int xResolution, int yResolution);

			int				fXResolution;
			int				fYResolution;
};

struct OrientationCap : public BaseCap {
							OrientationCap(const string &label, bool isDefault,
									JobData::Orientation orientation);

			JobData::Orientation	fOrientation;
};

struct PrintStyleCap : public BaseCap {
							PrintStyleCap(const string &label, bool isDefault,
								JobData::PrintStyle printStyle);

			JobData::PrintStyle		fPrintStyle;
};

struct BindingLocationCap : public BaseCap {
							BindingLocationCap(const string &label,
								bool isDefault,
								JobData::BindingLocation bindingLocation);

			JobData::BindingLocation	fBindingLocation;
};

struct ColorCap : public BaseCap {
							ColorCap(const string &label, bool isDefault,
								JobData::Color color);

			JobData::Color	fColor;
};

struct ProtocolClassCap : public BaseCap {
							ProtocolClassCap(const string &label,
								bool isDefault, int protocolClass,
								const string &description);

			int			fProtocolClass;
			string		fDescription;
};


class PrinterData;

class PrinterCap {
public:
							PrinterCap(const PrinterData *printer_data);
	virtual					~PrinterCap();

	enum CapID {
		kPaper,
		kPaperSource,
		kResolution,
		kOrientation,
		kPrintStyle,
		kBindingLocation,
		kColor,
		kProtocolClass,
		// Static boolean settings follow.
		// For them isSupport() has to be implemented only.
		kCopyCommand,       // supports printer page copy command?
	};

	virtual	int				countCap(CapID) const = 0;
	virtual	bool			isSupport(CapID) const = 0;
	virtual	const BaseCap**	enumCap(CapID) const = 0;
			const BaseCap*	getDefaultCap(CapID) const;
			int				getPrinterId() const;
			int				getProtocolClass() const;

protected:
							PrinterCap(const PrinterCap &);
			PrinterCap&		operator=(const PrinterCap &);

			const PrinterData*	getPrinterData() const;
			void				setPrinterId(int id);

private:
			const PrinterData*	fPrinterData;
			int					fPrinterID;
};


#endif	/* __PRINTERCAP_H */
