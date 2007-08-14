#ifndef _RAR_STRFN_
#define _RAR_STRFN_

const char *NullToEmpty(const char *Str);
const wchar *NullToEmpty(const wchar *Str);
char *IntNameToExt(const char *Name);
void ExtToInt(const char *Src,char *Dest);
void IntToExt(const char *Src,char *Dest);
char* strlower(char *Str);
char* strupper(char *Str);
int stricomp(const char *Str1,const char *Str2);
int strnicomp(const char *Str1,const char *Str2,int N);
char* RemoveEOL(char *Str);
char* RemoveLF(char *Str);
unsigned int loctolower(byte ch);
unsigned int loctoupper(byte ch);

char* strncpyz(char *dest, const char *src, size_t maxlen);
wchar* strncpyzw(wchar *dest, const wchar *src, size_t maxlen);

int etoupper(int ch);



bool LowAscii(const char *Str);
bool LowAscii(const wchar *Str);


int stricompc(const char *Str1,const char *Str2);
#ifndef SFX_MODULE
int stricompcw(const wchar *Str1,const wchar *Str2);
#endif

#endif
