#ifndef BACKUP_WIN_H
#define BACKUP_WIN_H

#include <Window.h>
#include <View.h>
#include <TextControl.h>
#include <Button.h>

class NetworkWindow;

// The window that appears when you press the 'Backup' button
class BackupWin : public BWindow  
{                                
public:
	BackupWin(const BRect &frame);	
	
	BTextControl 	*fName;
	BButton 		*fCancel;
	BButton			*fDone;	
	
	NetworkWindow *fParentWindow;
	
	void SaveBackup(const char *name=NULL);
	virtual void MessageReceived(BMessage *message);	
	
	static const int32 kSaveBackup_M		= 'fbam' ;		
	static const int32 kCancel_bak_M		= 'cbam' ;		
};

#endif

