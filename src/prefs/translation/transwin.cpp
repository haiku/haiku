/*
	
	transwin.cpp
	
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef TRANS_WIN_H
#include "transwin.h"
#endif

#include <stdlib.h>



DATWindow::DATWindow(BRect frame)
				: BWindow(frame, "Data Translations2", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	newIcon = false;

	BRect   r( 300, 250, 380, 270 );   		// Pos: Info-Button
	BRect all( 0, 0, 400, 300);       		// Fenster-Groesse
	BRect  cv( 150, 13, 387, 245);     		// ConfigView & hosting Box
	BRect mau( 146, 8, 390, 290);			// Grenz-Box around ConfView
	BRect DTN_pos( 195 , 245, 298 , 265);	// Pos: Dateiname
	BRect ic ( 156 , 245, 188 , 277);		// Pos: TrackerIcon

	Konf = new BView(cv, "KONF", B_NOT_RESIZABLE, B_WILL_DRAW );
	// Konf->SetViewColor(217,217,217);

	rBox = new BBox(cv, "Right_Side");
	gBox = new BBox(mau, "Grau");
	
	Icon = new BView(ic, "Ikon", B_NOT_RESIZABLE, B_WILL_DRAW | B_PULSE_NEEDED);
	
	Icon->SetDrawingMode(B_OP_OVER);
	Icon->SetViewColor(217,217,217);
	Icon->SetHighColor(217,217,217);
	Icon_bit = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8, true, false);
	
	dButton = new BButton(r, "STD", "Info. . .", new BMessage(BUTTON_MSG));

	fBox = new BBox(all, "All_Window", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);

    r.Set( 10, 10, 120, 288);  // List View
        
	liste = new LView(r, "Transen", B_SINGLE_SELECTION_LIST); 
	liste->SetSelectionMessage(new BMessage(SEL_CHANGE));
    
    DTN = new BStringView(DTN_pos, "DataName", "Test", B_NOT_RESIZABLE);
    DTN->SetViewColor(217,217,217);
    
    // Here we add the names of all installed translators    
    WriteTrans(); 
    
    // Setting up the Box around my LView
    r.Set( 8, 8, 123, 288); // Box around the List
    lBox = new BBox(r, "List-Box");
    lBox->SetBorder(B_NO_BORDER);
	lBox->SetViewColor(255,255,255);
	
	// Put list into a BScrollView, to get that nice srcollbar  
    tListe = new BScrollView("scroll_trans", liste, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true, B_FANCY_BORDER);
    
    
    rBox->SetBorder(B_NO_BORDER);
	rBox->AddChild(Konf);
	rBox->SetFlags(B_NAVIGABLE_JUMP);
	  
	// Attach Views to my Window
	AddChild(dButton);
	AddChild(tListe);
	AddChild(rBox);

	AddChild(fBox);
	AddChild(Icon);
	AddChild(DTN);
	AddChild(lBox);
	AddChild(gBox);	
	
	// Set the focus
	liste->MakeFocus();
	
	// Select the first Translator in list
    liste->Select(0, false);
    showInfo = true;
    
}


bool DATWindow::QuitRequested()
{
    be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

// Reads the installed translators and adds them to our BListView
int DATWindow::WriteTrans()
{
	BTranslatorRoster *roster = BTranslatorRoster::Default(); 
	
	int32 num_translators, i; 
	translator_id *translators; 
	const char *tname, *tinfo;
	
    int32 tversion;
	
	// Get all Translators on the system. Gives us the number of translators
	// installed in num_translators and a reference to the first one, so
	// we can use translators like any normal array
		
	roster->GetAllTranslators(&translators, &num_translators);

	for (i=0;i<num_translators;i++) {
	    // Getting the first three Infos: Name, Info & Version
		roster->GetTranslatorInfo(translators[i], &tname,&tinfo, &tversion);
		liste->AddItem(new BStringItem(tname));
	} 

	delete [] translators; // clean up our droppings
	
	return 0;
}


// Finds a specific translator by it´s id
void DATWindow::Trans_by_ID(int32 id)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default(); 
	
	int32 num_translators;
	translator_id *translators; 
	// const char *translator_name, *translator_info;
	
  
	roster->GetAllTranslators(&translators, &num_translators);

	// Getting the first three Infos: Name, Info & Version
	roster->GetTranslatorInfo(translators[id], &translator_name,&translator_info, &translator_version);
	roster->Archive(&archiv, true);
	
	rBox->RemoveChild(Konf);
	if (roster->MakeConfigurationView( translators[id], new BMessage() , &Konf, new BRect( 0, 0, 200, 233)) != B_OK )
		{
		BString text;
		text << "The translator '" << translator_name << "' has no settings";
		Konf = new BStringView(BRect(0, 0, 200, 233), "no_settings", text.String()); 
		};
		
	// ( 171, 11, 391, 284)
	Konf->SetViewColor(217,217,217);
	Konf->ResizeTo(230, 233);
	Konf->SetFlags(B_WILL_DRAW | B_NAVIGABLE);
	rBox->AddChild(Konf);
	
	UpdateIfNeeded();	// Fixes that one Icon problem 
	
	delete [] translators; // clean up our droppings
}

void DATWindow::DrawIcon()
{
	if (newIcon) 
	{	
		entry.SetTo(pfad);
		node.SetTo(&entry);
		info.SetTo(&node);
		Icon->FillRect(BRect(0, 0, 31, 31));
		newIcon = false;
	}
	if (info.GetTrackerIcon(Icon_bit) != B_OK) QuitRequested();
	Icon->DrawBitmap(Icon_bit);
}

void DATWindow::WindowActivated(bool state)
{
	if (showInfo) DrawIcon();	
}

void DATWindow::FrameMoved(BPoint origin)
{
	if (showInfo) DrawIcon();
}

void DATWindow::MessageReceived(BMessage* message)
{
	int32 selected;
	
	switch(message->what)
	{
		case BUTTON_MSG:
			selected=liste->CurrentSelection(0);
			
			// Should we show translator info? 
			if (!showInfo)
			{
				// No => Show standard box
				(new BAlert("yo!", "Translation Settings\n\nUse this control panel to set values that various\ntranslators use when no other settings are specified\nin the application.", "OK"))->Go();
				break;
			}
			// Yes => Show Translator info.
			Trans_by_ID(selected);
			archiv.FindString("be:translator_path", selected, &pfad);
			tex.SetTo("");
			tex << "Name: " << translator_name << "\nVersion: " << (int32)translator_version << "\nInfo: " << translator_info << "\nPath: " << pfad << "\n"; 
			(new BAlert("yo!", tex.String(), "OK"))->Go();
			tex.SetTo("");
			DrawIcon();
			break;

		case SEL_CHANGE:
			// Now we have to update Filename and Icon
			// First we find the selected Translator
			selected=liste->CurrentSelection(0);
			
			// If none selected, clear the old one
			if (selected < 0) 
			{
				Icon->FillRect(BRect(0, 0, 31, 31));
				DTN->SetText("");
				showInfo = false;
				rBox->RemoveChild(Konf);
				break;
			}
			
			newIcon = true;			
			showInfo = true;
			Trans_by_ID(selected);
			// Now we find the path, and update filename
			archiv.FindString("be:translator_path", selected, &pfad);
			tex.SetTo(pfad);
			tex.Remove(0,tex.FindLast("/")+1);	
			DTN->SetText(tex.String());
			DrawIcon();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}
