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
	virtual					~BaseCap();

	virtual	int				ID() const = 0;

			string			fLabel;
			bool			fIsDefault;
			string			fKey;
};

struct PaperCap : public BaseCap {
							PaperCap(const string &label, bool isDefault,
								JobData::Paper paper, const BRect &paperRect,
								const BRect &physicalRect);

			int				ID() const;

			JobData::Paper	fPaper;
			BRect			fPaperRect;
			BRect			fPhysicalRect;
};

struct PaperSourceCap : public BaseCap {
							PaperSourceCap(const string &label, bool isDefault,
								JobData::PaperSource paperSource);

			int				ID() const;

			JobData::PaperSource	fPaperSource;
};

struct ResolutionCap : public BaseCap {
							ResolutionCap(const string &label, bool isDefault,
								int id, int xResolution, int yResolution);

			int				ID() const;

			int				fID;
			int				fXResolution;
			int				fYResolution;
};

struct OrientationCap : public BaseCap {
							OrientationCap(const string &label, bool isDefault,
									JobData::Orientation orientation);

			int				ID() const;

			JobData::Orientation	fOrientation;
};

struct PrintStyleCap : public BaseCap {
							PrintStyleCap(const string &label, bool isDefault,
								JobData::PrintStyle printStyle);

			int				ID() const;

			JobData::PrintStyle		fPrintStyle;
};

struct BindingLocationCap : public BaseCap {
							BindingLocationCap(const string &label,
								bool isDefault,
								JobData::BindingLocation bindingLocation);

			int				ID() const;

			JobData::BindingLocation	fBindingLocation;
};

struct ColorCap : public BaseCap {
							ColorCap(const string &label, bool isDefault,
								JobData::Color color);

			int				ID() const;

			JobData::Color	fColor;
};

struct ProtocolClassCap : public BaseCap {
							ProtocolClassCap(const string &label,
								bool isDefault, int protocolClass,
								const string &description);

			int				ID() const;

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

	virtual	int				countCap(CapID category) const = 0;
	virtual	bool			isSupport(CapID category) const = 0;
	virtual	const BaseCap**	enumCap(CapID category) const = 0;
			const BaseCap*	getDefaultCap(CapID category) const;
			const BaseCap*	findCap(CapID category, int id) const;
			const BaseCap*	findCap(CapID category, const char* label) const;

			int				getPrinterId() const;
			int				getProtocolClass() const;

protected:
							PrinterCap(const PrinterCap &printerCap);
			PrinterCap&		operator=(const PrinterCap &printerCap);

			const PrinterData*	getPrinterData() const;
			void				setPrinterId(int id);

private:
			const PrinterData*	fPrinterData;
			int					fPrinterID;
};


#endif	/* __PRINTERCAP_H */
