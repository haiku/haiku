#include "CharacterSet.h"
#include "Font.h"
#include <strings.h>

CharacterSet::CharacterSet()
{
	id = 0;
	MIBenum = 0;
	strcpy(print_name,"");
	strcpy(iana_name,"");
	mime_name = "";
	aliases = NULL;
	aliases_count = 0;
}

CharacterSetRoster::CharacterSetRoster()
{
	// this information should be loaded from a file
	character_sets_count = 11;
	character_sets = new (CharacterSet*)[character_sets_count];
	CharacterSet * cs;
	{
		cs = new CharacterSet();
		cs->id = B_UNICODE_UTF8; // a.k.a. 0
		cs->MIBenum = 106;
		strcpy(cs->print_name,"Unicode");
		strcpy(cs->iana_name,"UTF-8");
		cs->mime_name = cs->iana_name;
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_1;
		cs->MIBenum = 4;
		strcpy(cs->print_name,"Latin 1");
		strcpy(cs->iana_name,"ISO_8859-1:1987");
		cs->aliases_count = 8;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-100";
		cs->aliases[1] = "ISO_8859-1";
		cs->aliases[2] = cs->mime_name = "ISO-8859-1";
		cs->aliases[3] = "latin1";
		cs->aliases[4] = "11";
		cs->aliases[5] = "IBM819";
		cs->aliases[6] = "CP819";
		cs->aliases[7] = "csISOLatin1";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_2;
		cs->MIBenum = 5;
		strcpy(cs->print_name,"Latin 2");
		strcpy(cs->iana_name,"ISO_8859-2:1987");
		cs->aliases_count = 6;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-101";
		cs->aliases[1] = "ISO_8859-2";
		cs->aliases[2] = cs->mime_name = "ISO-8859-2";
		cs->aliases[3] = "latin2";
		cs->aliases[4] = "12";
		cs->aliases[5] = "csISOLatin2";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_3;
		cs->MIBenum = 6;
		strcpy(cs->print_name,"Latin 3");
		strcpy(cs->iana_name,"ISO_8859-3:1988");
		cs->mime_name = cs->print_name;
		cs->aliases_count = 6;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-109";
		cs->aliases[1] = "ISO_8859-3";
		cs->aliases[2] = cs->mime_name = "ISO-8859-3";
		cs->aliases[3] = "latin3";
		cs->aliases[4] = "13";
		cs->aliases[5] = "csISOLatin3";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_4;
		cs->MIBenum = 7;
		strcpy(cs->print_name,"Latin 4");
		strcpy(cs->iana_name,"ISO_8859-4:1988");
		cs->aliases_count = 6;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-110";
		cs->aliases[1] = "ISO_8859-4";
		cs->aliases[2] = cs->mime_name = "ISO-8859-4";
		cs->aliases[3] = "latin4";
		cs->aliases[4] = "14";
		cs->aliases[5] = "csISOLatin4";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_5;
		cs->MIBenum = 8;
		strcpy(cs->print_name,"Cyrillic");
		strcpy(cs->iana_name,"ISO_8859-5:1988");
		cs->aliases_count = 5;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-144";
		cs->aliases[1] = "ISO_8859-5";
		cs->aliases[2] = cs->mime_name = "ISO-8859-5";
		cs->aliases[3] = "cyrillic";
		cs->aliases[4] = "csISOLatinCyrillic";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_6;
		cs->MIBenum = 9;
		strcpy(cs->print_name,"Arabic");
		strcpy(cs->iana_name,"ISO_8859-6:1987");
		cs->aliases_count = 7;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-127";
		cs->aliases[1] = "ISO_8859-6";
		cs->aliases[2] = cs->mime_name = "ISO-8859-6";
		cs->aliases[5] = "ECMA-114";
		cs->aliases[5] = "ASMO-708";
		cs->aliases[5] = "arabic";
		cs->aliases[6] = "csISOLatinArabic";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_7;
		cs->MIBenum = 10;
		strcpy(cs->print_name,"Greek");
		strcpy(cs->iana_name,"ISO_8859-7:1987");
		cs->aliases_count = 8;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-126";
		cs->aliases[1] = "ISO_8859-7";
		cs->aliases[2] = cs->mime_name = "ISO-8859-7";
		cs->aliases[5] = "ELOT_928";
		cs->aliases[5] = "ECMA-118";
		cs->aliases[5] = "greek";
		cs->aliases[6] = "greek8";
		cs->aliases[7] = "csISOLatinGreek";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_8;
		cs->MIBenum = 11;
		strcpy(cs->print_name,"Hebrew");
		strcpy(cs->iana_name,"ISO_8859-8:1988");
		cs->aliases_count = 5;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-138";
		cs->aliases[1] = "ISO_8859-8";
		cs->aliases[2] = cs->mime_name = "ISO-8859-8";
		cs->aliases[3] = "hebrew";
		cs->aliases[4] = "csISOLatinHebrew";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_9;
		cs->MIBenum = 12;
		strcpy(cs->print_name,"Latin 5");
		strcpy(cs->iana_name,"ISO_8859-9:1989");
		cs->aliases_count = 6;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-148";
		cs->aliases[1] = "ISO_8859-9";
		cs->aliases[2] = cs->mime_name = "ISO-8859-9";
		cs->aliases[3] = "latin5";
		cs->aliases[4] = "15";
		cs->aliases[5] = "csISOLatin5";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = B_ISO_8859_10;
		cs->MIBenum = 12;
		strcpy(cs->print_name,"Latin 6");
		strcpy(cs->iana_name,"ISO_8859-10");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 5;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-157";
		cs->aliases[1] = "16";
		cs->aliases[2] = "ISO-8859-10:1992";
		cs->aliases[3] = "csISOLatin6";
		cs->aliases[4] = "latin6";
		character_sets[cs->id] = cs;
	}
}

CharacterSetRoster * theRoster = NULL;

CharacterSetRoster *
CharacterSetRoster::Roster(status_t * outError)
{
	// TODO: add synchronization protection
	if (theRoster == NULL) {
		theRoster = new CharacterSetRoster();
	}
	if (outError != NULL) {
		*outError = (theRoster == NULL ? B_NO_MEMORY : B_OK);
	}
	return theRoster;
}

CharacterSetRoster *
CharacterSetRoster::CurrentRoster()
{
	return theRoster;
}

const CharacterSet * 
CharacterSetRoster::FindCharacterSetByFontID(uint32 id)
{
	return character_sets[id];
}

const CharacterSet * 
CharacterSetRoster::FindCharacterSetByConversionID(uint32 id)
{
	return character_sets[id+1];
}

const CharacterSet * 
CharacterSetRoster::FindCharacterSetByMIBenum(uint32 MIBenum)
{
	for (int id = 0 ; (id < character_sets_count) ; id++) {
		if (character_sets[id]->MIBenum == MIBenum) {
			return character_sets[id];
		}
	}
	return 0;
}

const CharacterSet * 
CharacterSetRoster::FindCharacterSetByPrintName(char * name)
{
	for (int id = 0 ; (id < character_sets_count) ; id++) {
		if (strcmp(character_sets[id]->print_name,name) == 0) {
			return character_sets[id];
		}
	}
	return 0;
}

const CharacterSet * 
CharacterSetRoster::FindCharacterSetByName(char * name)
{
	for (int id = 0 ; (id < character_sets_count) ; id++) {
		if (strcmp(character_sets[id]->iana_name,name) == 0) {
			return character_sets[id];
		}
		for (int alias = 0 ; (alias < character_sets[id]->aliases_count) ; alias++) {
			if (strcmp(character_sets[id]->aliases[alias],name) == 0) {
				return character_sets[id];
			}
		}
	}
	return 0;
}
