/*
 * DataTranslationsWindow.cpp
 *
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>


#include "DataTranslationsMessages.h"
#include "DataTranslationsWindow.h"
#include "DataTranslations.h"

#define DATA_TRANSLATIONS_WINDOW_RIGHT	400
#define DATA_TRANSLATIONS_WINDOW_BOTTOM	300

DataTranslationsWindow::DataTranslationsWindow()
				: BWindow(BRect(0,0,DATA_TRANSLATIONS_WINDOW_RIGHT,DATA_TRANSLATIONS_WINDOW_BOTTOM), "DataTranslations", B_TITLED_WINDOW, B_NOT_ZOOMABLE)
{
	BScreen screen;

	MoveTo(dynamic_cast<DataTranslationsApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	// above does not work correctly....have to work on it.
	
	BuildView();
	Show();

}


// Reads the installed translators and adds them to our BListView
int DataTranslationsWindow::WriteTrans()
{
	BTranslatorRoster *roster = BTranslatorRoster::Default(); 
	
	int32 num_translators, i; 
	translator_id *translators; 
	const char *tname, *tinfo;
	
    int32 tversion;
	
	// Get all Translators on the system. Gives us the number of translators
	// installed in num_translators and a reference to the first one

	roster->GetAllTranslators(&translators, &num_translators);

	for (i=0;i<num_translators;i++) {
	    // Getting the first three Infos: Name, Info & Version
		roster->GetTranslatorInfo(translators[i], &tname,&tinfo, &tversion);
		fTranListView->AddItem(new BStringItem(tname));
	} 
	
	delete [] translators; // Garbage collection
	
	return 0;
}

// Finds a specific translator by it´s id
void DataTranslationsWindow::Trans_by_ID(int32 id)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default(); 
	
	int32 num_translators;
	translator_id *translators; 
	bool has_view;
	
	has_view = true;  
	roster->GetAllTranslators(&translators, &num_translators);

	// Getting the first three Infos: Name, Info & Version
	roster->GetTranslatorInfo(translators[id], &translator_name,&translator_info, &translator_version);
	roster->Archive(&fRosterArchive, true);
	
	rBox->RemoveChild(Konf);
	if (roster->MakeConfigurationView( translators[id], new BMessage() , &Konf, new BRect( 0, 0, 200, 233)) != B_OK )
		{// be_app->PostMessage(B_QUIT_REQUESTED); // Something went wrong -> Quit
			rBox->RemoveChild(Konf);
			has_view = false;
		}

	// Konf->SetViewColor(ui_color(B_BACKGROUND_COLOR));
	if (has_view) 
	{
		BRect konfRect(rBox->Bounds());
		konfRect.InsetBy(3,3);
		konfRect.bottom -= 45;
		float width = 0, height = 0;
		Konf->GetPreferredSize(&width,&height);
		float widen = max_c(0,width-konfRect.Width());
		float heighten = max_c(0,height-konfRect.Height());
		if ((widen > 0) || (heighten > 0)) {
			ResizeBy(widen,heighten);
			konfRect.right += widen;
			konfRect.bottom += heighten;
		}
		Konf->MoveTo(konfRect.left,konfRect.top);
		Konf->ResizeTo(konfRect.Width(), konfRect.Height());
		rBox->AddChild(Konf);
	}
	
	UpdateIfNeeded();	
	
	delete [] translators; // Garbage collection
}

void DataTranslationsWindow::BuildView()
{
	BRect all(0, 0, 400, 300);       		// Fenster-Groesse
	fBox = new BBox(all, "All_Window", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS,
						B_PLAIN_BORDER);
	AddChild(fBox);
	
	BRect  configView( 150, 10, 390, 290);
	rBox = new BBox(configView, "Right_Side", B_FOLLOW_ALL_SIDES);
	fBox->AddChild(rBox);

	BRect innerRect(rBox->Bounds());
	innerRect.InsetBy(8,8);

	BRect iconRect(0,0,31,31);
	iconRect.OffsetTo(innerRect.left,innerRect.bottom-iconRect.Height());
	fIconView = new IconView(iconRect, "Ikon", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
                     B_WILL_DRAW | B_FRAME_EVENTS);
	//fIconView->SetDrawingMode(B_OP_OVER);
		// to preserve transparent areas of the icon
	//fIconView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	//fIconView->SetViewColor(0, 255, 0);
	//fIconView->SetHighColor(0, 0, 255);
	rBox->AddChild(fIconView);
	
	BRect infoRect(0,0,80,20);
	infoRect.OffsetTo(innerRect.right-infoRect.Width(),innerRect.bottom-10-infoRect.Height());
	dButton = new BButton(infoRect, "STD", "Info...", new BMessage(BUTTON_MSG),
	                      B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT,
                          B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	rBox->AddChild(dButton);
    
	BRect dataNameRect( iconRect.right+5 , iconRect.top,
                        infoRect.left-5 , iconRect.bottom);
    DTN = new BStringView(dataNameRect, "DataName", "Test", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
    // DTN->SetViewColor(ui_color(B_BACKGROUND_COLOR));
	rBox->AddChild(DTN);
    
	BRect konfRect(innerRect);
    konfRect.bottom = iconRect.top;
	Konf = new BView(konfRect, "KONF", B_FOLLOW_ALL_SIDES,
                     B_WILL_DRAW | B_FRAME_EVENTS );
	// Konf->SetViewColor(ui_color(B_BACKGROUND_COLOR));
	rBox->AddChild(Konf);

    BRect listRect(10, 10, 120, 288);  // List View
	fTranListView = new DataTranslationsView(listRect, "Transen", B_SINGLE_SELECTION_LIST); 
	fTranListView->SetSelectionMessage(new BMessage(SEL_CHANGE));

	// Put list into a BScrollView, to get that nice srcollbar  
    BScrollView *scrollView = new BScrollView("scroll_trans", fTranListView,
    	B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW | B_FRAME_EVENTS,
    	false, true, B_FANCY_BORDER);
	fBox->AddChild(scrollView);

    // Here we add the names of all installed translators    
    WriteTrans(); 
    
	// Set the focus
	fTranListView->MakeFocus();
	
	// Select the first Translator in list
    fTranListView->Select(0, false);
    fTranSelected = true;
    
}

bool DataTranslationsWindow::QuitRequested()
{

	dynamic_cast<DataTranslationsApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));

	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
}

void
DataTranslationsWindow::MessageReceived(BMessage *message)
{			
	int32 selected;
	
	switch(message->what) {
		case BUTTON_MSG:
			selected=fTranListView->CurrentSelection(0);
			
			// Should we show translator info? 
			if (!fTranSelected)
			{
				// No => Show standard box
				(new BAlert("yo!", "Translation Settings\n\nUse this control panel to set values that various\ntranslators use when no other settings are specified\nin the application.", "OK"))->Go();
				break;
			}
			// Yes => Show Translator info.
			Trans_by_ID(selected);
			fRosterArchive.FindString("be:translator_path", selected, &pfad);
			tex << "Name :" << translator_name << "\nVersion: " << (int32)translator_version << "\nInfo: " << translator_info << "\nPath: " << pfad << "\n"; 
			(new BAlert("yo!", tex.String(), "OK"))->Go();
			tex.SetTo("");
			break;

		case SEL_CHANGE:
			// Now we have to update Filename and Icon
			// First we find the selected Translator
			selected=fTranListView->CurrentSelection(0);
			
			// If none selected, clear the old one
			if (selected < 0) 
			{
				fIconView->DrawIcon(false);
				DTN->SetText("");
				fTranSelected = false;
				rBox->RemoveChild(Konf);
				break;
			}
			
			fTranSelected = true;
			Trans_by_ID(selected);
			// Now we find the path, and update filename
			fRosterArchive.FindString("be:translator_path", selected, &pfad);
			tex.SetTo(pfad);
			tex.Remove(0,tex.FindLast("/")+1);	
			DTN->SetText(tex.String());
			
			entry.SetTo(pfad);
			node.SetTo(&entry);
			info.SetTo(&node);
			fIconView->SetIconFromNodeInfo(info);
			break;

		
		default:
			BWindow::MessageReceived(message);
			break;
	}
	
}

DataTranslationsWindow::~DataTranslationsWindow()
{

}
