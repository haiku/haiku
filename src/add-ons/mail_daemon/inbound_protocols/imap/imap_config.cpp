/* IMAPConfig - config view for the IMAP protocol add-on
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <TextControl.h>

#include <ProtocolConfigView.h>
#include <MailAddon.h>

#include <MDRLanguage.h>

class IMAPConfig : public BMailProtocolConfigView {
	public:
		IMAPConfig(BMessage *archive);
		virtual ~IMAPConfig();
		virtual	status_t Archive(BMessage *into, bool deep = true) const;
		virtual void GetPreferredSize(float *width, float *height);
};

IMAPConfig::IMAPConfig(BMessage *archive)
	: BMailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME | B_MAIL_PROTOCOL_HAS_PASSWORD | B_MAIL_PROTOCOL_HAS_HOSTNAME | B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
	#ifdef USESSL
	 | B_MAIL_PROTOCOL_HAS_FLAVORS)
{
		AddFlavor("Unencrypted");
		AddFlavor("SSL");
	#else
		) {
	#endif
	
	SetTo(archive);
	
	((BControl *)(FindView("leave_mail_remote")))->SetValue(B_CONTROL_ON);
	((BControl *)(FindView("leave_mail_remote")))->Hide();
	
	BRect frame = FindView("delete_remote_when_local")->Frame();
	
	((BControl *)(FindView("delete_remote_when_local")))->SetEnabled(true);
	((BControl *)(FindView("delete_remote_when_local")))->MoveBy(0,-25);
	
		
	frame.right -= 10;// FindView("pass")->Frame().right;
	/*frame.top += 10;
	frame.bottom += 10;*/
	
	BTextControl *folder = new BTextControl(frame,"root","Mailbox Root: ","",NULL);
	folder->SetDivider(be_plain_font->StringWidth("Mailbox Root: "));
		
	if (archive->HasString("root"))
		folder->SetText(archive->FindString("root"));
	
	AddChild(folder);
		
	ResizeToPreferred();
}

IMAPConfig::~IMAPConfig() {}

status_t IMAPConfig::Archive(BMessage *into, bool deep) const {
	BMailProtocolConfigView::Archive(into,deep);
	
	if (into->ReplaceString("root",((BTextControl *)(FindView("root")))->Text()) != B_OK)
		into->AddString("root",((BTextControl *)(FindView("root")))->Text());
	
	into->PrintToStream();
	
	return B_OK;
}

void IMAPConfig::GetPreferredSize(float *width, float *height) {
	BMailProtocolConfigView::GetPreferredSize(width,height);
	*height -= 20;
}

BView* instantiate_config_panel(BMessage *settings,BMessage *) {
	return new IMAPConfig(settings);
}
