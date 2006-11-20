#ifndef _RAR_MATCH_
#define _RAR_MATCH_

enum {MATCH_NAMES,MATCH_PATH,MATCH_EXACTPATH,MATCH_SUBPATH,MATCH_WILDSUBPATH};

#define MATCH_MODEMASK           0x0000ffff
#define MATCH_FORCECASESENSITIVE 0x80000000

bool CmpName(char *Wildcard,char *Name,int CmpPath);
bool CmpName(wchar *Wildcard,wchar *Name,int CmpPath);

#endif
