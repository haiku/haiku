#include <string.h>
#include <CharacterSet.h>
#include <Debug.h>
#include "character_sets.h"

namespace BPrivate {

/**
 * These variables are used in defining the character_sets_by_id array below.
 * @see http://www.iana.org/assignments/character-sets
 **/

const BCharacterSet unicode(0,106,"Unicode","UTF-8","UTF-8",NULL);

const char * isoLatin1aliases[] =
 { "iso-ir-100","ISO_8859-1","ISO-8859-1","latin1","11","IBM819","CP819","csISOLatin1",NULL };
const BCharacterSet isoLatin1(1,4,"ISO Latin 1","ISO_8859-1:1987","ISO_8859-1",isoLatin1aliases);

const char * isoLatin2aliases[] =
 { "iso-ir-101","ISO_8859-2","ISO-8859-2","latin2","12","csISOLatin2",NULL };
const BCharacterSet isoLatin2(2,5,"ISO Latin 2","ISO_8859-2:1987","ISO_8859-2",isoLatin2aliases);

const char * isoLatin3aliases[] =
 { "iso-ir-109","ISO_8859-3","ISO-8859-3","latin3","13","csISOLatin3",NULL };
const BCharacterSet isoLatin3(3,6,"ISO Latin 3","ISO_8859-3:1988","ISO_8859-3",isoLatin3aliases);

const char * isoLatin4aliases[] =
 { "iso-ir-110","ISO_8859-4","ISO-8859-4","latin4","14","csISOLatin4",NULL };
const BCharacterSet isoLatin4(4,7,"ISO Latin 4","ISO_8859-4:1988","ISO_8859-4",isoLatin4aliases);

const char * isoLatin5aliases[] =
 { "iso-ir-144","ISO_8859-5","ISO-8859-5","cyrillic","csISOLatinCyrillic",NULL };
const BCharacterSet isoLatin5(5,8,"ISO Cyrillic","ISO_8859-5:1988","ISO_8859-5",isoLatin5aliases);

const char * isoLatin6aliases[] =
 { "iso-ir-127","ISO_8859-6","ISO-8859-6","ECMA-114","ASMO-708","arabic","csISOLatinArabic",NULL };
const BCharacterSet isoLatin6(6,9,"ISO Arabic","ISO_8859-6:1987","ISO_8859-6",isoLatin6aliases);

const char * isoLatin7aliases[] =
 { "iso-ir-126","ISO_8859-7","ISO-8859-7","ELOT_928","ECMA-118","greek","greek8","csISOLatinGreek",NULL };
const BCharacterSet isoLatin7(7,10,"ISO Greek","ISO_8859-7:1987","ISO_8859-7",isoLatin7aliases);

const char * isoLatin8aliases[] =
 { "iso-ir-138","ISO_8859-8","ISO-8859-8","hebrew","csISOLatinHebrew",NULL };
const BCharacterSet isoLatin8(8,11,"ISO Hebrew","ISO_8859-8:1988","ISO-8859-8",isoLatin8aliases);

const char * isoLatin9aliases[] =
 { "iso-ir-148","ISO_8859-9","ISO-8859-9","latin5","15","csISOLatin5",NULL };
const BCharacterSet isoLatin9(9,12,"ISO Latin 5","ISO_8859-9:1989","ISO-8859-9",isoLatin9aliases);

const char * isoLatin10aliases[] =
 { "iso-ir-157","16","ISO_8859-10:1992","csISOLatin6","latin6",NULL };
const BCharacterSet isoLatin10(10,13,"ISO Latin 6","ISO-8859-10","ISO-8859-10",isoLatin10aliases);

const char * macintoshAliases[] =
 { "mac","csMacintosh",NULL };
const BCharacterSet macintosh(11,2027,"Macintosh Roman","macintosh",NULL,macintoshAliases);

const char * shiftJISaliases[] =
 { "MS_Kanji","csShiftJIS",NULL };
const BCharacterSet shiftJIS(12,17,"Shift JIS","Shift_JIS","Shift_JIS",shiftJISaliases);

const char * EUCPackedJapaneseAliases[] =
 { "EUC-JP","csEUCPkdFmtJapanese",NULL };
const BCharacterSet packedJapanese(13,18,"EUC Packed Format Japanese",
                                   "Extended_UNIX_Code_Packed_Format_for_Japanese","EUC-JP",
                                   EUCPackedJapaneseAliases);

const char * JIS0208aliases[] =
 { "iso-ir-87","x0208","JIS_X0208-1983","csISO87JISX0208",NULL };
const BCharacterSet JIS0208(14,63,"JIS 0208","JIS_C6226-1983",NULL,JIS0208aliases);

const BCharacterSet windows1252(15,2252,"MS-Windows Codepage 1252","windows-1252",NULL,NULL);

const char * unicode2aliases[] =
 { "csUnicode",NULL };
const BCharacterSet unicode2(16,1000,"Unicode 2.0","ISO-10646-UCS-2",NULL,unicode2aliases);

const char * KOI8Raliases[] =
 { "csKOI8R",NULL };
const BCharacterSet KOI8R(17,2084,"KOI8-R Cyrillic","KOI8-R","KOI8-R",KOI8Raliases);

const BCharacterSet windows1251(18,2251,"MS-Windows Codepage 1251","windows-1251",NULL,NULL);

const char * IBM866aliases[] =
 { "cp866","866","csIBM866",NULL };
const BCharacterSet IBM866(19,2086,"IBM Codepage 866","IBM866","IBM866",IBM866aliases);

const char * IBM437aliases[] =
 { "cp437","437","csPC8CodePage437",NULL };
const BCharacterSet IBM437(20,2011,"IBM Codepage 437","IBM437","IBM437",IBM437aliases);

const char * eucKRaliases[] =
 { "csEUCKR",NULL };
const BCharacterSet eucKR(21,38,"EUC Korean","EUC-KR","EUC-KR",eucKRaliases);

const BCharacterSet iso13(22,109,"ISO 8859-13","ISO-8859-13","ISO-8859-13",NULL);

const char * iso14aliases[] =
 { "iso-ir-199","ISO_8859-14:1998","ISO_8859-14","latin8","iso-celtic","l8",NULL };
const BCharacterSet iso14(23,110,"ISO 8859-14","ISO-8859-14","ISO-8859-14",iso14aliases);

const char * iso15aliases[] =
 { "ISO_8859-14","Latin-9",NULL };
const BCharacterSet iso15(24,111,"ISO 8859-15","ISO-8859-15","ISO-8859-15",iso15aliases);

// chinese character set testing

const char * big5aliases[] =
 { "csBig5",NULL };
const BCharacterSet big5(25,2026,"Big5","Big5","Big5",big5aliases);

const BCharacterSet gb18030(26,114,"GB18030","GB18030",NULL,NULL);

/**
 * The following initializes the global character set array.
 * It is organized by id for efficient retrieval using predefined constants in UTF8.h and Font.h.
 * Character sets are stored contiguously and may be efficiently iterated over.
 * To add a new character set, define the character set above -- remember to increment the id --
 * and then add &<charSetName> to the _end_ of the following list.  That's all.
 **/

const BCharacterSet * character_sets_by_id[] = {
	&unicode,
	&isoLatin1, &isoLatin2, &isoLatin3,	&isoLatin4,	&isoLatin5,
	&isoLatin6,	&isoLatin7, &isoLatin8, &isoLatin9, &isoLatin10,
	&macintosh,
	// R5 BFont encodings end here
	&shiftJIS, &packedJapanese, &JIS0208,
	&windows1252, &unicode2, &KOI8R, &windows1251,
	&IBM866, &IBM437, &eucKR, &iso13, &iso14, &iso15,
	// R5 convert_to/from_utf8 encodings end here
	&big5,&gb18030,
};
const uint32 character_sets_by_id_count = sizeof(character_sets_by_id)/sizeof(const BCharacterSet*);

/**
 * The following code initializes the global MIBenum array.
 * This sparsely populated array exists as an efficient way to access character sets by MIBenum.
 * The MIBenum array is automatically allocated, and initialized by the following class.
 * The following class should only be instantiated once, this is assured by using an assertion.
 * No changes are required to the following code to add a new character set.
 **/

const BCharacterSet ** character_sets_by_MIBenum;
uint32 maximum_valid_MIBenum;

class MIBenumArrayInitializer {
public:
	MIBenumArrayInitializer() {
		DEBUG_ONLY(static int onlyOneTime = 0;)
		ASSERT_WITH_MESSAGE(onlyOneTime++ == 0,"MIBenumArrayInitializer should be instantiated only one time.");
		// analyzing character_sets_by_id
		uint32 max_MIBenum = 0;
		for (uint32 index = 0 ; index < character_sets_by_id_count ; index++ ) {
			if (max_MIBenum < character_sets_by_id[index]->GetMIBenum()) {
				max_MIBenum = character_sets_by_id[index]->GetMIBenum();
			}
		}
		// initializing extern variables
		character_sets_by_MIBenum = new (const BCharacterSet*)[max_MIBenum+2];
		maximum_valid_MIBenum = max_MIBenum;
		// initializing MIBenum array
		memset(character_sets_by_MIBenum,0,sizeof(BCharacterSet*)*(max_MIBenum+2));
		for (uint32 index2 = 0 ; index2 < character_sets_by_id_count ; index2++ ) {
			const BCharacterSet * charset = character_sets_by_id[index2];
			character_sets_by_MIBenum[charset->GetMIBenum()] = charset;
		}
	}
	~MIBenumArrayInitializer()
	{
		delete [] character_sets_by_MIBenum;
	}
} runTheInitializer;

}

