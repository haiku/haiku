#ifndef CLIENT_FONT_LIST_H
#define CLIENT_FONT_LIST_H

#include <Font.h>	// for BFont-related definitions

#include <List.h>
#include <Message.h>
#include <OS.h>

class ClientFontList
{
public:
	ClientFontList(void);
	~ClientFontList(void);
	bool Update(bool check_only);
	int32 CountFamilies(void);
	status_t GetFamily(int32 index, font_family *name, uint32 *flags=NULL);
	int32 CountStyles(font_family f);
	status_t GetStyle(font_family family, int32 index, font_style *name,uint32 *flags=NULL, uint16 *face=NULL);
private:
	BList *familylist;
	sem_id fontlock;
};

#endif
