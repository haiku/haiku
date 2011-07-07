/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBSETUPDLG_H
#define __JOBSETUPDLG_H

#include <View.h>
#include <map>

#include "DialogWindow.h"

#include "JobData.h"
#include "Halftone.h"
#include "JSDSlider.h"
#include "PrinterCap.h"

class BCheckBox;
class BGridLayout;
class BPopUpMenu;
class BRadioButton;
class BSlider;
class BTextControl;
class BTextView;
class HalftoneView;
class JobData;
class PagesView;
class PrinterCap;
class PrinterData;


template<typename T, typename R>
class Range
{
public:
	Range();
	Range(const char* label, const char* key, const R* range, BSlider* slider);
	const char* Key() const;
	T Value();
	void UpdateLabel();

private:
	const char* fLabel;
	const char* fKey;
	const R* fRange;
	BSlider* fSlider;
};


template<typename T, typename R>
Range<T, R>::Range()
	:
	fKey(NULL),
	fRange(NULL),
	fSlider(NULL)
{
}


template<typename T, typename R>
Range<T, R>::Range(const char* label, const char* key, const R* range,
	BSlider* slider)
	:
	fLabel(label),
	fKey(key),
	fRange(range),
	fSlider(slider)
{

}


template<typename T, typename R>
const char*
Range<T, R>::Key() const
{
	return fKey;
}


template<typename T, typename R>
T
Range<T, R>::Value()
{
	return static_cast<T>(fRange->Lower() +
		(fRange->Upper() - fRange->Lower()) * fSlider->Position());
}


template<typename T, typename R>
void
Range<T, R>::UpdateLabel()
{
	BString label = fLabel;
	label << " (" << Value() << ")";
	fSlider->SetLabel(label.String());
}


typedef Range<int32, IntRangeCap> IntRange;
typedef Range<double, DoubleRangeCap> DoubleRange;

class JobSetupView : public BView {
public:
					JobSetupView(JobData* jobData, PrinterData* printerData,
						const PrinterCap* printerCap);
	virtual	void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);
			bool	UpdateJobData();

private:
			void	UpdateButtonEnabledState();
			bool	IsHalftoneConfigurationNeeded();
			void	CreateHalftoneConfigurationUI();
			void	AddDriverSpecificSettings(BGridLayout* gridLayout, int row);
			void	AddPopUpMenu(const DriverSpecificCap* capability,
						BGridLayout* gridLayout, int& row);
			void	AddCheckBox(const DriverSpecificCap* capability,
						BGridLayout* gridLayout, int& row);
			void	AddIntSlider(const DriverSpecificCap* capability,
						BGridLayout* gridLayout, int& row);
			void	AddDoubleSlider(const DriverSpecificCap* capability,
						BGridLayout* gridLayout, int& row);
			string	GetDriverSpecificValue(PrinterCap::CapID category,
						const char* key);
			template<typename Predicate>
			void	FillCapabilityMenu(BPopUpMenu* menu, uint32 message,
						const BaseCap** capabilities, int count,
						Predicate& predicate);
			void	FillCapabilityMenu(BPopUpMenu* menu, uint32 message,
						PrinterCap::CapID category, int id);
			void	FillCapabilityMenu(BPopUpMenu* menu, uint32 message,
						const BaseCap** capabilities, int count, int id);
			int		GetID(const BaseCap** capabilities, int count,
						const char* label, int defaultValue);
			BRadioButton* CreatePageSelectionItem(const char* name,
						const char* label,
						JobData::PageSelection pageSelection);
			void	AllowOnlyDigits(BTextView* textView, int maxDigits);
			void	UpdateHalftonePreview();
			void	UpdateIntSlider(BMessage* message);
			void	UpdateDoubleSlider(BMessage* message);

			JobData::Color			Color();
			Halftone::DitherType	DitherType();
			float					Gamma();
			float					InkDensity();
			JobData::PaperSource	PaperSource();


	BTextControl*		fCopies;
	BTextControl*		fFromPage;
	BTextControl*		fToPage;
	JobData*			fJobData;
	PrinterData*		fPrinterData;
	const PrinterCap*	fPrinterCap;
	BPopUpMenu*			fColorType;
	BPopUpMenu*			fDitherType;
	BMenuField*			fDitherMenuField;
	JSDSlider*			fGamma;
	JSDSlider*			fInkDensity;
	HalftoneView*		fHalftone;
	BBox*				fHalftoneBox;
	BRadioButton*		fAll;
	BCheckBox*			fCollate;
	BCheckBox*			fReverse;
	PagesView*			fPages;
	BPopUpMenu*			fPaperFeed;
	BCheckBox*			fDuplex;
	BPopUpMenu*			fNup;
	BRadioButton*		fAllPages;
	BRadioButton*		fOddNumberedPages;
	BRadioButton*		fEvenNumberedPages;
	std::map<PrinterCap::CapID, BPopUpMenu*>	fDriverSpecificPopUpMenus;
	std::map<string, BCheckBox*>				fDriverSpecificCheckBoxes;
	std::map<PrinterCap::CapID, IntRange>		fDriverSpecificIntSliders;
	std::map<PrinterCap::CapID, DoubleRange>	fDriverSpecificDoubleSliders;
	BCheckBox*			fPreview;
};

class JobSetupDlg : public DialogWindow {
public:
					JobSetupDlg(JobData* jobData, PrinterData* printerData,
						const PrinterCap* printerCap);
	virtual	void	MessageReceived(BMessage* message);

private:
	JobSetupView*	fJobSetup;
};

#endif	/* __JOBSETUPDLG_H */
