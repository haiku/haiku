/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#ifndef _ShowImageWindow_h
#define _ShowImageWindow_h

#include <Window.h>
#include <FilePanel.h>
#include <TranslationDefs.h>

class ShowImageView;

class ShowImageWindow : public BWindow
{
public:
	static status_t NewWindow(const entry_ref* ref);
	static int32 CountWindows();

	ShowImageWindow(const entry_ref* ref, BBitmap* pBitmap);
	virtual ~ShowImageWindow();
	
	virtual void WindowActivated(bool active);
	virtual void FrameResized( float new_width, float new_height );
	virtual void MessageReceived(BMessage* message);
	
	status_t InitCheck();
	ShowImageView* GetShowImageView() const { return m_PrivateView; }
	
	void SetRef(const entry_ref* ref);
	void UpdateTitle();
	void LoadMenus(BMenuBar* pBar);
	void WindowRedimension( BBitmap *pBitmap );

	BMenuBar* pBar;	
private:
	BMenuItem * AddItemMenu( BMenu *pMenu, char *Caption, long unsigned int msg, 
			char shortcut, uint32 modifier, char target, bool enabled );
			
	void SaveAs(BMessage *pmsg);
		// Handle Save As submenu choice
	void SaveToFile(BMessage *pmsg);
		// Handle save file panel message

	BFilePanel *fpsavePanel;
	translator_id foutTranslator;
	uint32 foutType;
			
	entry_ref* m_pReferences;
	ShowImageView* m_PrivateView;
	
	static BLocker s_winListLocker;
	static BList s_winList;
};

#endif /* _ShowImageWindow_h */
