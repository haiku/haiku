/* crypt - simple encryption algorithm used for passwords
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>

#include <string.h>
#include <crypt.h>


static const char key[PASSWORD_LENGTH + 1] = "Dr. Zoidberg Enterprises, BeMail";


_EXPORT char *get_passwd(const BMessage *msg,const char *name)
{
	char *encryptedPassword;
	ssize_t length;
	if (msg->FindData(name,B_RAW_TYPE,(const void **)&encryptedPassword,&length) < B_OK || !encryptedPassword || length == 0)
		return NULL;

	char *buffer = new char[length];
	passwd_crypt(encryptedPassword,buffer,length);

	return buffer;
}


_EXPORT bool set_passwd(BMessage *msg,const char *name,const char *password)
{
	if (!password)
		return false;

	ssize_t length = strlen(password) + 1;
	char *buffer = new char[length];
	passwd_crypt((char *)password,buffer,length);

	msg->RemoveName(name);
	status_t status = msg->AddData(name,B_RAW_TYPE,buffer,length,false);

	delete [] buffer;
	return (status >= B_OK);
}


_EXPORT void passwd_crypt(char *in,char *out,int length)
{
	int i;

	memcpy(out,in,length);
	if (length > PASSWORD_LENGTH)
		length = PASSWORD_LENGTH;

	for (i = 0;i < length;i++)
		out[i] ^= key[i];
}

