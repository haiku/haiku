// Author:      Sebastian Nozzi
// Created:     3 may 2002

// Modifications:
// (please include author, date, and description)

// mmu_man@sf.net: note the original one doesn't link to libbe

#ifndef _ADDATTR_MSG_H
#define _ADDATTR_MSG_H

extern char *completeToolName;

void usageMsg();
void invalidAttrMsg( const char *attrTypeName );
void problemsWithFileMsg( const char *file );

#endif
