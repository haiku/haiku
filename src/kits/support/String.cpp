#include <String.h>
#include <cstdlib>
#include <cctype>

BString::BString()	
{
	_Init(NULL, 0);
}


BString::BString(const char* string)
{
	_Init(string, strlen(string));
}


BString::BString(const BString &string)		
{
	_Init(string.String(), string.Length());
}


BString::BString(const char *string, int32 maxLength)		
{
	int32 stringLen = strlen(string);
	int32 len = (maxLength <= stringLen ? maxLength : stringLen);
	_Init(string, len);
}


BString::~BString()
{
	if (_privateData) {
		_privateData -= sizeof(int32);
		free(_privateData);
	}
}


/*---- Access --------------------------------------------------------------*/
int32
BString::CountChars() const
{
	return 0; //TODO: Implement
}


/*---- Assignment ----------------------------------------------------------*/
BString&
BString::operator=(const BString &string)
{
	_DoAssign(string.String(), string.Length());
	return *this;
}


BString&
BString::operator=(const char *string)
{
	_DoAssign(string, strlen(string));	
	return *this;
}


BString&
BString::operator=(char c)
{
	char *tmp = (char*)calloc(2, sizeof(char));
	tmp[0] = c;
	_DoAssign(tmp, 1);
	free(tmp);
	
	return *this;
}


BString&
BString::SetTo(const char *string, int32 length)
{
	_DoAssign(string, length);
	return *this;
}


BString&
BString::SetTo(const BString &from)
{
	SetTo(from.String(), from.Length());
	return *this;
}


BString&
BString::Adopt(BString &from)
{
	//TODO: implement
	return *this;
}


BString&
BString::SetTo(const BString &string, int32 length)
{
	_DoAssign(string.String(), length);
	return *this;
}


BString&
BString::Adopt(BString &from, int32 length)
{
	//TODO: implement
	return *this;
}


BString&
BString::SetTo(char c, int32 count)
{
	char *tmp = (char*)calloc(count + 1, sizeof(char));
	memset(tmp, c, count);
	_DoAssign(tmp, count);
	free(tmp);
	
	return *this;	
}

/*---- Substring copying ---------------------------------------------------*/
void
BString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	int32 len = (Length() - fromOffset < length ? Length() - fromOffset : length);
	strncpy(into, _privateData + fromOffset, len); 
}


/*---- Appending -----------------------------------------------------------*/
BString&
BString::operator+=(const char *str)
{
	_DoAppend(str, strlen(str));
	return *this;
}


BString&
BString::operator+=(char c)
{
	char *tmp = (char*)calloc(2, sizeof(char));
	tmp[0] = c;
	_DoAppend(tmp, 1);
	free(tmp);
	return *this;
}


BString&
BString::Append(const BString &string, int32 length)
{
	_DoAppend(string.String(), length);
	return *this;
}


BString&
BString::Append(const char *str, int32 length)
{
	_DoAppend(str, length);
	return *this;
}


BString&
BString::Append(char c, int32 count)
{
	char *tmp = (char*)calloc(count + 1, sizeof(char));
	memset(tmp, c, count);
	_DoAppend(tmp, count);
	free(tmp);	
	return *this;
}


/*---- Prepending ----------------------------------------------------------*/
BString&
BString::Prepend(const char *str)
{
	_DoPrepend(str, strlen(str));
	return *this;
}


BString&
BString::Prepend(const BString &string)
{
	_DoPrepend(string.String(), string.Length());
	return *this;
}


BString&
BString::Prepend(const char *str, int32 len)
{
	_DoPrepend(str, len);
	return *this;
}


BString&
BString::Prepend(const BString &string, int32 len)
{
	_DoPrepend(string.String(), len);
	return *this;
}


BString&
BString::Prepend(char c, int32 count)
{
	char *tmp = (char*)calloc(count + 1, sizeof(char));
	memset(tmp, c, count);
	_DoPrepend(tmp, count);
	free(tmp);	
	return *this;
}


/*---- Inserting ----------------------------------------------------------*/

/*---- Simple sprintf replacement calls ------------------------------------*/
/*---- Slower than sprintf but type and overflow safe ----------------------*/
BString&
BString::operator<<(const char *str)
{
	_DoAppend(str, strlen(str));
	return *this;	
}


BString&
BString::operator<<(const BString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}


BString&
BString::operator<<(char c)
{
	char *tmp = (char*)calloc(2, sizeof(char));
	tmp[0] = c;
	_DoAppend(tmp, 1);
	free(tmp);
	
	return *this;
}


BString&
BString::operator<<(int i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(unsigned int i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(uint32 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(int32 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(uint64 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(int64 i)
{
	//TODO: implement
	return *this;
}

BString&
BString::operator<<(float f)
{
	//TODO: implement
	return *this;
}


/*---- Private or Reserved ------------------------------------------------*/
void
BString::_Init(const char* string, int32 len)
{
	_privateData = (char*)calloc(len + sizeof(int32) + 1, sizeof(char));
	_SetLength(len);
	_privateData += sizeof(int32);
	strncpy(_privateData, string, len);
}


void
BString::_DoAssign(const char *string, int32 len)
{	
	if (_privateData) {
		_privateData -= sizeof(int32);
		free(_privateData);
	}
	_privateData = (char*)calloc(len + sizeof(int32) + 1, sizeof(char));
	_SetLength(len);
	_privateData += sizeof(int32);
	strncpy(_privateData, string, len);
}


void
BString::_DoAppend(const char *str, int32 len)
{
	_privateData -= sizeof(int32);
	_privateData = _GrowBy(len);
	_SetLength(len);
	_privateData += sizeof(int32);
	strncat(_privateData, str, len);
}


char*
BString::_GrowBy(int32 size)
{
	int32 curLen = Length(); 
	_privateData = (char*)realloc(_privateData, curLen + size);
	return _privateData;
}


void
BString::_DoPrepend(const char *str, int32 count)
{
	char *tmp = strdup(_privateData);
	char *src = strdup(str);
	
	_privateData -= sizeof(int32);
	free(_privateData);
	_privateData = (char*)calloc(strlen(str) + 1 + sizeof(int32), sizeof(char));
	
	_SetLength(strlen(tmp) + count);
	_privateData += sizeof(int32);
	strncpy(_privateData, src, count);
	strcat(_privateData, tmp);
	free(tmp);
	free(src);
}


void
BString::_SetLength(int32 length)
{
	*(int32*)_privateData = length;
}