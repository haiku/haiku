/* ANSI-C code produced by gperf version 3.0 */
/* Command-line: gperf -m 10 lib/aliases.gperf  */
/* Computed positions: -k'1,3-11,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "lib/aliases.gperf"
struct alias { int name; unsigned int encoding_index; };

#define TOTAL_KEYWORDS 324
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 45
#define MIN_HASH_VALUE 15
#define MAX_HASH_VALUE 879
/* maximum key range = 865, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
aliases_hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880,   6,  69, 880,  44,   5,
        6,  18,  60,   9,   8,  50,  14,  13, 271, 880,
      880, 880, 880, 880, 880, 107, 152,   5,  29,   7,
       43, 112,  42,   5, 341, 106,  10, 158,   8,   5,
        6, 880,  61,  38,  98, 152, 194, 112,  30,  10,
        6, 880, 880, 880, 880,  60, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880, 880, 880,
      880, 880, 880, 880, 880, 880, 880, 880
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct stringpool_t
  {
    char stringpool_str15[sizeof("CN")];
    char stringpool_str17[sizeof("L1")];
    char stringpool_str18[sizeof("L2")];
    char stringpool_str20[sizeof("L6")];
    char stringpool_str21[sizeof("L5")];
    char stringpool_str26[sizeof("L8")];
    char stringpool_str29[sizeof("862")];
    char stringpool_str30[sizeof("L3")];
    char stringpool_str33[sizeof("866")];
    char stringpool_str34[sizeof("C99")];
    char stringpool_str38[sizeof("EUCCN")];
    char stringpool_str41[sizeof("CP1251")];
    char stringpool_str43[sizeof("CP1252")];
    char stringpool_str44[sizeof("CP862")];
    char stringpool_str45[sizeof("EUC-CN")];
    char stringpool_str47[sizeof("CP1256")];
    char stringpool_str48[sizeof("CP866")];
    char stringpool_str49[sizeof("CP1255")];
    char stringpool_str50[sizeof("HZ")];
    char stringpool_str52[sizeof("CP1361")];
    char stringpool_str53[sizeof("CP932")];
    char stringpool_str55[sizeof("CP819")];
    char stringpool_str57[sizeof("CP936")];
    char stringpool_str58[sizeof("X0212")];
    char stringpool_str59[sizeof("CP1258")];
    char stringpool_str62[sizeof("L7")];
    char stringpool_str67[sizeof("CP1253")];
    char stringpool_str72[sizeof("L4")];
    char stringpool_str75[sizeof("CP1133")];
    char stringpool_str77[sizeof("R8")];
    char stringpool_str84[sizeof("CHINESE")];
    char stringpool_str85[sizeof("ISO8859-1")];
    char stringpool_str87[sizeof("ISO8859-2")];
    char stringpool_str91[sizeof("ISO8859-6")];
    char stringpool_str92[sizeof("ISO-8859-1")];
    char stringpool_str93[sizeof("ISO8859-5")];
    char stringpool_str94[sizeof("ISO-8859-2")];
    char stringpool_str95[sizeof("X0201")];
    char stringpool_str97[sizeof("ISO8859-16")];
    char stringpool_str98[sizeof("ISO-8859-6")];
    char stringpool_str99[sizeof("ISO8859-15")];
    char stringpool_str100[sizeof("ISO-8859-5")];
    char stringpool_str101[sizeof("ISO8859-9")];
    char stringpool_str103[sizeof("ISO8859-8")];
    char stringpool_str104[sizeof("ISO-8859-16")];
    char stringpool_str105[sizeof("850")];
    char stringpool_str106[sizeof("ISO-8859-15")];
    char stringpool_str108[sizeof("ISO-8859-9")];
    char stringpool_str109[sizeof("CP949")];
    char stringpool_str110[sizeof("ISO-8859-8")];
    char stringpool_str111[sizeof("ISO8859-3")];
    char stringpool_str112[sizeof("ISO-IR-6")];
    char stringpool_str113[sizeof("X0208")];
    char stringpool_str114[sizeof("CYRILLIC")];
    char stringpool_str116[sizeof("ISO-2022-CN")];
    char stringpool_str117[sizeof("ISO8859-13")];
    char stringpool_str118[sizeof("ISO-8859-3")];
    char stringpool_str119[sizeof("CP1250")];
    char stringpool_str120[sizeof("CP950")];
    char stringpool_str121[sizeof("CP850")];
    char stringpool_str122[sizeof("ISO646-CN")];
    char stringpool_str123[sizeof("SJIS")];
    char stringpool_str124[sizeof("ISO-8859-13")];
    char stringpool_str125[sizeof("ISO-IR-126")];
    char stringpool_str126[sizeof("ISO-IR-226")];
    char stringpool_str127[sizeof("ISO-IR-166")];
    char stringpool_str129[sizeof("ISO-IR-165")];
    char stringpool_str131[sizeof("CP1257")];
    char stringpool_str132[sizeof("ASCII")];
    char stringpool_str134[sizeof("ISO-IR-58")];
    char stringpool_str136[sizeof("CP367")];
    char stringpool_str137[sizeof("LATIN1")];
    char stringpool_str138[sizeof("ISO-IR-159")];
    char stringpool_str139[sizeof("LATIN2")];
    char stringpool_str142[sizeof("ISO-IR-199")];
    char stringpool_str143[sizeof("LATIN6")];
    char stringpool_str145[sizeof("LATIN5")];
    char stringpool_str146[sizeof("ISO_8859-1")];
    char stringpool_str147[sizeof("CSISO2022CN")];
    char stringpool_str148[sizeof("ISO_8859-2")];
    char stringpool_str149[sizeof("ISO-IR-138")];
    char stringpool_str151[sizeof("CP1254")];
    char stringpool_str152[sizeof("ISO_8859-6")];
    char stringpool_str154[sizeof("ISO_8859-5")];
    char stringpool_str155[sizeof("LATIN8")];
    char stringpool_str157[sizeof("ISO-IR-101")];
    char stringpool_str158[sizeof("ISO_8859-16")];
    char stringpool_str159[sizeof("GB2312")];
    char stringpool_str160[sizeof("ISO_8859-15")];
    char stringpool_str161[sizeof("ISO-CELTIC")];
    char stringpool_str162[sizeof("ISO_8859-9")];
    char stringpool_str163[sizeof("LATIN3")];
    char stringpool_str164[sizeof("ISO_8859-8")];
    char stringpool_str165[sizeof("UHC")];
    char stringpool_str169[sizeof("ISO8859-10")];
    char stringpool_str170[sizeof("ISO_8859-15:1998")];
    char stringpool_str171[sizeof("MAC")];
    char stringpool_str172[sizeof("ISO_8859-3")];
    char stringpool_str173[sizeof("ISO-IR-109")];
    char stringpool_str175[sizeof("ISO8859-7")];
    char stringpool_str176[sizeof("ISO-8859-10")];
    char stringpool_str177[sizeof("CSASCII")];
    char stringpool_str178[sizeof("ISO_8859-13")];
    char stringpool_str179[sizeof("ISO-IR-179")];
    char stringpool_str182[sizeof("ISO-8859-7")];
    char stringpool_str184[sizeof("ISO-IR-203")];
    char stringpool_str189[sizeof("ISO-IR-149")];
    char stringpool_str190[sizeof("MS-EE")];
    char stringpool_str191[sizeof("ISO-IR-148")];
    char stringpool_str192[sizeof("US")];
    char stringpool_str194[sizeof("CP874")];
    char stringpool_str195[sizeof("ISO8859-4")];
    char stringpool_str196[sizeof("ISO-IR-110")];
    char stringpool_str197[sizeof("ISO_8859-10:1992")];
    char stringpool_str199[sizeof("ISO_8859-16:2000")];
    char stringpool_str201[sizeof("ISO8859-14")];
    char stringpool_str202[sizeof("ISO-8859-4")];
    char stringpool_str203[sizeof("IBM862")];
    char stringpool_str206[sizeof("ISO-IR-57")];
    char stringpool_str207[sizeof("IBM866")];
    char stringpool_str208[sizeof("ISO-8859-14")];
    char stringpool_str209[sizeof("ISO-IR-127")];
    char stringpool_str210[sizeof("ISO-2022-CN-EXT")];
    char stringpool_str211[sizeof("ISO-IR-87")];
    char stringpool_str212[sizeof("ISO-IR-157")];
    char stringpool_str213[sizeof("UCS-2")];
    char stringpool_str214[sizeof("IBM819")];
    char stringpool_str221[sizeof("ISO_8859-14:1998")];
    char stringpool_str222[sizeof("ISO-IR-14")];
    char stringpool_str225[sizeof("ELOT_928")];
    char stringpool_str227[sizeof("LATIN7")];
    char stringpool_str228[sizeof("UTF-16")];
    char stringpool_str230[sizeof("ISO_8859-10")];
    char stringpool_str232[sizeof("CSUNICODE")];
    char stringpool_str233[sizeof("UCS-2LE")];
    char stringpool_str234[sizeof("UTF-8")];
    char stringpool_str235[sizeof("ISO-IR-100")];
    char stringpool_str236[sizeof("ISO_8859-7")];
    char stringpool_str237[sizeof("UTF-32")];
    char stringpool_str238[sizeof("CHAR")];
    char stringpool_str241[sizeof("UNICODE-1-1")];
    char stringpool_str242[sizeof("CSUNICODE11")];
    char stringpool_str244[sizeof("TIS620")];
    char stringpool_str245[sizeof("EUCKR")];
    char stringpool_str246[sizeof("UTF-16LE")];
    char stringpool_str247[sizeof("LATIN4")];
    char stringpool_str250[sizeof("KSC_5601")];
    char stringpool_str251[sizeof("TIS-620")];
    char stringpool_str252[sizeof("EUC-KR")];
    char stringpool_str254[sizeof("IBM-CP1133")];
    char stringpool_str256[sizeof("ISO_8859-4")];
    char stringpool_str257[sizeof("UTF-32LE")];
    char stringpool_str258[sizeof("VISCII")];
    char stringpool_str259[sizeof("KOI8-R")];
    char stringpool_str262[sizeof("ISO_8859-14")];
    char stringpool_str264[sizeof("CSKOI8R")];
    char stringpool_str266[sizeof("GREEK8")];
    char stringpool_str267[sizeof("MS-CYRL")];
    char stringpool_str270[sizeof("CSVISCII")];
    char stringpool_str280[sizeof("IBM850")];
    char stringpool_str283[sizeof("ISO-IR-144")];
    char stringpool_str286[sizeof("BIG5")];
    char stringpool_str287[sizeof("UCS-4LE")];
    char stringpool_str288[sizeof("GB18030")];
    char stringpool_str290[sizeof("MACCYRILLIC")];
    char stringpool_str291[sizeof("CSUNICODE11UTF7")];
    char stringpool_str292[sizeof("UNICODE-1-1-UTF-7")];
    char stringpool_str293[sizeof("BIG-5")];
    char stringpool_str295[sizeof("IBM367")];
    char stringpool_str296[sizeof("TIS620-0")];
    char stringpool_str298[sizeof("CSBIG5")];
    char stringpool_str299[sizeof("NEXTSTEP")];
    char stringpool_str301[sizeof("CSKSC56011987")];
    char stringpool_str302[sizeof("CSISOLATIN1")];
    char stringpool_str303[sizeof("KOREAN")];
    char stringpool_str304[sizeof("CSISOLATIN2")];
    char stringpool_str305[sizeof("CN-BIG5")];
    char stringpool_str306[sizeof("UTF-7")];
    char stringpool_str308[sizeof("CSISOLATIN6")];
    char stringpool_str309[sizeof("CSISOLATINCYRILLIC")];
    char stringpool_str310[sizeof("CSISOLATIN5")];
    char stringpool_str312[sizeof("TCVN")];
    char stringpool_str315[sizeof("TIS620.2529-1")];
    char stringpool_str318[sizeof("CSGB2312")];
    char stringpool_str320[sizeof("ISO-10646-UCS-2")];
    char stringpool_str321[sizeof("UCS-4")];
    char stringpool_str322[sizeof("MULELAO-1")];
    char stringpool_str323[sizeof("ISO-2022-KR")];
    char stringpool_str324[sizeof("ECMA-118")];
    char stringpool_str325[sizeof("GB_2312-80")];
    char stringpool_str326[sizeof("CSUCS4")];
    char stringpool_str327[sizeof("GBK")];
    char stringpool_str328[sizeof("CSISOLATIN3")];
    char stringpool_str329[sizeof("ISO646-US")];
    char stringpool_str331[sizeof("US-ASCII")];
    char stringpool_str332[sizeof("TIS620.2533-1")];
    char stringpool_str333[sizeof("KOI8-T")];
    char stringpool_str334[sizeof("MS-ANSI")];
    char stringpool_str335[sizeof("KS_C_5601-1989")];
    char stringpool_str336[sizeof("GB_1988-80")];
    char stringpool_str339[sizeof("EUCTW")];
    char stringpool_str343[sizeof("GREEK")];
    char stringpool_str346[sizeof("EUC-TW")];
    char stringpool_str347[sizeof("WINDOWS-1251")];
    char stringpool_str348[sizeof("WINDOWS-1252")];
    char stringpool_str349[sizeof("JP")];
    char stringpool_str350[sizeof("WINDOWS-1256")];
    char stringpool_str351[sizeof("WINDOWS-1255")];
    char stringpool_str353[sizeof("VISCII1.1-1")];
    char stringpool_str354[sizeof("CSISO2022KR")];
    char stringpool_str356[sizeof("WINDOWS-1258")];
    char stringpool_str360[sizeof("WINDOWS-1253")];
    char stringpool_str361[sizeof("ARMSCII-8")];
    char stringpool_str366[sizeof("CSIBM866")];
    char stringpool_str368[sizeof("ROMAN8")];
    char stringpool_str369[sizeof("HZ-GB-2312")];
    char stringpool_str370[sizeof("EUCJP")];
    char stringpool_str371[sizeof("TIS620.2533-0")];
    char stringpool_str372[sizeof("KS_C_5601-1987")];
    char stringpool_str373[sizeof("MACICELAND")];
    char stringpool_str374[sizeof("ISO-10646-UCS-4")];
    char stringpool_str375[sizeof("UCS-2BE")];
    char stringpool_str377[sizeof("EUC-JP")];
    char stringpool_str386[sizeof("WINDOWS-1250")];
    char stringpool_str387[sizeof("ARABIC")];
    char stringpool_str388[sizeof("UTF-16BE")];
    char stringpool_str391[sizeof("TCVN-5712")];
    char stringpool_str392[sizeof("WINDOWS-1257")];
    char stringpool_str394[sizeof("CSPC862LATINHEBREW")];
    char stringpool_str396[sizeof("TCVN5712-1")];
    char stringpool_str399[sizeof("UTF-32BE")];
    char stringpool_str402[sizeof("WINDOWS-1254")];
    char stringpool_str404[sizeof("CSEUCKR")];
    char stringpool_str406[sizeof("ASMO-708")];
    char stringpool_str409[sizeof("CSISOLATINARABIC")];
    char stringpool_str410[sizeof("MACINTOSH")];
    char stringpool_str411[sizeof("UCS-2-INTERNAL")];
    char stringpool_str412[sizeof("CSISOLATIN4")];
    char stringpool_str416[sizeof("ECMA-114")];
    char stringpool_str418[sizeof("CN-GB-ISOIR165")];
    char stringpool_str420[sizeof("ANSI_X3.4-1986")];
    char stringpool_str421[sizeof("CSISO57GB1988")];
    char stringpool_str423[sizeof("CSISO58GB231280")];
    char stringpool_str424[sizeof("HP-ROMAN8")];
    char stringpool_str426[sizeof("ANSI_X3.4-1968")];
    char stringpool_str427[sizeof("MACTHAI")];
    char stringpool_str429[sizeof("UCS-4BE")];
    char stringpool_str430[sizeof("CSHPROMAN8")];
    char stringpool_str432[sizeof("CN-GB")];
    char stringpool_str434[sizeof("UNICODELITTLE")];
    char stringpool_str435[sizeof("ISO_8859-5:1988")];
    char stringpool_str438[sizeof("ISO_8859-9:1989")];
    char stringpool_str440[sizeof("ISO_8859-8:1988")];
    char stringpool_str441[sizeof("KOI8-U")];
    char stringpool_str444[sizeof("ISO_8859-3:1988")];
    char stringpool_str448[sizeof("ISO-2022-JP")];
    char stringpool_str449[sizeof("ISO-2022-JP-1")];
    char stringpool_str450[sizeof("ISO-2022-JP-2")];
    char stringpool_str451[sizeof("CSISOLATINHEBREW")];
    char stringpool_str454[sizeof("ISO646-JP")];
    char stringpool_str457[sizeof("EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE")];
    char stringpool_str465[sizeof("UCS-4-INTERNAL")];
    char stringpool_str467[sizeof("ISO_8859-1:1987")];
    char stringpool_str468[sizeof("ISO_8859-2:1987")];
    char stringpool_str470[sizeof("ISO_8859-6:1987")];
    char stringpool_str479[sizeof("CSISO2022JP")];
    char stringpool_str480[sizeof("CSISO2022JP2")];
    char stringpool_str486[sizeof("ISO_8859-4:1988")];
    char stringpool_str489[sizeof("MACCENTRALEUROPE")];
    char stringpool_str492[sizeof("HEBREW")];
    char stringpool_str494[sizeof("MS-HEBR")];
    char stringpool_str496[sizeof("ISO_646.IRV:1991")];
    char stringpool_str498[sizeof("CSEUCTW")];
    char stringpool_str503[sizeof("KOI8-RU")];
    char stringpool_str505[sizeof("WINDOWS-874")];
    char stringpool_str508[sizeof("JIS0208")];
    char stringpool_str509[sizeof("GEORGIAN-PS")];
    char stringpool_str512[sizeof("ISO_8859-7:1987")];
    char stringpool_str514[sizeof("CSISOLATINGREEK")];
    char stringpool_str515[sizeof("JIS_C6226-1983")];
    char stringpool_str518[sizeof("MACROMAN")];
    char stringpool_str519[sizeof("UCS-2-SWAPPED")];
    char stringpool_str524[sizeof("CSMACINTOSH")];
    char stringpool_str527[sizeof("BIGFIVE")];
    char stringpool_str528[sizeof("CSISO159JISX02121990")];
    char stringpool_str529[sizeof("CSISO14JISC6220RO")];
    char stringpool_str530[sizeof("CSPC850MULTILINGUAL")];
    char stringpool_str534[sizeof("BIG-FIVE")];
    char stringpool_str541[sizeof("JIS_C6220-1969-RO")];
    char stringpool_str545[sizeof("JIS_X0212")];
    char stringpool_str549[sizeof("BIG5HKSCS")];
    char stringpool_str553[sizeof("JISX0201-1976")];
    char stringpool_str554[sizeof("GEORGIAN-ACADEMY")];
    char stringpool_str556[sizeof("BIG5-HKSCS")];
    char stringpool_str560[sizeof("CSISO87JISX0208")];
    char stringpool_str570[sizeof("MACGREEK")];
    char stringpool_str571[sizeof("MS-GREEK")];
    char stringpool_str573[sizeof("UCS-4-SWAPPED")];
    char stringpool_str578[sizeof("MACCROATIAN")];
    char stringpool_str582[sizeof("JIS_X0201")];
    char stringpool_str585[sizeof("WCHAR_T")];
    char stringpool_str594[sizeof("UNICODEBIG")];
    char stringpool_str599[sizeof("JIS_X0212-1990")];
    char stringpool_str600[sizeof("JIS_X0208")];
    char stringpool_str614[sizeof("MACARABIC")];
    char stringpool_str619[sizeof("CSHALFWIDTHKATAKANA")];
    char stringpool_str620[sizeof("JIS_X0208-1983")];
    char stringpool_str621[sizeof("SHIFT-JIS")];
    char stringpool_str626[sizeof("MACUKRAINE")];
    char stringpool_str635[sizeof("CSEUCPKDFMTJAPANESE")];
    char stringpool_str646[sizeof("JIS_X0208-1990")];
    char stringpool_str663[sizeof("CSSHIFTJIS")];
    char stringpool_str664[sizeof("JIS_X0212.1990-0")];
    char stringpool_str665[sizeof("MACHEBREW")];
    char stringpool_str675[sizeof("SHIFT_JIS")];
    char stringpool_str685[sizeof("TCVN5712-1:1993")];
    char stringpool_str694[sizeof("MS-TURK")];
    char stringpool_str717[sizeof("MACTURKISH")];
    char stringpool_str731[sizeof("MACROMANIA")];
    char stringpool_str750[sizeof("MS-ARAB")];
    char stringpool_str753[sizeof("JAVA")];
    char stringpool_str798[sizeof("MS_KANJI")];
    char stringpool_str799[sizeof("JOHAB")];
    char stringpool_str879[sizeof("WINBALTRIM")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "CN",
    "L1",
    "L2",
    "L6",
    "L5",
    "L8",
    "862",
    "L3",
    "866",
    "C99",
    "EUCCN",
    "CP1251",
    "CP1252",
    "CP862",
    "EUC-CN",
    "CP1256",
    "CP866",
    "CP1255",
    "HZ",
    "CP1361",
    "CP932",
    "CP819",
    "CP936",
    "X0212",
    "CP1258",
    "L7",
    "CP1253",
    "L4",
    "CP1133",
    "R8",
    "CHINESE",
    "ISO8859-1",
    "ISO8859-2",
    "ISO8859-6",
    "ISO-8859-1",
    "ISO8859-5",
    "ISO-8859-2",
    "X0201",
    "ISO8859-16",
    "ISO-8859-6",
    "ISO8859-15",
    "ISO-8859-5",
    "ISO8859-9",
    "ISO8859-8",
    "ISO-8859-16",
    "850",
    "ISO-8859-15",
    "ISO-8859-9",
    "CP949",
    "ISO-8859-8",
    "ISO8859-3",
    "ISO-IR-6",
    "X0208",
    "CYRILLIC",
    "ISO-2022-CN",
    "ISO8859-13",
    "ISO-8859-3",
    "CP1250",
    "CP950",
    "CP850",
    "ISO646-CN",
    "SJIS",
    "ISO-8859-13",
    "ISO-IR-126",
    "ISO-IR-226",
    "ISO-IR-166",
    "ISO-IR-165",
    "CP1257",
    "ASCII",
    "ISO-IR-58",
    "CP367",
    "LATIN1",
    "ISO-IR-159",
    "LATIN2",
    "ISO-IR-199",
    "LATIN6",
    "LATIN5",
    "ISO_8859-1",
    "CSISO2022CN",
    "ISO_8859-2",
    "ISO-IR-138",
    "CP1254",
    "ISO_8859-6",
    "ISO_8859-5",
    "LATIN8",
    "ISO-IR-101",
    "ISO_8859-16",
    "GB2312",
    "ISO_8859-15",
    "ISO-CELTIC",
    "ISO_8859-9",
    "LATIN3",
    "ISO_8859-8",
    "UHC",
    "ISO8859-10",
    "ISO_8859-15:1998",
    "MAC",
    "ISO_8859-3",
    "ISO-IR-109",
    "ISO8859-7",
    "ISO-8859-10",
    "CSASCII",
    "ISO_8859-13",
    "ISO-IR-179",
    "ISO-8859-7",
    "ISO-IR-203",
    "ISO-IR-149",
    "MS-EE",
    "ISO-IR-148",
    "US",
    "CP874",
    "ISO8859-4",
    "ISO-IR-110",
    "ISO_8859-10:1992",
    "ISO_8859-16:2000",
    "ISO8859-14",
    "ISO-8859-4",
    "IBM862",
    "ISO-IR-57",
    "IBM866",
    "ISO-8859-14",
    "ISO-IR-127",
    "ISO-2022-CN-EXT",
    "ISO-IR-87",
    "ISO-IR-157",
    "UCS-2",
    "IBM819",
    "ISO_8859-14:1998",
    "ISO-IR-14",
    "ELOT_928",
    "LATIN7",
    "UTF-16",
    "ISO_8859-10",
    "CSUNICODE",
    "UCS-2LE",
    "UTF-8",
    "ISO-IR-100",
    "ISO_8859-7",
    "UTF-32",
    "CHAR",
    "UNICODE-1-1",
    "CSUNICODE11",
    "TIS620",
    "EUCKR",
    "UTF-16LE",
    "LATIN4",
    "KSC_5601",
    "TIS-620",
    "EUC-KR",
    "IBM-CP1133",
    "ISO_8859-4",
    "UTF-32LE",
    "VISCII",
    "KOI8-R",
    "ISO_8859-14",
    "CSKOI8R",
    "GREEK8",
    "MS-CYRL",
    "CSVISCII",
    "IBM850",
    "ISO-IR-144",
    "BIG5",
    "UCS-4LE",
    "GB18030",
    "MACCYRILLIC",
    "CSUNICODE11UTF7",
    "UNICODE-1-1-UTF-7",
    "BIG-5",
    "IBM367",
    "TIS620-0",
    "CSBIG5",
    "NEXTSTEP",
    "CSKSC56011987",
    "CSISOLATIN1",
    "KOREAN",
    "CSISOLATIN2",
    "CN-BIG5",
    "UTF-7",
    "CSISOLATIN6",
    "CSISOLATINCYRILLIC",
    "CSISOLATIN5",
    "TCVN",
    "TIS620.2529-1",
    "CSGB2312",
    "ISO-10646-UCS-2",
    "UCS-4",
    "MULELAO-1",
    "ISO-2022-KR",
    "ECMA-118",
    "GB_2312-80",
    "CSUCS4",
    "GBK",
    "CSISOLATIN3",
    "ISO646-US",
    "US-ASCII",
    "TIS620.2533-1",
    "KOI8-T",
    "MS-ANSI",
    "KS_C_5601-1989",
    "GB_1988-80",
    "EUCTW",
    "GREEK",
    "EUC-TW",
    "WINDOWS-1251",
    "WINDOWS-1252",
    "JP",
    "WINDOWS-1256",
    "WINDOWS-1255",
    "VISCII1.1-1",
    "CSISO2022KR",
    "WINDOWS-1258",
    "WINDOWS-1253",
    "ARMSCII-8",
    "CSIBM866",
    "ROMAN8",
    "HZ-GB-2312",
    "EUCJP",
    "TIS620.2533-0",
    "KS_C_5601-1987",
    "MACICELAND",
    "ISO-10646-UCS-4",
    "UCS-2BE",
    "EUC-JP",
    "WINDOWS-1250",
    "ARABIC",
    "UTF-16BE",
    "TCVN-5712",
    "WINDOWS-1257",
    "CSPC862LATINHEBREW",
    "TCVN5712-1",
    "UTF-32BE",
    "WINDOWS-1254",
    "CSEUCKR",
    "ASMO-708",
    "CSISOLATINARABIC",
    "MACINTOSH",
    "UCS-2-INTERNAL",
    "CSISOLATIN4",
    "ECMA-114",
    "CN-GB-ISOIR165",
    "ANSI_X3.4-1986",
    "CSISO57GB1988",
    "CSISO58GB231280",
    "HP-ROMAN8",
    "ANSI_X3.4-1968",
    "MACTHAI",
    "UCS-4BE",
    "CSHPROMAN8",
    "CN-GB",
    "UNICODELITTLE",
    "ISO_8859-5:1988",
    "ISO_8859-9:1989",
    "ISO_8859-8:1988",
    "KOI8-U",
    "ISO_8859-3:1988",
    "ISO-2022-JP",
    "ISO-2022-JP-1",
    "ISO-2022-JP-2",
    "CSISOLATINHEBREW",
    "ISO646-JP",
    "EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE",
    "UCS-4-INTERNAL",
    "ISO_8859-1:1987",
    "ISO_8859-2:1987",
    "ISO_8859-6:1987",
    "CSISO2022JP",
    "CSISO2022JP2",
    "ISO_8859-4:1988",
    "MACCENTRALEUROPE",
    "HEBREW",
    "MS-HEBR",
    "ISO_646.IRV:1991",
    "CSEUCTW",
    "KOI8-RU",
    "WINDOWS-874",
    "JIS0208",
    "GEORGIAN-PS",
    "ISO_8859-7:1987",
    "CSISOLATINGREEK",
    "JIS_C6226-1983",
    "MACROMAN",
    "UCS-2-SWAPPED",
    "CSMACINTOSH",
    "BIGFIVE",
    "CSISO159JISX02121990",
    "CSISO14JISC6220RO",
    "CSPC850MULTILINGUAL",
    "BIG-FIVE",
    "JIS_C6220-1969-RO",
    "JIS_X0212",
    "BIG5HKSCS",
    "JISX0201-1976",
    "GEORGIAN-ACADEMY",
    "BIG5-HKSCS",
    "CSISO87JISX0208",
    "MACGREEK",
    "MS-GREEK",
    "UCS-4-SWAPPED",
    "MACCROATIAN",
    "JIS_X0201",
    "WCHAR_T",
    "UNICODEBIG",
    "JIS_X0212-1990",
    "JIS_X0208",
    "MACARABIC",
    "CSHALFWIDTHKATAKANA",
    "JIS_X0208-1983",
    "SHIFT-JIS",
    "MACUKRAINE",
    "CSEUCPKDFMTJAPANESE",
    "JIS_X0208-1990",
    "CSSHIFTJIS",
    "JIS_X0212.1990-0",
    "MACHEBREW",
    "SHIFT_JIS",
    "TCVN5712-1:1993",
    "MS-TURK",
    "MACTURKISH",
    "MACROMANIA",
    "MS-ARAB",
    "JAVA",
    "MS_KANJI",
    "JOHAB",
    "WINBALTRIM"
  };
#define stringpool ((const char *) &stringpool_contents)

static const struct alias aliases[] =
  {
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 271 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str15, ei_iso646_cn},
    {-1},
#line 60 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str17, ei_iso8859_1},
#line 68 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str18, ei_iso8859_2},
    {-1},
#line 133 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str20, ei_iso8859_10},
#line 125 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str21, ei_iso8859_9},
    {-1}, {-1}, {-1}, {-1},
#line 147 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str26, ei_iso8859_14},
    {-1}, {-1},
#line 196 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str29, ei_cp862},
#line 76 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str30, ei_iso8859_3},
    {-1}, {-1},
#line 200 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str33, ei_cp866},
#line 51 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str34, ei_c99},
    {-1}, {-1}, {-1},
#line 301 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str38, ei_euc_cn},
    {-1}, {-1},
#line 167 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str41, ei_cp1251},
    {-1},
#line 170 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str43, ei_cp1252},
#line 194 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str44, ei_cp862},
#line 300 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str45, ei_euc_cn},
    {-1},
#line 182 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str47, ei_cp1256},
#line 198 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str48, ei_cp866},
#line 179 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str49, ei_cp1255},
#line 311 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str50, ei_hz},
    {-1},
#line 331 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str52, ei_johab},
#line 294 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str53, ei_cp932},
    {-1},
#line 57 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str55, ei_iso8859_1},
    {-1},
#line 306 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str57, ei_ces_gbk},
#line 265 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str58, ei_jisx0212},
#line 188 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str59, ei_cp1258},
    {-1}, {-1},
#line 140 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str62, ei_iso8859_13},
    {-1}, {-1}, {-1}, {-1},
#line 173 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str67, ei_cp1253},
    {-1}, {-1}, {-1}, {-1},
#line 84 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str72, ei_iso8859_4},
    {-1}, {-1},
#line 227 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str75, ei_cp1133},
    {-1},
#line 219 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str77, ei_hp_roman8},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 276 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str84, ei_gb2312},
#line 62 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str85, ei_iso8859_1},
    {-1},
#line 70 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str87, ei_iso8859_2},
    {-1}, {-1}, {-1},
#line 102 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str91, ei_iso8859_6},
#line 53 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str92, ei_iso8859_1},
#line 93 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str93, ei_iso8859_5},
#line 63 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str94, ei_iso8859_2},
#line 252 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str95, ei_jisx0201},
    {-1},
#line 159 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str97, ei_iso8859_16},
#line 94 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str98, ei_iso8859_6},
#line 154 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str99, ei_iso8859_15},
#line 87 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str100, ei_iso8859_5},
#line 127 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str101, ei_iso8859_9},
    {-1},
#line 119 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str103, ei_iso8859_8},
#line 155 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str104, ei_iso8859_16},
#line 192 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str105, ei_cp850},
#line 150 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str106, ei_iso8859_15},
    {-1},
#line 120 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str108, ei_iso8859_9},
#line 328 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str109, ei_cp949},
#line 113 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str110, ei_iso8859_8},
#line 78 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str111, ei_iso8859_3},
#line 16 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str112, ei_ascii},
#line 258 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str113, ei_jisx0208},
#line 91 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str114, ei_iso8859_5},
    {-1},
#line 308 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str116, ei_iso2022_cn},
#line 141 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str117, ei_iso8859_13},
#line 71 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str118, ei_iso8859_3},
#line 164 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str119, ei_cp1250},
#line 322 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str120, ei_cp950},
#line 190 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str121, ei_cp850},
#line 269 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str122, ei_iso646_cn},
#line 291 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str123, ei_sjis},
#line 136 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str124, ei_iso8859_13},
#line 106 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str125, ei_iso8859_7},
#line 158 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str126, ei_iso8859_16},
#line 235 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str127, ei_tis620},
    {-1},
#line 277 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str129, ei_isoir165},
    {-1},
#line 185 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str131, ei_cp1257},
#line 13 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str132, ei_ascii},
    {-1},
#line 274 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str134, ei_gb2312},
    {-1},
#line 19 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str136, ei_ascii},
#line 59 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str137, ei_iso8859_1},
#line 266 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str138, ei_jisx0212},
#line 67 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str139, ei_iso8859_2},
    {-1}, {-1},
#line 145 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str142, ei_iso8859_14},
#line 132 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str143, ei_iso8859_10},
    {-1},
#line 124 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str145, ei_iso8859_9},
#line 54 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str146, ei_iso8859_1},
#line 309 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str147, ei_iso2022_cn},
#line 64 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str148, ei_iso8859_2},
#line 116 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str149, ei_iso8859_8},
    {-1},
#line 176 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str151, ei_cp1254},
#line 95 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str152, ei_iso8859_6},
    {-1},
#line 88 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str154, ei_iso8859_5},
#line 146 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str155, ei_iso8859_14},
    {-1},
#line 66 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str157, ei_iso8859_2},
#line 156 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str158, ei_iso8859_16},
#line 302 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str159, ei_euc_cn},
#line 151 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str160, ei_iso8859_15},
#line 148 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str161, ei_iso8859_14},
#line 121 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str162, ei_iso8859_9},
#line 75 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str163, ei_iso8859_3},
#line 114 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str164, ei_iso8859_8},
#line 329 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str165, ei_cp949},
    {-1}, {-1}, {-1},
#line 135 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str169, ei_iso8859_10},
#line 152 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str170, ei_iso8859_15},
#line 204 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str171, ei_mac_roman},
#line 72 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str172, ei_iso8859_3},
#line 74 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str173, ei_iso8859_3},
    {-1},
#line 112 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str175, ei_iso8859_7},
#line 128 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str176, ei_iso8859_10},
#line 22 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str177, ei_ascii},
#line 137 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str178, ei_iso8859_13},
#line 138 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str179, ei_iso8859_13},
    {-1}, {-1},
#line 103 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str182, ei_iso8859_7},
    {-1},
#line 153 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str184, ei_iso8859_15},
    {-1}, {-1}, {-1}, {-1},
#line 282 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str189, ei_ksc5601},
#line 166 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str190, ei_cp1250},
#line 123 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str191, ei_iso8859_9},
#line 21 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str192, ei_ascii},
    {-1},
#line 236 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str194, ei_cp874},
#line 86 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str195, ei_iso8859_4},
#line 82 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str196, ei_iso8859_4},
#line 130 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str197, ei_iso8859_10},
    {-1},
#line 157 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str199, ei_iso8859_16},
    {-1},
#line 149 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str201, ei_iso8859_14},
#line 79 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str202, ei_iso8859_4},
#line 195 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str203, ei_cp862},
    {-1}, {-1},
#line 270 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str206, ei_iso646_cn},
#line 199 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str207, ei_cp866},
#line 142 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str208, ei_iso8859_14},
#line 97 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str209, ei_iso8859_6},
#line 310 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str210, ei_iso2022_cn_ext},
#line 259 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str211, ei_jisx0208},
#line 131 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str212, ei_iso8859_10},
#line 24 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str213, ei_ucs2},
#line 58 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str214, ei_iso8859_1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 144 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str221, ei_iso8859_14},
#line 247 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str222, ei_iso646_jp},
    {-1}, {-1},
#line 108 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str225, ei_iso8859_7},
    {-1},
#line 139 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str227, ei_iso8859_13},
#line 38 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str228, ei_utf16},
    {-1},
#line 129 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str230, ei_iso8859_10},
    {-1},
#line 26 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str232, ei_ucs2},
#line 31 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str233, ei_ucs2le},
#line 23 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str234, ei_utf8},
#line 56 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str235, ei_iso8859_1},
#line 104 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str236, ei_iso8859_7},
#line 41 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str237, ei_utf32},
#line 334 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str238, ei_local_char},
    {-1}, {-1},
#line 29 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str241, ei_ucs2be},
#line 30 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str242, ei_ucs2be},
    {-1},
#line 230 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str244, ei_tis620},
#line 326 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str245, ei_euc_kr},
#line 40 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str246, ei_utf16le},
#line 83 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str247, ei_iso8859_4},
    {-1}, {-1},
#line 279 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str250, ei_ksc5601},
#line 229 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str251, ei_tis620},
#line 325 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str252, ei_euc_kr},
    {-1},
#line 228 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str254, ei_cp1133},
    {-1},
#line 80 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str256, ei_iso8859_4},
#line 43 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str257, ei_utf32le},
#line 238 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str258, ei_viscii},
#line 160 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str259, ei_koi8_r},
    {-1}, {-1},
#line 143 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str262, ei_iso8859_14},
    {-1},
#line 161 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str264, ei_koi8_r},
    {-1},
#line 109 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str266, ei_iso8859_7},
#line 169 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str267, ei_cp1251},
    {-1}, {-1},
#line 240 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str270, ei_viscii},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 191 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str280, ei_cp850},
    {-1}, {-1},
#line 90 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str283, ei_iso8859_5},
    {-1}, {-1},
#line 316 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str286, ei_ces_big5},
#line 37 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str287, ei_ucs4le},
#line 307 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str288, ei_gb18030},
    {-1},
#line 210 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str290, ei_mac_cyrillic},
#line 46 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str291, ei_utf7},
#line 45 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str292, ei_utf7},
#line 317 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str293, ei_ces_big5},
    {-1},
#line 20 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str295, ei_ascii},
#line 231 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str296, ei_tis620},
    {-1},
#line 321 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str298, ei_ces_big5},
#line 221 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str299, ei_nextstep},
    {-1},
#line 283 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str301, ei_ksc5601},
#line 61 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str302, ei_iso8859_1},
#line 284 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str303, ei_ksc5601},
#line 69 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str304, ei_iso8859_2},
#line 320 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str305, ei_ces_big5},
#line 44 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str306, ei_utf7},
    {-1},
#line 134 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str308, ei_iso8859_10},
#line 92 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str309, ei_iso8859_5},
#line 126 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str310, ei_iso8859_9},
    {-1},
#line 241 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str312, ei_tcvn},
    {-1}, {-1},
#line 232 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str315, ei_tis620},
    {-1}, {-1},
#line 304 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str318, ei_euc_cn},
    {-1},
#line 25 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str320, ei_ucs2},
#line 33 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str321, ei_ucs4},
#line 226 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str322, ei_mulelao},
#line 332 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str323, ei_iso2022_kr},
#line 107 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str324, ei_iso8859_7},
#line 273 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str325, ei_gb2312},
#line 35 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str326, ei_ucs4},
#line 305 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str327, ei_ces_gbk},
#line 77 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str328, ei_iso8859_3},
#line 14 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str329, ei_ascii},
    {-1},
#line 12 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str331, ei_ascii},
#line 234 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str332, ei_tis620},
#line 225 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str333, ei_koi8_t},
#line 172 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str334, ei_cp1252},
#line 281 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str335, ei_ksc5601},
#line 268 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str336, ei_iso646_cn},
    {-1}, {-1},
#line 314 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str339, ei_euc_tw},
    {-1}, {-1}, {-1},
#line 110 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str343, ei_iso8859_7},
    {-1}, {-1},
#line 313 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str346, ei_euc_tw},
#line 168 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str347, ei_cp1251},
#line 171 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str348, ei_cp1252},
#line 248 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str349, ei_iso646_jp},
#line 183 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str350, ei_cp1256},
#line 180 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str351, ei_cp1255},
    {-1},
#line 239 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str353, ei_viscii},
#line 333 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str354, ei_iso2022_kr},
    {-1},
#line 189 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str356, ei_cp1258},
    {-1}, {-1}, {-1},
#line 174 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str360, ei_cp1253},
#line 222 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str361, ei_armscii_8},
    {-1}, {-1}, {-1}, {-1},
#line 201 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str366, ei_cp866},
    {-1},
#line 218 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str368, ei_hp_roman8},
#line 312 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str369, ei_hz},
#line 286 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str370, ei_euc_jp},
#line 233 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str371, ei_tis620},
#line 280 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str372, ei_ksc5601},
#line 207 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str373, ei_mac_iceland},
#line 34 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str374, ei_ucs4},
#line 27 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str375, ei_ucs2be},
    {-1},
#line 285 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str377, ei_euc_jp},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 165 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str386, ei_cp1250},
#line 100 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str387, ei_iso8859_6},
#line 39 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str388, ei_utf16be},
    {-1}, {-1},
#line 242 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str391, ei_tcvn},
#line 186 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str392, ei_cp1257},
    {-1},
#line 197 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str394, ei_cp862},
    {-1},
#line 243 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str396, ei_tcvn},
    {-1}, {-1},
#line 42 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str399, ei_utf32be},
    {-1}, {-1},
#line 177 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str402, ei_cp1254},
    {-1},
#line 327 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str404, ei_euc_kr},
    {-1},
#line 99 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str406, ei_iso8859_6},
    {-1}, {-1},
#line 101 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str409, ei_iso8859_6},
#line 203 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str410, ei_mac_roman},
#line 47 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str411, ei_ucs2internal},
#line 85 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str412, ei_iso8859_4},
    {-1}, {-1}, {-1},
#line 98 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str416, ei_iso8859_6},
    {-1},
#line 278 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str418, ei_isoir165},
    {-1},
#line 18 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str420, ei_ascii},
#line 272 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str421, ei_iso646_cn},
    {-1},
#line 275 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str423, ei_gb2312},
#line 217 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str424, ei_hp_roman8},
    {-1},
#line 17 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str426, ei_ascii},
#line 216 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str427, ei_mac_thai},
    {-1},
#line 36 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str429, ei_ucs4be},
#line 220 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str430, ei_hp_roman8},
    {-1},
#line 303 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str432, ei_euc_cn},
    {-1},
#line 32 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str434, ei_ucs2le},
#line 89 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str435, ei_iso8859_5},
    {-1}, {-1},
#line 122 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str438, ei_iso8859_9},
    {-1},
#line 115 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str440, ei_iso8859_8},
#line 162 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str441, ei_koi8_u},
    {-1}, {-1},
#line 73 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str444, ei_iso8859_3},
    {-1}, {-1}, {-1},
#line 295 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str448, ei_iso2022_jp},
#line 297 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str449, ei_iso2022_jp1},
#line 298 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str450, ei_iso2022_jp2},
#line 118 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str451, ei_iso8859_8},
    {-1}, {-1},
#line 246 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str454, ei_iso646_jp},
    {-1}, {-1},
#line 287 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str457, ei_euc_jp},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 49 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str465, ei_ucs4internal},
    {-1},
#line 55 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str467, ei_iso8859_1},
#line 65 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str468, ei_iso8859_2},
    {-1},
#line 96 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str470, ei_iso8859_6},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 296 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str479, ei_iso2022_jp},
#line 299 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str480, ei_iso2022_jp2},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 81 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str486, ei_iso8859_4},
    {-1}, {-1},
#line 206 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str489, ei_mac_centraleurope},
    {-1}, {-1},
#line 117 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str492, ei_iso8859_8},
    {-1},
#line 181 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str494, ei_cp1255},
    {-1},
#line 15 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str496, ei_ascii},
    {-1},
#line 315 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str498, ei_euc_tw},
    {-1}, {-1}, {-1}, {-1},
#line 163 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str503, ei_koi8_ru},
    {-1},
#line 237 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str505, ei_cp874},
    {-1}, {-1},
#line 257 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str508, ei_jisx0208},
#line 224 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str509, ei_georgian_ps},
    {-1}, {-1},
#line 105 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str512, ei_iso8859_7},
    {-1},
#line 111 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str514, ei_iso8859_7},
#line 260 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str515, ei_jisx0208},
    {-1}, {-1},
#line 202 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str518, ei_mac_roman},
#line 48 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str519, ei_ucs2swapped},
    {-1}, {-1}, {-1}, {-1},
#line 205 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str524, ei_mac_roman},
    {-1}, {-1},
#line 319 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str527, ei_ces_big5},
#line 267 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str528, ei_jisx0212},
#line 249 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str529, ei_iso646_jp},
#line 193 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str530, ei_cp850},
    {-1}, {-1}, {-1},
#line 318 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str534, ei_ces_big5},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 245 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str541, ei_iso646_jp},
    {-1}, {-1}, {-1},
#line 262 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str545, ei_jisx0212},
    {-1}, {-1}, {-1},
#line 324 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str549, ei_big5hkscs},
    {-1}, {-1}, {-1},
#line 251 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str553, ei_jisx0201},
#line 223 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str554, ei_georgian_academy},
    {-1},
#line 323 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str556, ei_big5hkscs},
    {-1}, {-1}, {-1},
#line 261 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str560, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 212 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str570, ei_mac_greek},
#line 175 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str571, ei_cp1253},
    {-1},
#line 50 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str573, ei_ucs4swapped},
    {-1}, {-1}, {-1}, {-1},
#line 208 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str578, ei_mac_croatian},
    {-1}, {-1}, {-1},
#line 250 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str582, ei_jisx0201},
    {-1}, {-1},
#line 335 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str585, ei_local_wchar_t},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 28 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str594, ei_ucs2be},
    {-1}, {-1}, {-1}, {-1},
#line 264 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str599, ei_jisx0212},
#line 254 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str600, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 215 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str614, ei_mac_arabic},
    {-1}, {-1}, {-1}, {-1},
#line 253 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str619, ei_jisx0201},
#line 255 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str620, ei_jisx0208},
#line 290 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str621, ei_sjis},
    {-1}, {-1}, {-1}, {-1},
#line 211 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str626, ei_mac_ukraine},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 288 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str635, ei_euc_jp},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1},
#line 256 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str646, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 293 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str663, ei_sjis},
#line 263 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str664, ei_jisx0212},
#line 214 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str665, ei_mac_hebrew},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 289 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str675, ei_sjis},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 244 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str685, ei_tcvn},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 178 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str694, ei_cp1254},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 213 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str717, ei_mac_turkish},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 209 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str731, ei_mac_romania},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 184 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str750, ei_cp1256},
    {-1}, {-1},
#line 52 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str753, ei_java},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 292 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str798, ei_sjis},
#line 330 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str799, ei_johab},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 187 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str879, ei_cp1257}
  };

#ifdef __GNUC__
__inline
#endif
const struct alias *
aliases_lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = aliases_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = aliases[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &aliases[key];
            }
        }
    }
  return 0;
}
