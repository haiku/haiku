/* C-mail API - compatibility function (stubs) for the old mail kit
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <E-mail.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <List.h>

#include <string.h>

#include <MailDaemon.h>
#include <MailSettings.h>
#include <MailMessage.h>
#include <crypt.h>


_EXPORT status_t check_for_mail(int32 * incoming_count)
{
	status_t err = BMailDaemon::CheckMail(true);
	if (err < B_OK)
		return err;
		
	if (incoming_count != NULL)
		*incoming_count = BMailDaemon::CountNewMessages(true);
		
	return B_OK;
}
	
_EXPORT status_t send_queued_mail(void)
{
	return BMailDaemon::SendQueuedMail();
}

_EXPORT int32 count_pop_accounts(void)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	if (status < B_OK)
		return 0;

	path.Append("Mail/chains/inbound");
	BDirectory dir(path.Path());
	return dir.CountEntries();
}

_EXPORT status_t get_mail_notification(mail_notification *notification)
{
	notification->alert = true;
	notification->beep = false;
	return B_OK;
}

_EXPORT status_t set_mail_notification(mail_notification *, bool)
{
	return B_NO_REPLY;
}

_EXPORT status_t get_pop_account(mail_pop_account* account, int32 index)
{
	status_t err = B_OK;
	const char *password, *passwd;
	BMessage settings;
	
	BList chains;
	GetInboundMailChains(&chains);
	
	BMailChain *chain = (BMailChain *)(chains.ItemAt(index));
	if (chain == NULL) {
		err = B_BAD_INDEX;
		goto clean_up; //------Eek! A goto!
	}
	
	chain->GetFilter(0,&settings);
	strcpy(account->pop_name,settings.FindString("username"));
	strcpy(account->pop_host,settings.FindString("server"));
	strcpy(account->real_name,chain->MetaData()->FindString("real_name"));
	strcpy(account->reply_to,chain->MetaData()->FindString("reply_to"));
	
	password = settings.FindString("password");
	passwd = get_passwd(&settings,"cpasswd");
	if (passwd)
		password = passwd;
	strcpy(account->pop_password,password);
	
	//-------Note that we don't do the scheduling flags
	
  clean_up:
	for (int32 i = 0; i < chains.CountItems(); i++)
		delete (BMailChain *)chains.ItemAt(i);
		
	return err;
}

_EXPORT status_t set_pop_account(mail_pop_account *, int32, bool)
{
	return B_NO_REPLY;
}

_EXPORT status_t get_smtp_host(char* buffer)
{
	BMailChain chain(BMailSettings().DefaultOutboundChainID());
	status_t err = chain.InitCheck();
	if (err < B_OK)
		return err;
		
	BMessage settings;
	err = chain.GetFilter(chain.CountFilters() - 1,&settings);
	if (err < B_OK)
		return err;
	
	if (settings.HasString("server"))
		strcpy(buffer,settings.FindString("server"));
	else
		return B_NAME_NOT_FOUND;
		
	return B_OK;
}

_EXPORT status_t set_smtp_host(char * /* host */, bool /* save */)
{
	return B_NO_REPLY;
}

_EXPORT status_t forward_mail(entry_ref *ref, const char *recipients, bool now)
{
	BFile file(ref, O_RDONLY);
	status_t status = file.InitCheck();
	if (status < B_OK)
		return status;

	BEmailMessage mail(&file);
	mail.SetTo(recipients);
	
	return mail.Send(now);
}

