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

	virtual	int32			ID() const = 0;
			const char*		Key() const;

			string			fLabel;
			bool			fIsDefault;
			string			fKey;
};

struct PaperCap : public BaseCap {
							PaperCap(const string &label, bool isDefault,
								JobData::Paper paper, const BRect &paperRect,
								const BRect &physicalRect);

			int32			ID() const;

			JobData::Paper	fPaper;
			BRect			fPaperRect;
			BRect			fPhysicalRect;
};

struct PaperSourceCap : public BaseCap {
							PaperSourceCap(const string &label, bool isDefault,
								JobData::PaperSource paperSource);

			int32			ID() const;

			JobData::PaperSource	fPaperSource;
};

struct ResolutionCap : public BaseCap {
							ResolutionCap(const string &label, bool isDefault,
								int32 id, int xResolution, int yResolution);

			int32			ID() const;

			int32			fID;
			int				fXResolution;
			int				fYResolution;
};

struct OrientationCap : public BaseCap {
							OrientationCap(const string &label, bool isDefault,
									JobData::Orientation orientation);

			int32			ID() const;

			JobData::Orientation	fOrientation;
};

struct PrintStyleCap : public BaseCap {
							PrintStyleCap(const string &label, bool isDefault,
								JobData::PrintStyle printStyle);

			int32			ID() const;

			JobData::PrintStyle		fPrintStyle;
};

struct BindingLocationCap : public BaseCap {
							BindingLocationCap(const string &label,
								bool isDefault,
								JobData::BindingLocation bindingLocation);

			int32			ID() const;

			JobData::BindingLocation	fBindingLocation;
};

struct ColorCap : public BaseCap {
							ColorCap(const string &label, bool isDefault,
								JobData::Color color);

			int32			ID() const;

			JobData::Color	fColor;
};

struct ProtocolClassCap : public BaseCap {
							ProtocolClassCap(const string &label,
								bool isDefault, int32 protocolClass,
								const string &description);

			int32			ID() const;

			int32		fProtocolClass;
			string		fDescription;
};


struct DriverSpecificCap : public BaseCap {
		enum Type {
			kList,
			kCheckBox,
			kRange
		};

							DriverSpecificCap(const string& label,
								int32 category, Type type);

			int32			ID() const;

			int32			fCategory;
			Type			fType;
};

struct ListItemCap : public BaseCap {
							ListItemCap(const string& label,
								bool isDefault, int32 id);

			int32			ID() const;
			int32			fID;
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
		kDriverSpecificCapabilities,

		// Static boolean settings follow.
		// For them isSupport() has to be implemented only.
		kCopyCommand,	// supports printer page copy command?
		kHalftone,		// needs the printer driver the configuration
						// for class Halftone?

		// The driver specific generic capabilities start here
		kDriverSpecificCapabilitiesBegin = 100
	};

	struct IDPredicate
	{
		IDPredicate(int id)
			: fID(id)
		{
		}


		bool operator()(const BaseCap* baseCap) {
			return baseCap->ID() == fID;
		}

		int fID;
	};

	struct LabelPredicate
	{
		LabelPredicate(const char* label)
			: fLabel(label)
		{
		}


		bool operator()(const BaseCap* baseCap) {
			return baseCap->fLabel == fLabel;
		}

		const char* fLabel;

	};

	struct KeyPredicate
	{
		KeyPredicate(const char* key)
			: fKey(key)
		{
		}


		bool operator()(const BaseCap* baseCap) {
			return baseCap->fKey == fKey;
		}

		const char* fKey;

	};

	virtual	int				countCap(CapID category) const = 0;
	virtual	bool			isSupport(CapID category) const = 0;
	virtual	const BaseCap**	enumCap(CapID category) const = 0;
			const BaseCap*	getDefaultCap(CapID category) const;
			const BaseCap*	findCap(CapID category, int id) const;
			const BaseCap*	findCap(CapID category, const char* label) const;
			const BaseCap*	findCapWithKey(CapID category, const char* key)
								const;

			int				getProtocolClass() const;

protected:
							PrinterCap(const PrinterCap& printerCap);
			PrinterCap&		operator=(const PrinterCap& printerCap);
			template<typename Predicate>
			const BaseCap*	findCap(CapID category, Predicate& predicate) const;

			const PrinterData*	getPrinterData() const;

private:
			const PrinterData*	fPrinterData;
};




#endif	/* __PRINTERCAP_H */
