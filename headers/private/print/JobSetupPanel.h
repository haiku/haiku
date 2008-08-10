/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */
#ifndef _JOB_SETUP_PANEL_H_
#define _JOB_SETUP_PANEL_H_


#include <PrintPanel.h>


class BButton;
class BCheckBox;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BStringView;
class BTextControl;
class BTextView;


namespace BPrivate {
	namespace Print {


class BPrinter;
class BPrinterRoster;


enum print_range {
	B_ALL_PAGES		= 0,
	B_SELECTION		= 1,
	B_PAGE_RANGE	= 2
};


const uint32 B_NO_OPTIONS			= 0x00000000;
const uint32 B_PRINT_TO_FILE		= 0x00000001;
const uint32 B_PRINT_SELECTION		= 0x00000002;
const uint32 B_PRINT_PAGE_RANGE		= 0x00000004;
const uint32 B_PRINT_COLLATE_COPIES	= 0x00000008;



class BJobSetupPanel : public BPrintPanel {
public:
								BJobSetupPanel(BPrinter* printer);
								BJobSetupPanel(BPrinter* printer, uint32 flags);
	virtual						~BJobSetupPanel();

								BJobSetupPanel(BMessage* data);
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;
	virtual	void				MessageReceived(BMessage* message);

	virtual	status_t			Go();

			BPrinter*			Printer() const;
			void				SetPrinter(BPrinter* printer, bool keepSettings);

			print_range			PrintRange() const;
			void				SetPrintRange(print_range range);

			int32				FirstPage() const;
			int32				LastPage() const;
			void				SetPageRange(int32 firstPage, int32 lastPage);

			uint32				OptionFlags() const;
			void				SetOptionFlags(uint32 flags);

private:
			void				_InitObject();
			void				_SetupInterface();
			void				_DisallowChar(BTextView* textView);

private:
			BPrinter*			fPrinter;
			BPrinterRoster*		fPrinterRoster;

			print_range			fPrintRange;
			uint32				fJobPanelFlags;

			BPopUpMenu*			fPrinterPopUp;
			BMenuField*			fPrinterMenuField;
			BButton*			fProperties;
			BStringView*		fPrinterInfo;
			BCheckBox*			fPrintToFile;
			BRadioButton*		fPrintAll;
			BRadioButton*		fPagesFrom;
			BTextControl*		fFirstPage;
			BTextControl*		fLastPage;
			BRadioButton*		fSelection;
			BTextControl*		fNumberOfCopies;
			BCheckBox*			fCollate;
			BCheckBox*			fReverse;
			BCheckBox*			fColor;
			BCheckBox*			fDuplex;
};


	}	// namespace Print
}	// namespace BPrivate


#endif	// _JOB_SETUP_PANEL_H_
