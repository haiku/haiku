#include "AutoConfig.h"
#include "DNSQuery.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <stdio.h>


status_t
AutoConfig::GetInfoFromMailAddress(const char* email, provider_info *info)
{
	BString provider = ExtractProvider(email);

	// first check the database
	if (LoadProviderInfo(provider, info) == B_OK)
		return B_OK;

	// fallback try to read MX record
	if (GetMXRecord(provider.String(), info) == B_OK) 
		return B_ENTRY_NOT_FOUND;

	// if no MX record received guess a name
	GuessServerName(provider.String(), info);
	return B_ENTRY_NOT_FOUND;
}


status_t
AutoConfig::GetMXRecord(const char* provider, provider_info *info)
{
	BObjectList<mx_record> mxList(5, true);
	DNSQuery dnsQuery;
	if (dnsQuery.GetMXRecords(provider, &mxList) != B_OK)
		return B_ERROR;

	mx_record *mxRec = mxList.ItemAt(0);
	if (mxRec == NULL)
		return B_ERROR;

	info->imap_server = mxRec->serverName;
	info->pop_server =  mxRec->serverName;
	info->smtp_server =  mxRec->serverName;

	info->authentification_pop = 0;
	info->authentification_smtp = 0;
	info->username_pattern = 0;
	return B_OK;

}


status_t
AutoConfig::GuessServerName(const char* provider, provider_info* info)
{
	info->imap_server = "mail.";
	info->imap_server += provider;
	info->pop_server = "mail.";
	info->pop_server +=  provider;
	info->smtp_server = "mail.";
	info->smtp_server +=  provider;

	info->authentification_pop = 0;
	info->authentification_smtp = 0;
	info->username_pattern = 0;
	return B_OK;
}


void
AutoConfig::PrintProviderInfo(provider_info* pInfo)
{
	printf("Provider: %s:\n", pInfo->provider.String());
	printf("pop_mail_host: %s\n", pInfo->pop_server.String());
	printf("imap_mail_host: %s\n", pInfo->imap_server.String());
	printf("smtp_host: %s\n", pInfo->smtp_server.String());
	printf("pop authentication: %i\n", int(pInfo->authentification_pop));
	printf("smtp authentication: %i\n",
			int(pInfo->authentification_smtp));
	printf("username_pattern: %i\n",
			int(pInfo->username_pattern));
}


BString
AutoConfig::ExtractProvider(const char* email)
{
	BString emailS(email);
	BString provider;
	int32 at = emailS.FindLast("@");
	emailS.CopyInto(provider, at + 1, emailS.Length() - at);
	return provider;
}



status_t
AutoConfig::LoadProviderInfo(const BString &provider, provider_info* info)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	path.Append(INFO_DIR);
	BDirectory infoDir(path.Path());
	
	BFile infoFile(&infoDir, provider.String(), B_READ_ONLY);
	if (infoFile.InitCheck() != B_OK)
		return B_ENTRY_NOT_FOUND;

	info->provider = provider;
	if (ReadProviderInfo(&infoFile, info) == true)
		return B_OK;
	
	return B_ERROR;
}


bool
AutoConfig::ReadProviderInfo(BNode *node, provider_info* info)
{
	bool infoFound = false;
	char buffer[255];

	// server
	ssize_t size;
	size = node->ReadAttr(ATTR_NAME_POPSERVER, B_STRING_TYPE, 0, &buffer, 255);
	if (size > 0) {
		info->pop_server = buffer;
		infoFound = true;
	}
	size = node->ReadAttr(ATTR_NAME_IMAPSERVER, B_STRING_TYPE, 0, &buffer, 255);
	if (size > 0) {
		info->imap_server = buffer;
		infoFound = true;
	}
	size = node->ReadAttr(ATTR_NAME_SMTPSERVER, B_STRING_TYPE, 0, &buffer, 255);
	if (size > 0) {
		info->smtp_server = buffer;
		infoFound = true;
	}

	// authentication type
	int32 authType;
	size = node->ReadAttr(ATTR_NAME_AUTHPOP, B_INT32_TYPE, 0, &authType,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->authentification_pop = authType;
		infoFound = true;
	}
	size = node->ReadAttr(ATTR_NAME_AUTHSMTP, B_INT32_TYPE, 0, &authType,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->authentification_smtp = authType;
		infoFound = true;
	}

	// ssl
	int32 ssl;
	size = node->ReadAttr(ATTR_NAME_POPSSL, B_INT32_TYPE, 0, &ssl,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->ssl_pop = ssl;
		infoFound = true;
	}
	size = node->ReadAttr(ATTR_NAME_IMAPSSL, B_INT32_TYPE, 0, &ssl,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->ssl_imap = ssl;
		infoFound = true;
	}
	size = node->ReadAttr(ATTR_NAME_SMTPSSL, B_INT32_TYPE, 0, &ssl,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->ssl_smtp = ssl;
		infoFound = true;
	}

	// username pattern
	int32 pattern;
	size = node->ReadAttr(ATTR_NAME_USERNAME, B_INT32_TYPE, 0, &pattern,
							sizeof(int32));
	if (size == sizeof(int32)) {
		info->username_pattern = pattern;
		infoFound = true;
	}
	
	return infoFound;
}

