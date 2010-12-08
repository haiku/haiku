/*
 * JobData.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBDATA_H
#define __JOBDATA_H

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <Rect.h>

#include <map>
#include <string>

#include "Halftone.h"
#include "MarginView.h" // for MarginUnit

class BMessage;
class PrinterCap;


using namespace std;

class DriverSpecificSettings
{
public:
	DriverSpecificSettings();
	DriverSpecificSettings(const DriverSpecificSettings& Settings);

	DriverSpecificSettings &operator=(const DriverSpecificSettings &Settings);

	void MakeEmpty();

	bool HasString(const char* key) const;
	const char* GetString(const char* key) const;
	void SetString(const char* key, const char* value);

	bool HasBoolean(const char* ekey) const;
	bool GetBoolean(const char* key) const;
	void SetBoolean(const char* key, bool value);

	bool HasInt(const char* ekey) const;
	int32 GetInt(const char* key) const;
	void SetInt(const char* key, int32 value);

	bool HasDouble(const char* ekey) const;
	double GetDouble(const char* key) const;
	void SetDouble(const char* key, double value);

	BMessage& Message();

private:
	BMessage fSettings;
};


class JobData {
public:
	enum Orientation {
		kPortrait,
		kLandscape
	};

	enum Paper {
		kLetter = 1,            //   1  Letter 8 1/2 x 11 in
		kLetterSmall,           //   2  Letter Small 8 1/2 x 11 in
		kTabloid,               //   3  Tabloid 11 x 17 in
		kLedger,                //   4  Ledger 17 x 11 in
		kLegal,                 //   5  Legal 8 1/2 x 14 in
		kStatement,             //   6  Statement 5 1/2 x 8 1/2 in
		kExecutive,             //   7  Executive 7 1/4 x 10 1/2 in
		kA3,                    //   8  A3 297 x 420 mm
		kA4,                    //   9  A4 210 x 297 mm
		kA4Small,               //  10  A4 Small 210 x 297 mm
		kA5,                    //  11  A5 148 x 210 mm
		kB4,                    //  12  B4 (JIS) 250 x 354
		kB5,                    //  13  B5 (JIS) 182 x 257 mm
		kFolio,                 //  14  Folio 8 1/2 x 13 in
		kQuarto,                //  15  Quarto 215 x 275 mm
		k10X14,                 //  16  10x14 in
		k11X17,                 //  17  11x17 in
		kNote,                  //  18  Note 8 1/2 x 11 in
		kEnv9,                  //  19  Envelope #9 3 7/8 x 8 7/8
		kEnv10,                 //  20  Envelope #10 4 1/8 x 9 1/2
		kEnv11,                 //  21  Envelope #11 4 1/2 x 10 3/8
		kEnv12,                 //  22  Envelope #12 4 \276 x 11
		kEnv14,                 //  23  Envelope #14 5 x 11 1/2
		kCSheet,                //  24  C size sheet
		kDSheet,                //  25  D size sheet
		kESheet,                //  26  E size sheet
		kEnvDL,                 //  27  Envelope DL 110 x 220mm
		kEnvC5,                 //  28  Envelope C5 162 x 229 mm
		kEnvC3,                 //  29  Envelope C3  324 x 458 mm
		kEnvC4,                 //  30  Envelope C4  229 x 324 mm
		kEnvC6,                 //  31  Envelope C6  114 x 162 mm
		kEnvC65,                //  32  Envelope C65 114 x 229 mm
		kEnvB4,                 //  33  Envelope B4  250 x 353 mm
		kEnvB5,                 //  34  Envelope B5  176 x 250 mm
		kEnvB6,                 //  35  Envelope B6  176 x 125 mm
		kEnvItaly,              //  36  Envelope 110 x 230 mm
		kEnvMonarch,            //  37  Envelope Monarch 3.875 x 7.5 in
		kEnvPersonal,           //  38  6 3/4 Envelope 3 5/8 x 6 1/2 in
		kFanFoldUS,             //  39  US Std Fanfold 14 7/8 x 11 in
		kFanFoldStdGerman,      //  40  German Std Fanfold 8 1/2 x 12 in
		kFanFoldLglGerman,      //  41  German Legal Fanfold 8 1/2 x 13 in
		kIsoB4,                 //  42  B4 (ISO) 250 x 353 mm
		kJapanesePostcard,      //  43  Japanese Postcard 100 x 148 mm
		k9X11,                  //  44  9 x 11 in
		k10X11,                 //  45  10 x 11 in
		k15X11,                 //  46  15 x 11 in
		kEnvInvite,             //  47  Envelope Invite 220 x 220 mm
		kReserved48,            //  48  RESERVED--DO NOT USE
		kReserved49,            //  49  RESERVED--DO NOT USE
		kLetterExtra,	        //  50  Letter Extra 9 \275 x 12 in
		kLegalExtra, 	        //  51  Legal Extra 9 \275 x 15 in
		kTabloidExtra,	        //  52  Tabloid Extra 11.69 x 18 in
		kA4Extra,     	        //  53  A4 Extra 9.27 x 12.69 in
		kLetterTransverse,      //  54  Letter Transverse 8 \275 x 11 in
		kA4Transverse,          //  55  A4 Transverse 210 x 297 mm
		kLetterExtraTransverse, //  56  Letter Extra Transverse 9\275 x 12 in
		kAPlus,                 //  57  SuperA/SuperA/A4 227 x 356 mm
		kBPlus,                 //  58  SuperB/SuperB/A3 305 x 487 mm
		kLetterPlus,            //  59  Letter Plus 8.5 x 12.69 in
		kA4Plus,                //  60  A4 Plus 210 x 330 mm
		kA5Transverse,          //  61  A5 Transverse 148 x 210 mm
		kB5Transverse,          //  62  B5 (JIS) Transverse 182 x 257 mm
		kA3Extra,               //  63  A3 Extra 322 x 445 mm
		kA5Extra,               //  64  A5 Extra 174 x 235 mm
		kB5Extra,               //  65  B5 (ISO) Extra 201 x 276 mm
		kA2,                    //  66  A2 420 x 594 mm
		kA3Transverse,          //  67  A3 Transverse 297 x 420 mm
		kA3ExtraTransverse,     //  68  A3 Extra Transverse 322 x 445 mm
		kDBLJapanesePostcard,   //  69  Japanese Double Postcard 200 x 148 mm
		kA6,                    //  70  A6 105 x 148 mm
		kJEnvKaku2,             //  71  Japanese Envelope Kaku #2
		kJEnvKaku3,             //  72  Japanese Envelope Kaku #3
		kJEnvChou3,             //  73  Japanese Envelope Chou #3
		kJEnvChou4,             //  74  Japanese Envelope Chou #4
		kLetterRotated,         //  75  Letter Rotated 11 x 8 1/2 11 in
		kA3Rotated,             //  76  A3 Rotated 420 x 297 mm
		kA4Rotated,             //  77  A4 Rotated 297 x 210 mm
		kA5Rotated,             //  78  A5 Rotated 210 x 148 mm
		kB4JISRotated,          //  79  B4 (JIS) Rotated 364 x 257 mm
		kB5JISRotated,          //  80  B5 (JIS) Rotated 257 x 182 mm
		kJapanesePostcardRotated, //  81 Japanese Postcard Rotated 148 x 100 mm
		kDBLJapanesePostcardRotated, // 82 Double Japanese Postcard Rotated 148 x 200 mm
		kA6Rotated,             //  83  A6 Rotated 148 x 105 mm
		kJEnvKaku2Rotated,      //  84  Japanese Envelope Kaku #2 Rotated
		kJEnvKaku3Rotated,      //  85  Japanese Envelope Kaku #3 Rotated
		kJEnvChou3Rotated,      //  86  Japanese Envelope Chou #3 Rotated
		kJEnvChou4Rotated,      //  87  Japanese Envelope Chou #4 Rotated
		kB6JIS,                 //  88  B6 (JIS) 128 x 182 mm
		kB6JISRotated,          //  89  B6 (JIS) Rotated 182 x 128 mm
		k12X11,                 //  90  12 x 11 in
		kJEnvYou4,              //  91  Japanese Envelope You #4
		kJEnvYou4Rotated,       //  92  Japanese Envelope You #4 Rotated
		kP16K,                  //  93  PRC 16K 146 x 215 mm
		kP32K,                  //  94  PRC 32K 97 x 151 mm
		kP32KBig,               //  95  PRC 32K(Big) 97 x 151 mm
		kPEnv1,                 //  96  PRC Envelope #1 102 x 165 mm
		kPEnv2,                 //  97  PRC Envelope #2 102 x 176 mm
		kPEnv3,                 //  98  PRC Envelope #3 125 x 176 mm
		kPEnv4,                 //  99  PRC Envelope #4 110 x 208 mm
		kPEnv5,                 // 100  PRC Envelope #5 110 x 220 mm
		kPEnv6,                 // 101  PRC Envelope #6 120 x 230 mm
		kPEnv7,                 // 102  PRC Envelope #7 160 x 230 mm
		kPEnv8,                 // 103  PRC Envelope #8 120 x 309 mm
		kPEnv9,                 // 104  PRC Envelope #9 229 x 324 mm
		kPEnv10,                // 105  PRC Envelope #10 324 x 458 mm
		kP16KRotated,           // 106  PRC 16K Rotated
		kP32KRotated,           // 107  PRC 32K Rotated
		kP32KBIGRotated,        // 108  PRC 32K(Big) Rotated
		kPEnv1Rotated,          // 109  PRC Envelope #1 Rotated 165 x 102 mm
		kPEnv2Rotated,          // 110  PRC Envelope #2 Rotated 176 x 102 mm
		kPEnv3Rotated,          // 111  PRC Envelope #3 Rotated 176 x 125 mm
		kPEnv4Rotated,          // 112  PRC Envelope #4 Rotated 208 x 110 mm
		kPEnv5Rotated,          // 113  PRC Envelope #5 Rotated 220 x 110 mm
		kPEnv6Rotated,          // 114  PRC Envelope #6 Rotated 230 x 120 mm
		kPEnv7Rotated,          // 115  PRC Envelope #7 Rotated 230 x 160 mm
		kPEnv8Rotated,          // 116  PRC Envelope #8 Rotated 309 x 120 mm
		kPEnv9Rotated,          // 117  PRC Envelope #9 Rotated 324 x 229 mm
		kPEnv10Rotated,         // 118  PRC Envelope #10 Rotated 458 x 324 mm
		kUserDefined = 256
	};

	enum PaperSource {
		kAuto,          // 7	o
		kManual,        // 4	o
		kUpper,         // 1	o
		kMiddle,        // 3	o
		kLower,         // 2	o
//		kOnlyOne,       // 1	x
//		kEnvelope,      // 5	o
//		kEnvManual,     // 6	x
//		kTractor,       // 8	x
//		kSmallFmt,      // 9	x
//		kLargeFmt,      // 10	x
//		kLargeCapacity, // 11	x
//		kCassette,      // 14	x
//		kFormSource,    // 15	x
		kCassette1 = 21,
		kCassette2,
		kCassette3,
		kCassette4,
		kCassette5,
		kCassette6,
		kCassette7,
		kCassette8,
		kCassette9,
		kUser = 256     // device specific bins start here
	};

	enum PrintStyle {
		kSimplex,
		kDuplex,
		kBooklet
	};
	
	enum BindingLocation {
		kLongEdgeLeft,
		kLongEdgeRight,
		kShortEdgeTop,
		kShortEdgeBottom,
		kLongEdge  = kLongEdgeLeft,
		kShortEdge = kShortEdgeTop
	};
	
	enum PageOrder {
		kAcrossFromLeft,
		kDownFromLeft,
		kAcrossFromRight,
		kDownFromRight,
		kLeftToRight = kAcrossFromLeft,
		kRightToLeft = kAcrossFromRight
	};

/*
	enum Quality {
		kDraft  = -1,
		kLow    = -2,
		kMedium = -3,
		kHigh   = -4
	};
*/
	enum Color {
		kMonochrome = 1,
		kColor,
		// Some PCL6 printers do not support compressed data
		// in color mode.
		kColorCompressionDisabled
	};

	enum SettingType {
		kPageSettings,
		kJobSettings
	};
	
	enum PageSelection {
		kAllPages,
		kOddNumberedPages,
		kEvenNumberedPages
	};

				JobData(BMessage* message, const PrinterCap* printerCap,
					SettingType type);
				~JobData();

				JobData(const JobData& jobData);
	JobData&	operator=(const JobData& jobData);

	void		Load(BMessage* message, const PrinterCap* printerCap,
					SettingType type);
	void		Save(BMessage* message = NULL);

	bool		GetShowPreview() const;
	void		SetShowPreview(bool showPreview);

	Paper		GetPaper() const;
	void		SetPaper(Paper paper);

	int32		GetResolutionID() const;
	void		SetResolutionID(int32 resolution);

	int32		GetXres() const;
	void		SetXres(int32 xres);

	int32		GetYres() const;
	void		SetYres(int32 yres);

	Orientation	GetOrientation() const;
	void		SetOrientation(Orientation orientation);

	float		GetScaling() const;
	void		SetScaling(float scaling);

	const BRect&	GetPaperRect() const;
	void 			SetPaperRect(const BRect& paperRect);

	const BRect&	GetScaledPaperRect() const;
	void  			SetScaledPaperRect(const BRect& paperRect);

	const BRect&	GetPrintableRect() const;
	void 			SetPrintableRect(const BRect& printableRect);

	const BRect&	GetScaledPrintableRect() const;
	void			SetScaledPrintableRect(const BRect& printableRect);

	const BRect&	GetPhysicalRect() const;
	void 			SetPhysicalRect(const BRect& PhysicalRect);

	const BRect&	GetScaledPhysicalRect() const;
	void			SetScaledPhysicalRect(const BRect& PhysicalRect);

	int32			GetNup() const;
	void			SetNup(int32 nup);

	bool			GetReverse() const;
	void			SetReverse(bool reverse);

	int32			GetFirstPage() const;
	void			SetFirstPage(int32 firstPage);

	int32			GetLastPage() const;
	void			SetLastPage(int32 lastPage);

	// libprint supports only B_RGB32
	color_space		GetSurfaceType() const;

	float			GetGamma() const;
	void			SetGamma(float gamma);

	float			GetInkDensity() const;
	void			SetInkDensity(float inkDensity);

	PaperSource		GetPaperSource() const;
	void			SetPaperSource(PaperSource paperSource);

	int32			GetCopies() const;
	void			SetCopies(int32 copies);

	bool			GetCollate() const;
	void			SetCollate(bool collate);

	PrintStyle		GetPrintStyle() const;
	void			SetPrintStyle(PrintStyle printStyle);

	BindingLocation	GetBindingLocation() const;
	void			SetBindingLocation(BindingLocation bindingLocation);

	PageOrder		GetPageOrder() const;
	void			SetPageOrder(PageOrder pageOrder);
	
	Color			GetColor() const;
	void			SetColor(Color color);
	
	Halftone::DitherType	GetDitherType() const;
	void					SetDitherType(Halftone::DitherType ditherType);
	
	PageSelection	GetPageSelection() const;
	void			SetPageSelection(PageSelection pageSelection);
	
	MarginUnit		GetMarginUnit() const;
	void			SetMarginUnit(MarginUnit marginUnit);

	DriverSpecificSettings& 		Settings();
	const DriverSpecificSettings&	Settings() const;

private:
	bool        fShowPreview;
	Paper       fPaper;
	int32       fResolutionID;
	int32       fXRes;
	int32       fYRes;
	Orientation fOrientation;
	float       fScaling;
	BRect       fPaperRect;
	BRect       fScaledPaperRect;
	BRect       fPrintableRect;
	BRect       fScaledPrintableRect;
	BRect       fPhysicalRect;
	BRect       fScaledPhysicalRect;
	int32       fNup;
	int32       fFirstPage;
	int32       fLastPage;
	float       fGamma;      // 1 identiy, < 1 brigther, > 1 darker
	float       fInkDensity; // [0, 255] lower means higher density
	PaperSource fPaperSource;
	int32       fCopies;
	bool        fCollate;
	bool        fReverse;
	PrintStyle  fPrintStyle;
	BindingLocation fBindingLocation;
	PageOrder   fPageOrder;
	SettingType fSettingType;
	BMessage    *fMsg;
	Color       fColor;
	Halftone::DitherType fDitherType;
	PageSelection        fPageSelection;
	MarginUnit  fMarginUnit;
	DriverSpecificSettings fDriverSpecificSettings;
};


inline bool
JobData::GetShowPreview() const
{
	return fShowPreview;
}

inline void
JobData::SetShowPreview(bool showPreview)
{
	fShowPreview = showPreview;
}


inline JobData::Paper
JobData::GetPaper() const
{
	return fPaper;
}


inline void
JobData::SetPaper(Paper paper)
{
	fPaper = paper;
}


inline int32
JobData::GetResolutionID() const
{
	return fResolutionID;
}


inline void
JobData::SetResolutionID(int32 resolution)
{
	fResolutionID = resolution;
}


inline int32
JobData::GetXres() const
{
	return fXRes;
}


inline void
JobData::SetXres(int32 xres)
{
	fXRes = xres;
}


inline int32
JobData::GetYres() const
{
	return fYRes;
}


inline void
JobData::SetYres(int32 yres)
{
	fYRes = yres;
};


inline JobData::Orientation
JobData::GetOrientation() const
{
	return fOrientation;
}


inline void
JobData::SetOrientation(Orientation orientation)
{
	fOrientation = orientation;
}


inline float
JobData::GetScaling() const
{
	return fScaling;
}


inline void
JobData::SetScaling(float scaling)
{
	fScaling = scaling;
}


inline const BRect&
JobData::GetPaperRect() const
{
	return fPaperRect;
}


inline void
JobData::SetPaperRect(const BRect &rect)
{
	fPaperRect = rect;
}


inline const BRect&
JobData::GetScaledPaperRect() const
{
	return fScaledPaperRect;
}


inline void
JobData::SetScaledPaperRect(const BRect &rect)
{
	fScaledPaperRect = rect;
}


inline const BRect &
JobData::GetPrintableRect() const
{
	return fPrintableRect;
}


inline void
JobData::SetPrintableRect(const BRect &rect)
{
	fPrintableRect = rect;
}


inline const BRect&
JobData::GetScaledPrintableRect() const
{
	return fScaledPrintableRect;
}


inline void
JobData::SetScaledPrintableRect(const BRect &rect)
{
	fScaledPrintableRect = rect;
}


inline const BRect&
JobData::GetPhysicalRect() const
{
	return fPhysicalRect;
}


inline void
JobData::SetPhysicalRect(const BRect &rect)
{
	fPhysicalRect = rect;
}


inline const BRect&
JobData::GetScaledPhysicalRect() const
{
	return fScaledPhysicalRect;
}



inline void
JobData::SetScaledPhysicalRect(const BRect &rect)
{
	fScaledPhysicalRect = rect;
}


inline int32
JobData::GetNup() const
{
	return fNup;
}


inline void
JobData::SetNup(int32 nup)
{
	fNup = nup;
}


inline bool
JobData::GetReverse() const
{
	return fReverse;
}


inline void
JobData::SetReverse(bool reverse)
{
	fReverse = reverse;
}


inline int32
JobData::GetFirstPage() const
{
	return fFirstPage;
}


inline void
JobData::SetFirstPage(int32 firstPage)
{
	fFirstPage = firstPage;
}


inline int32
JobData::GetLastPage() const
{
	return fLastPage;
}


inline void
JobData::SetLastPage(int32 lastPage)
{
	fLastPage = lastPage;
}


color_space
inline JobData::GetSurfaceType() const
{
	return B_RGB32;
}


inline float
JobData::GetGamma() const
{
	return fGamma;
}


inline void
JobData::SetGamma(float gamma)
{
	fGamma = gamma;
}


inline float
JobData::GetInkDensity() const
{
	return fInkDensity;
}


inline void
JobData::SetInkDensity(float inkDensity)
{
	fInkDensity = inkDensity;
}


inline JobData::PaperSource
JobData::GetPaperSource() const
{
	return fPaperSource;
}


inline void
JobData::SetPaperSource(PaperSource paperSource)
{
	fPaperSource = paperSource;
};


inline int32
JobData::GetCopies() const
{
	return fCopies;
}


inline void
JobData::SetCopies(int32 copies)
{
	fCopies = copies;
}


inline bool
JobData::GetCollate() const
{
	return fCollate;
}


inline void
JobData::SetCollate(bool collate)
{
	fCollate = collate;
}


inline JobData::PrintStyle
JobData::GetPrintStyle() const
{
	return fPrintStyle;
}


inline void
JobData::SetPrintStyle(PrintStyle print_style)
{
	fPrintStyle = print_style;
}


inline JobData::BindingLocation
JobData::GetBindingLocation() const
{
	return fBindingLocation;
}


inline void
JobData::SetBindingLocation(BindingLocation binding_location)
{
	fBindingLocation = binding_location;
}


inline JobData::PageOrder
JobData::GetPageOrder() const { return fPageOrder; }


inline void
JobData::SetPageOrder(PageOrder page_order)
{
	fPageOrder = page_order;
}


inline JobData::Color
JobData::GetColor() const
{
	return fColor;
}


inline void
JobData::SetColor(Color color)
{
	fColor = color;
}


inline Halftone::DitherType
JobData::GetDitherType() const
{
	return fDitherType;
}


inline void
JobData::SetDitherType(Halftone::DitherType dither_type)
{
	fDitherType = dither_type;
}


inline JobData::PageSelection
JobData::GetPageSelection() const
{
	return fPageSelection;
}


inline void
JobData::SetPageSelection(PageSelection pageSelection)
{
	fPageSelection = pageSelection;
}


inline MarginUnit
JobData::GetMarginUnit() const
{
	return fMarginUnit;
}


inline void
JobData::SetMarginUnit(MarginUnit marginUnit)
{
	fMarginUnit = marginUnit;
}

#endif	/* __JOBDATA_H */
