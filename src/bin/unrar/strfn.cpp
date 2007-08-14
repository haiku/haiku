#include "rar.hpp"

const char *NullToEmpty(const char *Str)
{
  return(Str==NULL ? "":Str);
}


const wchar *NullToEmpty(const wchar *Str)
{
  return(Str==NULL ? L"":Str);
}


char *IntNameToExt(const char *Name)
{
  static char OutName[NM];
  IntToExt(Name,OutName);
  return(OutName);
}


void ExtToInt(const char *Src,char *Dest)
{
#if defined(_WIN_32)
  CharToOem(Src,Dest);
#else
  if (Dest!=Src)
    strcpy(Dest,Src);
#endif
}


void IntToExt(const char *Src,char *Dest)
{
#if defined(_WIN_32)
  OemToChar(Src,Dest);
#else
  if (Dest!=Src)
    strcpy(Dest,Src);
#endif
}


char* strlower(char *Str)
{
#ifdef _WIN_32
  CharLower((LPTSTR)Str);
#else
  for (char *ChPtr=Str;*ChPtr;ChPtr++)
    *ChPtr=(char)loctolower(*ChPtr);
#endif
  return(Str);
}


char* strupper(char *Str)
{
#ifdef _WIN_32
  CharUpper((LPTSTR)Str);
#else
  for (char *ChPtr=Str;*ChPtr;ChPtr++)
    *ChPtr=(char)loctoupper(*ChPtr);
#endif
  return(Str);
}


int stricomp(const char *Str1,const char *Str2)
{
  char S1[NM*2],S2[NM*2];
  strncpyz(S1,Str1,ASIZE(S1));
  strncpyz(S2,Str2,ASIZE(S2));
  return(strcmp(strupper(S1),strupper(S2)));
}


int strnicomp(const char *Str1,const char *Str2,int N)
{
  char S1[NM*2],S2[NM*2];
  strncpyz(S1,Str1,ASIZE(S1));
  strncpyz(S2,Str2,ASIZE(S2));
  return(strncmp(strupper(S1),strupper(S2),N));
}


char* RemoveEOL(char *Str)
{
  for (int I=strlen(Str)-1;I>=0 && (Str[I]=='\r' || Str[I]=='\n' || Str[I]==' ' || Str[I]=='\t');I--)
    Str[I]=0;
  return(Str);
}


char* RemoveLF(char *Str)
{
  for (int I=strlen(Str)-1;I>=0 && (Str[I]=='\r' || Str[I]=='\n');I--)
    Str[I]=0;
  return(Str);
}


unsigned int loctolower(byte ch)
{
#ifdef _WIN_32
  return((int)CharLower((LPTSTR)ch));
#else
  return(tolower(ch));
#endif
}


unsigned int loctoupper(byte ch)
{
#ifdef _WIN_32
  return((int)CharUpper((LPTSTR)ch));
#else
  return(toupper(ch));
#endif
}


// toupper with English only results. Avoiding Turkish i -> I conversion
// problem
int etoupper(int ch)
{
  if (ch=='i')
    return('I');
  return(toupper(ch));
}






bool LowAscii(const char *Str)
{
  for (int I=0;Str[I]!=0;I++)
    if ((byte)Str[I]<32 || (byte)Str[I]>127)
      return(false);
  return(true);
}


bool LowAscii(const wchar *Str)
{
  for (int I=0;Str[I]!=0;I++)
    if (Str[I]<32 || Str[I]>127)
      return(false);
  return(true);
}




int stricompc(const char *Str1,const char *Str2)
{
#if defined(_UNIX)
  return(strcmp(Str1,Str2));
#else
  return(stricomp(Str1,Str2));
#endif
}


#ifndef SFX_MODULE
int stricompcw(const wchar *Str1,const wchar *Str2)
{
#if defined(_UNIX)
  return(strcmpw(Str1,Str2));
#else
  return(stricmpw(Str1,Str2));
#endif
}
#endif


// safe strncpy: copies maxlen-1 max and always returns zero terminated dest
char* strncpyz(char *dest, const char *src, size_t maxlen)
{
  if (maxlen>0)
  {
    strncpy(dest,src,maxlen-1);
    dest[maxlen-1]=0;
  }
  return(dest);
}

// safe strncpyw: copies maxlen-1 max and always returns zero terminated dest
wchar* strncpyzw(wchar *dest, const wchar *src, size_t maxlen)
{
  if (maxlen>0)
  {
    strncpyw(dest,src,maxlen-1);
    dest[maxlen-1]=0;
  }
  return(dest);
}
