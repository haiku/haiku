#include "rar.hpp"

static bool match(char *pattern,char *string,bool ForceCase);
static bool match(wchar *pattern,wchar *string,bool ForceCase);

static int mstricompc(const char *Str1,const char *Str2,bool ForceCase);
static int mstricompcw(const wchar *Str1,const wchar *Str2,bool ForceCase);
static int mstrnicompc(const char *Str1,const char *Str2,int N,bool ForceCase);
static int mstrnicompcw(const wchar *Str1,const wchar *Str2,int N,bool ForceCase);

inline uint toupperc(byte ch,bool ForceCase)
{
  if (ForceCase)
    return(ch);
#ifdef _WIN_32
  return((uint)CharUpper((LPTSTR)(ch)));
#elif defined(_UNIX)
  return(ch);
#else
  return(toupper(ch));
#endif
}


inline uint touppercw(uint ch,bool ForceCase)
{
  if (ForceCase)
    return(ch);
#if defined(_UNIX)
  return(ch);
#else
  return(toupperw(ch));
#endif
}


bool CmpName(char *Wildcard,char *Name,int CmpPath)
{
  bool ForceCase=(CmpPath&MATCH_FORCECASESENSITIVE)!=0;

  CmpPath&=MATCH_MODEMASK;
  
  if (CmpPath!=MATCH_NAMES)
  {
    int WildLength=strlen(Wildcard);
    if (CmpPath!=MATCH_EXACTPATH && mstrnicompc(Wildcard,Name,WildLength,ForceCase)==0)
    {
      char NextCh=Name[WildLength];
      if (NextCh=='\\' || NextCh=='/' || NextCh==0)
        return(true);
    }
    char Path1[NM],Path2[NM];
    GetFilePath(Wildcard,Path1,ASIZE(Path1));
    GetFilePath(Name,Path2,ASIZE(Path1));
    if (mstricompc(Wildcard,Path2,ForceCase)==0)
      return(true);
    if ((CmpPath==MATCH_PATH || CmpPath==MATCH_EXACTPATH) && mstricompc(Path1,Path2,ForceCase)!=0)
      return(false);
    if (CmpPath==MATCH_SUBPATH || CmpPath==MATCH_WILDSUBPATH)
      if (IsWildcard(Path1))
        return(match(Wildcard,Name,ForceCase));
      else
        if (CmpPath==MATCH_SUBPATH || IsWildcard(Wildcard))
        {
          if (*Path1 && mstrnicompc(Path1,Path2,strlen(Path1),ForceCase)!=0)
            return(false);
        }
        else
          if (mstricompc(Path1,Path2,ForceCase)!=0)
            return(false);
  }
  char *Name1=PointToName(Wildcard);
  char *Name2=PointToName(Name);
  if (mstrnicompc("__rar_",Name2,6,false)==0)
    return(false);
  return(match(Name1,Name2,ForceCase));
}


#ifndef SFX_MODULE
bool CmpName(wchar *Wildcard,wchar *Name,int CmpPath)
{
  bool ForceCase=(CmpPath&MATCH_FORCECASESENSITIVE)!=0;

  CmpPath&=MATCH_MODEMASK;

  if (CmpPath!=MATCH_NAMES)
  {
    int WildLength=strlenw(Wildcard);
    if (CmpPath!=MATCH_EXACTPATH && mstrnicompcw(Wildcard,Name,WildLength,ForceCase)==0)
    {
      wchar NextCh=Name[WildLength];
      if (NextCh==L'\\' || NextCh==L'/' || NextCh==0)
        return(true);
    }
    wchar Path1[NM],Path2[NM];
    GetFilePath(Wildcard,Path1,ASIZE(Path1));
    GetFilePath(Name,Path2,ASIZE(Path2));
    if ((CmpPath==MATCH_PATH || CmpPath==MATCH_EXACTPATH) && mstricompcw(Path1,Path2,ForceCase)!=0)
      return(false);
    if (CmpPath==MATCH_SUBPATH || CmpPath==MATCH_WILDSUBPATH)
      if (IsWildcard(NULL,Path1))
        return(match(Wildcard,Name,ForceCase));
      else
        if (CmpPath==MATCH_SUBPATH || IsWildcard(NULL,Wildcard))
        {
          if (*Path1 && mstrnicompcw(Path1,Path2,strlenw(Path1),ForceCase)!=0)
            return(false);
        }
        else
          if (mstricompcw(Path1,Path2,ForceCase)!=0)
            return(false);
  }
  wchar *Name1=PointToName(Wildcard);
  wchar *Name2=PointToName(Name);
  if (mstrnicompcw(L"__rar_",Name2,6,false)==0)
    return(false);
  return(match(Name1,Name2,ForceCase));
}
#endif


bool match(char *pattern,char *string,bool ForceCase)
{
  for (;; ++string)
  {
    char stringc=toupperc(*string,ForceCase);
    char patternc=toupperc(*pattern++,ForceCase);
    switch (patternc)
    {
      case 0:
        return(stringc==0);
      case '?':
        if (stringc == 0)
          return(false);
        break;
      case '*':
        if (*pattern==0)
          return(true);
        if (*pattern=='.')
        {
          if (pattern[1]=='*' && pattern[2]==0)
            return(true);
          char *dot=strchr(string,'.');
          if (pattern[1]==0)
            return (dot==NULL || dot[1]==0);
          if (dot!=NULL)
          {
            string=dot;
            if (strpbrk(pattern,"*?")==NULL && strchr(string+1,'.')==NULL)
              return(mstricompc(pattern+1,string+1,ForceCase)==0);
          }
        }

        while (*string)
          if (match(pattern,string++,ForceCase))
            return(true);
        return(false);
      default:
        if (patternc != stringc)
          if (patternc=='.' && stringc==0)
            return(match(pattern,string,ForceCase));
          else
            return(false);
        break;
    }
  }
}


#ifndef SFX_MODULE
bool match(wchar *pattern,wchar *string,bool ForceCase)
{
  for (;; ++string)
  {
    wchar stringc=touppercw(*string,ForceCase);
    wchar patternc=touppercw(*pattern++,ForceCase);
    switch (patternc)
    {
      case 0:
        return(stringc==0);
      case '?':
        if (stringc == 0)
          return(false);
        break;
      case '*':
        if (*pattern==0)
          return(true);
        if (*pattern=='.')
        {
          if (pattern[1]=='*' && pattern[2]==0)
            return(true);
          wchar *dot=strchrw(string,'.');
          if (pattern[1]==0)
            return (dot==NULL || dot[1]==0);
          if (dot!=NULL)
          {
            string=dot;
            if (strpbrkw(pattern,L"*?")==NULL && strchrw(string+1,'.')==NULL)
              return(mstricompcw(pattern+1,string+1,ForceCase)==0);
          }
        }

        while (*string)
          if (match(pattern,string++,ForceCase))
            return(true);
        return(false);
      default:
        if (patternc != stringc)
          if (patternc=='.' && stringc==0)
            return(match(pattern,string,ForceCase));
          else
            return(false);
        break;
    }
  }
}
#endif


int mstricompc(const char *Str1,const char *Str2,bool ForceCase)
{
  if (ForceCase)
    return(strcmp(Str1,Str2));
  return(stricompc(Str1,Str2));
}


#ifndef SFX_MODULE
int mstricompcw(const wchar *Str1,const wchar *Str2,bool ForceCase)
{
  if (ForceCase)
    return(strcmpw(Str1,Str2));
  return(stricompcw(Str1,Str2));
}
#endif


int mstrnicompc(const char *Str1,const char *Str2,int N,bool ForceCase)
{
  if (ForceCase)
    return(strncmp(Str1,Str2,N));
#if defined(_UNIX)
  return(strncmp(Str1,Str2,N));
#else
  return(strnicomp(Str1,Str2,N));
#endif
}


#ifndef SFX_MODULE
int mstrnicompcw(const wchar *Str1,const wchar *Str2,int N,bool ForceCase)
{
  if (ForceCase)
    return(strncmpw(Str1,Str2,N));
#if defined(_UNIX)
  return(strncmpw(Str1,Str2,N));
#else
  return(strnicmpw(Str1,Str2,N));
#endif
}
#endif
