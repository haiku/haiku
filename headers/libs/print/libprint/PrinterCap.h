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
							BaseCap(const string &label);
	virtual					~BaseCap();

			const char*		Label() const;

			string			fLabel;
};

struct EnumCap : public BaseCap {
							EnumCap(const string& label, bool isDefault);

	virtual	int32			ID() const = 0;
			const char*		Key() const;

			bool			fIsDefault;
			string			fKey;
};


struct PaperCap : public EnumCap {
							PaperCap(const string &label, bool isDefault,
								JobData::Paper paper, const BRect &paperRect,
								const BRect &physicalRect);

			int32			ID() const;

			JobData::Paper	fPaper;
			BRect			fPaperRect;
			BRect			fPhysicalRect;
};

struct PaperSourceCap : public EnumCap {
							PaperSourceCap(const string &label, bool isDefault,
								JobData::PaperSource paperSource);

			int32			ID() const;

			JobData::PaperSource	fPaperSource;
};

struct ResolutionCap : public EnumCap {
							ResolutionCap(const string &label, bool isDefault,
								int32 id, int xResolution, int yResolution);

			int32			ID() const;

			int32			fID;
			int				fXResolution;
			int				fYResolution;
};

struct OrientationCap : public EnumCap {
							OrientationCap(const string &label, bool isDefault,
									JobData::Orientation orientation);

			int32			ID() const;

			JobData::Orientation	fOrientation;
};

struct PrintStyleCap : public EnumCap {
							PrintStyleCap(const string &label, bool isDefault,
								JobData::PrintStyle printStyle);

			int32			ID() const;

			JobData::PrintStyle		fPrintStyle;
};

struct BindingLocationCap : public EnumCap {
							BindingLocationCap(const string &label,
								bool isDefault,
								JobData::BindingLocation bindingLocation);

			int32			ID() const;

			JobData::BindingLocation	fBindingLocation;
};

struct ColorCap : public EnumCap {
							ColorCap(const string &label, bool isDefault,
								JobData::Color color);

			int32			ID() const;

			JobData::Color	fColor;
};

struct ProtocolClassCap : public EnumCap {
							ProtocolClassCap(const string &label,
								bool isDefault, int32 protocolClass,
								const string &description);

			int32			ID() const;

			int32		fProtocolClass;
			string		fDescription;
};


struct DriverSpecificCap : public EnumCap {
		enum Type {
			kList,
			kBoolean,
			kIntRange,
			kIntDimension,
			kDoubleRange
		};

							DriverSpecificCap(const string& label,
								int32 category, Type type);

			int32			ID() const;

			int32			fCategory;
			Type			fType;
};

struct ListItemCap : public EnumCap {
							ListItemCap(const string& label,
								bool isDefault, int32 id);

			int32			ID() const;

private:
			int32			fID;
};

struct BooleanCap : public BaseCap {
							BooleanCap(const string& label, bool defaultValue);

			bool			DefaultValue() const;

private:
			bool			fDefaultValue;
};

struct IntRangeCap : public BaseCap {
							IntRangeCap(const string& label, int lower,
								int upper, int defaultValue);

			int32			Lower() const;
			int32			Upper() const;
			int32			DefaultValue() const;

private:
			int32			fLower;
			int32			fUpper;
			int32			fDefaultValue;
};

struct DoubleRangeCap : public BaseCap {
							DoubleRangeCap(const string& label, double lower,
								double upper, double defaultValue);

			double			Lower() const;
			double			Upper() const;
			double			DefaultValue() const;

			double			fLower;
			double			fUpper;
			double			fDefaultValue;
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
		// For them Supports() has to be implemented only.
		kCopyCommand,	// supports printer page copy command?
		kHalftone,		// needs the printer driver the configuration
						// for class Halftone?
		kCanRotatePageInLandscape,
						// can the printer driver rotate the page
						// printing in landscape

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
			const EnumCap* enumCap = dynamic_cast<const EnumCap*>(baseCap);
			if (enumCap == NULL)
				return false;
			return enumCap->ID() == fID;
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
			const EnumCap* enumCap = dynamic_cast<const EnumCap*>(baseCap);
			if (enumCap == NULL)
				return false;
			return enumCap->fKey == fKey;
		}

		const char* fKey;

	};

	virtual	int				CountCap(CapID category) const = 0;
	virtual	bool			Supports(CapID category) const = 0;
	virtual	const BaseCap**	GetCaps(CapID category) const = 0;

			const EnumCap*	GetDefaultCap(CapID category) const;
			const EnumCap*	FindCap(CapID category, int id) const;
			const BaseCap*	FindCap(CapID category, const char* label) const;
			const EnumCap*	FindCapWithKey(CapID category, const char* key)
								const;

			const BooleanCap*		FindBooleanCap(CapID category) const;
			const IntRangeCap*		FindIntRangeCap(CapID category) const;
			const DoubleRangeCap*	FindDoubleRangeCap(CapID category) const;

			int				GetProtocolClass() const;

protected:
							PrinterCap(const PrinterCap& printerCap);
			PrinterCap&		operator=(const PrinterCap& printerCap);
			template<typename Predicate>
			const BaseCap*	FindCap(CapID category, Predicate& predicate) const;

			const PrinterData*	GetPrinterData() const;

private:
			const PrinterData*	fPrinterData;
};




#endif	/* __PRINTERCAP_H */
