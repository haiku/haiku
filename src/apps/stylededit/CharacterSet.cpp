#include "CharacterSet.h"
#include "Font.h"
#include <strings.h>

CharacterSet::CharacterSet()
{
	id = 0;
	MIBenum = 0;
	strcpy(print_name,"");
	strcpy(iana_name,"");
	mime_name = NULL;
	aliases = NULL;
	aliases_count = 0;
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

CharacterSetRoster::CharacterSetRoster()
{
	// this information should be loaded from a file
	character_sets_count = 25;
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
		strcpy(cs->print_name,"ISO Latin 1");
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
		strcpy(cs->print_name,"ISO Latin 2");
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
		strcpy(cs->print_name,"ISO Latin 3");
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
		strcpy(cs->print_name,"ISO Latin 4");
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
		strcpy(cs->print_name,"ISO Cyrillic");
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
		strcpy(cs->print_name,"ISO Arabic");
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
		strcpy(cs->print_name,"ISO Greek");
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
		strcpy(cs->print_name,"ISO Hebrew");
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
		strcpy(cs->print_name,"ISO Latin 5");
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
		strcpy(cs->print_name,"ISO Latin 6");
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
	{ // this is a guess
		cs = new CharacterSet();
		cs->id = B_MACINTOSH_ROMAN;
		cs->MIBenum = 2027;
		strcpy(cs->print_name,"Macintosh Roman");
		strcpy(cs->iana_name,"macintosh");
		cs->aliases_count = 2;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "mac";
		cs->aliases[1] = "csMacintosh";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 12;
		cs->MIBenum = 17;
		strcpy(cs->print_name,"Shift JIS");
		strcpy(cs->iana_name,"Shift_JIS");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 2;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "MS_Kanji";
		cs->aliases[1] = "csShiftJIS";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 13;
		cs->MIBenum = 17;
		strcpy(cs->print_name,"EUC Packed Format Japanese");
		strcpy(cs->iana_name,"Extended_UNIX_Code_Packed_Format_for_Japanese");
		cs->aliases_count = 2;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = cs->mime_name = "EUC-JP";
		cs->aliases[1] = "csEUCPkdFmtJapanese";
		character_sets[cs->id] = cs;
	}
	{ // this is just a guess...
		cs = new CharacterSet();
		cs->id = 14;
		cs->MIBenum = 19;
		strcpy(cs->print_name,"EUC Fixed Width Japanese");
		strcpy(cs->iana_name,"Extended_UNIX_Code_Fixed_Width_for_Japanese");
		cs->aliases_count = 1;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "csEUCFixWidJapanese";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 15;
		cs->MIBenum = 2252;
		strcpy(cs->print_name,"MS-Windows Codepage 1252");
		strcpy(cs->iana_name,"windows-1252");
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 16;
		cs->MIBenum = 1000;
		strcpy(cs->print_name,"Unicode 2.0");
		strcpy(cs->iana_name,"ISO-10646-UCS-2");
		cs->aliases_count = 1;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "csUnicode";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 17;
		cs->MIBenum = 2084;
		strcpy(cs->print_name,"KOI8-R Cyrillic");
		strcpy(cs->iana_name,"KOI8-R");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 1;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "csKOI8R";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 18;
		cs->MIBenum = 2251;
		strcpy(cs->print_name,"MS-Windows Codepage 1251");
		strcpy(cs->iana_name,"windows-1251");
		cs->mime_name = cs->iana_name;
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 19;
		cs->MIBenum = 2086;
		strcpy(cs->print_name,"IBM Codepage 866");
		strcpy(cs->iana_name,"IBM866");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 3;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "cp866";
		cs->aliases[1] = "866";
		cs->aliases[2] = "csIBM866";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 20;
		cs->MIBenum = 2011;
		strcpy(cs->print_name,"IBM Codepage 437");
		strcpy(cs->iana_name,"IBM437");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 3;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "cp437";
		cs->aliases[1] = "437";
		cs->aliases[2] = "csPC8CodePage437";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 21;
		cs->MIBenum = 38;
		strcpy(cs->print_name,"EUC Korean");
		strcpy(cs->iana_name,"EUC-KR");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 1;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "csEUCKR";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 22;
		cs->MIBenum = 109;
		strcpy(cs->print_name,"ISO 8859-13");
		strcpy(cs->iana_name,"ISO-8859-13");
		cs->mime_name = cs->iana_name;
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 23;
		cs->MIBenum = 110;
		strcpy(cs->print_name,"ISO 8859-14");
		strcpy(cs->iana_name,"ISO-8859-14");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 6;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "iso-ir-199";
		cs->aliases[1] = "ISO_8859-14:1998";
		cs->aliases[2] = "ISO_8859-14";
		cs->aliases[3] = "latin8";
		cs->aliases[4] = "iso-celtic";
		cs->aliases[5] = "l8";
		character_sets[cs->id] = cs;
	}
	{
		cs = new CharacterSet();
		cs->id = 24;
		cs->MIBenum = 111;
		strcpy(cs->print_name,"ISO 8859-15");
		strcpy(cs->iana_name,"ISO-8859-15");
		cs->mime_name = cs->iana_name;
		cs->aliases_count = 2;
		cs->aliases = new (char*)[cs->aliases_count];
		cs->aliases[0] = "ISO_8859-15";
		cs->aliases[1] = "Latin-9";
		character_sets[cs->id] = cs;
	}
}
