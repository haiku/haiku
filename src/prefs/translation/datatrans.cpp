/** 
 * DataTranslations2
 *
 * This is to become the OBOS-replacement for the original BeOS 
 * DataTranslations Preferences App. This App is not doing a lot,
 * basically it just shows all installed Translators, and helps you
 * installing new ones. (Drag em on the ListBox; Drop; done)
 *
 */

#include <stdlib.h>

#include <Alert.h>


#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef DATATRANS_H
#include "datatrans.h"
#endif

#ifndef TRANS_WIN_H
#include "transwin.h"
#endif

int main(int, char**)
{	
	DATApp		myApplication;

	myApplication.Run();

	return(0);
}

DATApp::DATApp():BApplication("application/x-vnd.Be-prefs-translations2")
{
	DATWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(40, 40, 440, 340);
	aWindow = new DATWindow(aRect);
      
        	
	// make window visible
	aWindow->Show();

}

void DATApp :: RefsReceived(BMessage *message)
{

	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;
	BString newfile;
	
	char e_name[B_FILE_NAME_LENGTH];
	char mbuf[256];
	
	
	message->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 
	
    
    if ( message->FindRef("refs", &ref) == B_OK )
    {
    	BEntry entry(&ref, true);
    	if ( entry.IsFile() && entry.GetPath(&path)==B_OK)
    	{
    		entry.GetName(e_name);
    		BNode      node (&entry); 
	        BNodeInfo  info (&node); 
	        info.GetType (mbuf);
	        string.SetTo(mbuf);
		
			if (string.FindFirst("application/x-vnd.Be-elfexecutable") != B_ERROR )
			{
    			BDirectory dirt("/boot/home/config/add-ons/Translators/");
    			newfile.SetTo("/boot/home/config/add-ons/Translators/");
    			newfile << e_name; // Newfile with full path.
    			
    			// Find out wether we need to copy it
    			string.SetTo("");
    			string << "The item '" << e_name << "' is now copied into the right place.\nWhat do you want to do with the original?";
    			BAlert *keep = new BAlert("keep",  string.String() , "Remove", "Keep");
    			keep->SetShortcut(1, B_ESCAPE);
    			
    			if (keep->Go() == 0) // Just a quick move 
    			{
    				if (dirt.Contains(newfile.String()))
    				{
    					string.SetTo("");
    					string << "An item named '" << e_name <<"' already is in the \nTranslators folder";
    					BAlert *myAlert = new BAlert("title",  string.String() , "Overwrite", "Stop");
    					myAlert->SetShortcut(1, B_ESCAPE); 
					
						if (myAlert->Go() != 0) 
							return;
						// File exists, we are still here. Kill it.
						BEntry dele;
						dirt.FindEntry(e_name, &dele);
						dele.Remove();	
					}
					entry.MoveTo(&dirt);
    				(new BAlert("", "You have to quit and restart running applications\nfor the installed Translators to be available in them.", "OK"))->Go();
    				return;
    			}
    			
    			if (dirt.Contains(newfile.String()))
    			{
    				// File exists, What are we supposed to do now (Overwrite/_Stopp_?)
    				string.SetTo("");
    				string << "An item named '" << e_name <<"' already is in the \nTranslators folder";
    				BAlert *myAlert = new BAlert("title",  string.String() , "Overwrite", "Stop");
    				myAlert->SetShortcut(1, B_ESCAPE); 
					
					if (myAlert->Go() != 0) 
						return;
				}
    			
    			string.SetTo(path.Path()); // Fullpath+Filename 
    			
    			BString 
				command	=	"copyattr -d -r ";
				command << string.String() << " " << newfile.String(); // Prepare Copy 
				
				system(command.String()); // Execute copy command
    			
    			string.SetTo("");
    			string << "Filename: " << e_name << "\nPath: " << path.Path() ;
    			
    			// The new Translator has been installed by now.
    			(new BAlert("", "You have to quit and restart running applications\nfor the installed Translators to be available in them.", "OK"))->Go();
    			// And done we are
    			return;
    		}
    		else
    		{
    			// Not a Translator
    			entry.GetName(e_name);
    			string.SetTo("");
	    		string << "The item '" << e_name << "' does not appear to be a Translator and will not be installed";
    			(new BAlert("", string.String(), "Stop"))->Go();
    		}
    	}
    	else
    	{
    		// Not even a file...
    		entry.GetName(e_name);
    		string.SetTo("");
    		string << "The item '" << e_name << "' does not appear to be a Translator and will not be installed";
    		(new BAlert("", string.String(), "Stop"))->Go();	
    	}	
    }
}
