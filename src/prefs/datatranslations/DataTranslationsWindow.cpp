/*
 * DataTranslationsWindow.cpp
 *
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <TranslationDefs.h>
#include "DataTranslationsMessages.h"
#include "DataTranslationsWindow.h"
#include "DataTranslations.h"

#define DTW_RIGHT	400
#define DTW_BOTTOM	300

DataTranslationsWindow::DataTranslationsWindow()
	: BWindow(BRect(0, 0, DTW_RIGHT, DTW_BOTTOM),
		"DataTranslations", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
	MoveTo(dynamic_cast<DataTranslationsApplication *>(be_app)->WindowCorner());

	// Make sure that the window isn't positioned off screen
	BScreen screen;
	BRect scrf = screen.Frame(), winf = Frame();
	float x = winf.left, y = winf.top;
	scrf.top += 24;
	scrf.left += 5;
	scrf.right -= 5;
	if (winf.left < scrf.left)
		x = scrf.left;
	if (winf.right > scrf.right)
		x = scrf.right - winf.Width() - 5;
	if (winf.top < scrf.top)
		y = scrf.top;
	if (winf.bottom > scrf.bottom)
		y = scrf.bottom - winf.Height() - 15;
	
	if (x != winf.left || y != winf.top)
		MoveTo(x, y);
	
	BuildView();
	Show();
}

DataTranslationsWindow::~DataTranslationsWindow()
{
}

// Reads the installed translators and adds them to our BListView
int
DataTranslationsWindow::WriteTrans()
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();

	// Get all Translators on the system. Gives us the number of translators
	// installed in num_translators and a reference to the first one
	int32 num_translators;
	translator_id *translators;
	roster->GetAllTranslators(&translators, &num_translators);

	for (int32 i = 0; i < num_translators; i++) {
	    // Getting the first three Infos: Name, Info & Version
	    int32 tversion;
	    const char *tname, *tinfo;
		roster->GetTranslatorInfo(translators[i], &tname, &tinfo, &tversion);
		fTranListView->AddItem(new BStringItem(tname));
	} 
	
	delete[] translators;
	translators = NULL;
	
	return 0;
}

status_t
DataTranslationsWindow::GetTranInfo(int32 id, const char *&tranName,
	const char *&tranInfo, int32 &tranVersion, BPath &tranPath)
{
	// Returns information about the translator with the given id

	if (id < 0)
		return B_BAD_VALUE;

	int32 num_translators = 0;
	translator_id tid = 0, *translators = NULL;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &num_translators);
	tid = ((id < num_translators) ? translators[id] : 0);
	delete[] translators;
	translators = NULL;
	
	if (id >= num_translators)
		return B_BAD_VALUE;

	// Getting the first three Infos: Name, Info & Version
	roster->GetTranslatorInfo(tid, &tranName, &tranInfo, &tranVersion);
	
	// Get the translator's path
	entry_ref tranRef;
	roster->GetRefFor(tid, &tranRef);
	BEntry tmpEntry(&tranRef);
	tranPath.SetTo(&tmpEntry);
	
	return B_OK;
}

status_t
DataTranslationsWindow::ShowConfigView(int32 id)
{
	// Shows the config panel for the translator with the given id
	
	if (id < 0)
		return B_BAD_VALUE;

	int32 num_translators = 0;
	translator_id tid = 0, *translators = NULL;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	roster->GetAllTranslators(&translators, &num_translators);
	tid = ((id < num_translators) ? translators[id] : 0);
	delete[] translators;
	translators = NULL;

	if (id >= num_translators)
		return B_BAD_VALUE;
	
	fConfigBox->RemoveChild(fConfigView);
	BMessage emptyMsg;
	BRect rect(0, 0, 200, 233);
	status_t ret = roster->MakeConfigurationView(tid, &emptyMsg, &fConfigView, &rect);
	if (ret != B_OK) {
		fConfigBox->RemoveChild(fConfigView);
		return ret;
	}

	BRect konfRect(fConfigBox->Bounds());
	konfRect.InsetBy(3, 3);
	konfRect.bottom -= 45;
	float width = 0, height = 0;
	fConfigView->GetPreferredSize(&width, &height);
	float widen = max_c(0, width - konfRect.Width());
	float heighten = max_c(0, height - konfRect.Height());
	if (widen > 0 || heighten > 0) {
		ResizeBy(widen, heighten);
		konfRect.right += widen;
		konfRect.bottom += heighten;
	}
	fConfigView->MoveTo(konfRect.left, konfRect.top);
	fConfigView->ResizeTo(konfRect.Width(), konfRect.Height());
	fConfigBox->AddChild(fConfigView);
	
	UpdateIfNeeded();
	
	return B_OK;
}

void
DataTranslationsWindow::BuildView()
{
	BRect all(0, 0, 400, 300);       		// Fenster-Groesse
	BBox *mainBox = new BBox(all, "All_Window", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS,
						B_PLAIN_BORDER);
	AddChild(mainBox);
	
	BRect  configView( 150, 10, 390, 290);
	fConfigBox = new BBox(configView, "Right_Side", B_FOLLOW_ALL_SIDES);
	mainBox->AddChild(fConfigBox);

	BRect innerRect(fConfigBox->Bounds());
	innerRect.InsetBy(8,8);

	BRect iconRect(0,0,31,31);
	iconRect.OffsetTo(innerRect.left,innerRect.bottom-iconRect.Height());
	fIconView = new IconView(iconRect, "Ikon", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
                     B_WILL_DRAW | B_FRAME_EVENTS);
	fConfigBox->AddChild(fIconView);
	
	BRect infoRect(0,0,80,20);
	infoRect.OffsetTo(innerRect.right-infoRect.Width(),innerRect.bottom-10-infoRect.Height());
	BButton *button = new BButton(infoRect, "STD", "Info...", new BMessage(BUTTON_MSG),
	                      B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,
                          B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	fConfigBox->AddChild(button);
    
	BRect dataNameRect( iconRect.right+5 , iconRect.top,
                        infoRect.left-5 , iconRect.bottom);
    fTranNameView = new BStringView(dataNameRect, "DataName", "Test", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
    // fTranNameView->SetViewColor(ui_color(B_BACKGROUND_COLOR));
	fConfigBox->AddChild(fTranNameView);
    
	BRect konfRect(innerRect);
    konfRect.bottom = iconRect.top;
	fConfigView = new BView(konfRect, "KONF", B_FOLLOW_ALL_SIDES,
                     B_WILL_DRAW | B_FRAME_EVENTS );
	// fConfigView->SetViewColor(ui_color(B_BACKGROUND_COLOR));
	fConfigBox->AddChild(fConfigView);

    BRect listRect(10, 10, 120, 288);  // List View
	fTranListView = new DataTranslationsView(listRect, "Transen", B_SINGLE_SELECTION_LIST); 
	fTranListView->SetSelectionMessage(new BMessage(SEL_CHANGE));

	// Put list into a BScrollView, to get that nice srcollbar  
    BScrollView *scrollView = new BScrollView("scroll_trans", fTranListView,
    	B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS,
    	false, true, B_FANCY_BORDER);
	mainBox->AddChild(scrollView);

    // Here we add the names of all installed translators    
    WriteTrans();
    
	// Set the focus
	fTranListView->MakeFocus();
	
	// Select the first Translator in list
    fTranListView->Select(0, false);    
}

bool
DataTranslationsWindow::QuitRequested()
{
	BPoint pt(Frame().left, Frame().top);
	dynamic_cast<DataTranslationsApplication *>(be_app)->SetWindowCorner(pt);
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}

void
DataTranslationsWindow::MessageReceived(BMessage *message)
{
	BPath tranPath;
	BString strInfoMsg;
	const char *tranName = NULL, *tranInfo = NULL;
	int32 selected = -1, tranVersion = 0;
		
	switch (message->what) {
	
		case BUTTON_MSG:
			selected = fTranListView->CurrentSelection(0);
			if (selected < 0) {
				// If no translator is selected, show a message explaining
				// what the config panel is for
				(new BAlert("yo!",
					"Translation Settings\n\n"
					"Use this control panel to set values that various\n"
					"translators use when no other settings are specified\n"
					"in the application.",
					"OK"))->Go();
				break;
			}
			
			GetTranInfo(selected, tranName, tranInfo, tranVersion, tranPath);
			strInfoMsg << "Name: " << tranName << "\nVersion: ";
			if (tranVersion < 0x100)
				// If the version number doesn't follow the standard format,
				// just print it as is
				strInfoMsg << tranVersion;
			else {
				// Convert the version number into a readable format
				strInfoMsg <<
					static_cast<int>(B_TRANSLATION_MAJOR_VER(tranVersion)) << '.' <<
					static_cast<int>(B_TRANSLATION_MINOR_VER(tranVersion)) << '.' <<
					static_cast<int>(B_TRANSLATION_REVSN_VER(tranVersion));
			}				
			strInfoMsg << "\nInfo: " << tranInfo <<
				"\nPath: " << tranPath.Path() << "\n"; 
				
			(new BAlert("yo!", strInfoMsg.String(), "OK"))->Go();
			break;

		case SEL_CHANGE:
			// Update the icon and translator info panel
			// to match the new selection
			selected = fTranListView->CurrentSelection(0);
			if (selected < 0) {
				// If none selected, clear the old one
				fIconView->DrawIcon(false);
				fTranNameView->SetText("");
				fConfigBox->RemoveChild(fConfigView);
				break;
			}
			
			ShowConfigView(selected);
			GetTranInfo(selected, tranName, tranInfo, tranVersion, tranPath);
			fTranNameView->SetText(tranPath.Leaf());
			fIconView->SetIcon(tranPath);
			break;
		
		default:
			BWindow::MessageReceived(message);
			break;
	}	
}

