/* ANSI-C code produced by gperf version 3.0.1 */
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

#define TOTAL_KEYWORDS 338
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 45
#define MIN_HASH_VALUE 13
#define MAX_HASH_VALUE 997
/* maximum key range = 985, duplicates = 0 */

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
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998,   4, 122, 998,  79,   6,
       29,  65,  13,  15,   4,  88,  20,  22, 405, 998,
      998, 998, 998, 998, 998,  47, 188, 110,   6,  26,
       63,  19,  12,   5, 281, 202,   7, 166,  11,   5,
       64, 998,   4,  11,  20, 185, 110, 152,  63,   4,
        4, 998, 998, 998, 998,   5, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998, 998, 998,
      998, 998, 998, 998, 998, 998, 998, 998
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
    char stringpool_str13[sizeof("L6")];
    char stringpool_str15[sizeof("L1")];
    char stringpool_str18[sizeof("HZ")];
    char stringpool_str22[sizeof("L4")];
    char stringpool_str24[sizeof("L5")];
    char stringpool_str26[sizeof("R8")];
    char stringpool_str29[sizeof("L8")];
    char stringpool_str31[sizeof("866")];
    char stringpool_str38[sizeof("L2")];
    char stringpool_str42[sizeof("SJIS")];
    char stringpool_str43[sizeof("ISO-IR-6")];
    char stringpool_str55[sizeof("ISO-IR-166")];
    char stringpool_str57[sizeof("LATIN6")];
    char stringpool_str61[sizeof("LATIN1")];
    char stringpool_str68[sizeof("ISO-IR-14")];
    char stringpool_str74[sizeof("L3")];
    char stringpool_str75[sizeof("LATIN4")];
    char stringpool_str77[sizeof("ISO-IR-165")];
    char stringpool_str79[sizeof("LATIN5")];
    char stringpool_str80[sizeof("ISO-IR-126")];
    char stringpool_str81[sizeof("862")];
    char stringpool_str82[sizeof("ISO-IR-144")];
    char stringpool_str89[sizeof("LATIN8")];
    char stringpool_str91[sizeof("ISO-IR-58")];
    char stringpool_str96[sizeof("ISO-IR-148")];
    char stringpool_str97[sizeof("L7")];
    char stringpool_str98[sizeof("LATIN-9")];
    char stringpool_str100[sizeof("ISO-IR-149")];
    char stringpool_str102[sizeof("ISO-IR-159")];
    char stringpool_str103[sizeof("ISO-IR-226")];
    char stringpool_str107[sizeof("LATIN2")];
    char stringpool_str108[sizeof("ISO8859-6")];
    char stringpool_str109[sizeof("ISO-IR-199")];
    char stringpool_str112[sizeof("ISO8859-1")];
    char stringpool_str113[sizeof("ISO-8859-6")];
    char stringpool_str114[sizeof("ISO_8859-6")];
    char stringpool_str115[sizeof("ISO8859-16")];
    char stringpool_str116[sizeof("PT154")];
    char stringpool_str117[sizeof("ISO-8859-1")];
    char stringpool_str118[sizeof("ISO_8859-1")];
    char stringpool_str119[sizeof("ISO8859-11")];
    char stringpool_str120[sizeof("ISO-8859-16")];
    char stringpool_str121[sizeof("ISO_8859-16")];
    char stringpool_str123[sizeof("CN")];
    char stringpool_str124[sizeof("ISO-8859-11")];
    char stringpool_str125[sizeof("ISO_8859-11")];
    char stringpool_str126[sizeof("ISO8859-4")];
    char stringpool_str128[sizeof("ISO_8859-16:2001")];
    char stringpool_str130[sizeof("ISO8859-5")];
    char stringpool_str131[sizeof("ISO-8859-4")];
    char stringpool_str132[sizeof("ISO_8859-4")];
    char stringpool_str133[sizeof("ISO8859-14")];
    char stringpool_str134[sizeof("ISO-IR-101")];
    char stringpool_str135[sizeof("ISO-8859-5")];
    char stringpool_str136[sizeof("ISO_8859-5")];
    char stringpool_str137[sizeof("ISO8859-15")];
    char stringpool_str138[sizeof("ISO-8859-14")];
    char stringpool_str139[sizeof("ISO_8859-14")];
    char stringpool_str140[sizeof("ISO8859-8")];
    char stringpool_str142[sizeof("ISO-8859-15")];
    char stringpool_str143[sizeof("ISO_8859-15")];
    char stringpool_str144[sizeof("ISO8859-9")];
    char stringpool_str145[sizeof("ISO-8859-8")];
    char stringpool_str146[sizeof("ISO_8859-8")];
    char stringpool_str147[sizeof("CP866")];
    char stringpool_str148[sizeof("ISO-IR-138")];
    char stringpool_str149[sizeof("ISO-8859-9")];
    char stringpool_str150[sizeof("ISO_8859-9")];
    char stringpool_str151[sizeof("ISO_8859-14:1998")];
    char stringpool_str153[sizeof("ISO_8859-15:1998")];
    char stringpool_str155[sizeof("ELOT_928")];
    char stringpool_str156[sizeof("TCVN")];
    char stringpool_str157[sizeof("C99")];
    char stringpool_str158[sizeof("ISO8859-2")];
    char stringpool_str161[sizeof("X0212")];
    char stringpool_str162[sizeof("CP154")];
    char stringpool_str163[sizeof("ISO-8859-2")];
    char stringpool_str164[sizeof("ISO_8859-2")];
    char stringpool_str166[sizeof("ISO-IR-109")];
    char stringpool_str168[sizeof("L10")];
    char stringpool_str169[sizeof("CHAR")];
    char stringpool_str174[sizeof("CP1256")];
    char stringpool_str175[sizeof("ISO-IR-179")];
    char stringpool_str176[sizeof("ISO646-CN")];
    char stringpool_str177[sizeof("ASCII")];
    char stringpool_str178[sizeof("CP1251")];
    char stringpool_str179[sizeof("LATIN3")];
    char stringpool_str181[sizeof("850")];
    char stringpool_str183[sizeof("GB2312")];
    char stringpool_str185[sizeof("CP819")];
    char stringpool_str188[sizeof("X0201")];
    char stringpool_str192[sizeof("CP1254")];
    char stringpool_str194[sizeof("CP949")];
    char stringpool_str196[sizeof("CP1255")];
    char stringpool_str197[sizeof("CP862")];
    char stringpool_str198[sizeof("US")];
    char stringpool_str203[sizeof("CP1361")];
    char stringpool_str206[sizeof("CP1258")];
    char stringpool_str207[sizeof("ISO-IR-110")];
    char stringpool_str209[sizeof("IBM866")];
    char stringpool_str210[sizeof("CP936")];
    char stringpool_str211[sizeof("GEORGIAN-PS")];
    char stringpool_str214[sizeof("LATIN10")];
    char stringpool_str216[sizeof("X0208")];
    char stringpool_str222[sizeof("CHINESE")];
    char stringpool_str224[sizeof("CP1252")];
    char stringpool_str225[sizeof("LATIN7")];
    char stringpool_str226[sizeof("ISO_8859-10:1992")];
    char stringpool_str227[sizeof("ISO-IR-57")];
    char stringpool_str228[sizeof("TIS620")];
    char stringpool_str230[sizeof("ISO8859-3")];
    char stringpool_str231[sizeof("UCS-4")];
    char stringpool_str232[sizeof("ISO-IR-87")];
    char stringpool_str233[sizeof("TIS-620")];
    char stringpool_str234[sizeof("ISO-IR-157")];
    char stringpool_str235[sizeof("ISO-8859-3")];
    char stringpool_str236[sizeof("ISO_8859-3")];
    char stringpool_str237[sizeof("ISO8859-13")];
    char stringpool_str240[sizeof("CSISOLATIN6")];
    char stringpool_str241[sizeof("BIG5")];
    char stringpool_str242[sizeof("ISO-8859-13")];
    char stringpool_str243[sizeof("ISO_8859-13")];
    char stringpool_str244[sizeof("CSISOLATIN1")];
    char stringpool_str245[sizeof("KOI8-R")];
    char stringpool_str246[sizeof("BIG-5")];
    char stringpool_str247[sizeof("IBM819")];
    char stringpool_str248[sizeof("ISO-IR-127")];
    char stringpool_str249[sizeof("CP874")];
    char stringpool_str251[sizeof("ISO646-US")];
    char stringpool_str252[sizeof("VISCII")];
    char stringpool_str253[sizeof("MS-EE")];
    char stringpool_str256[sizeof("MS-ANSI")];
    char stringpool_str258[sizeof("CSISOLATIN4")];
    char stringpool_str259[sizeof("IBM862")];
    char stringpool_str260[sizeof("CP932")];
    char stringpool_str262[sizeof("CSISOLATIN5")];
    char stringpool_str263[sizeof("UCS-2")];
    char stringpool_str265[sizeof("ISO8859-10")];
    char stringpool_str266[sizeof("MS936")];
    char stringpool_str267[sizeof("WCHAR_T")];
    char stringpool_str270[sizeof("ISO-8859-10")];
    char stringpool_str271[sizeof("ISO_8859-10")];
    char stringpool_str272[sizeof("UTF-16")];
    char stringpool_str273[sizeof("EUCCN")];
    char stringpool_str274[sizeof("ROMAN8")];
    char stringpool_str275[sizeof("ISO-IR-203")];
    char stringpool_str276[sizeof("ISO8859-7")];
    char stringpool_str277[sizeof("KOI8-T")];
    char stringpool_str278[sizeof("EUC-CN")];
    char stringpool_str279[sizeof("UCS-4LE")];
    char stringpool_str280[sizeof("ISO-IR-100")];
    char stringpool_str281[sizeof("ISO-8859-7")];
    char stringpool_str282[sizeof("ISO_8859-7")];
    char stringpool_str283[sizeof("MULELAO-1")];
    char stringpool_str284[sizeof("GB_1988-80")];
    char stringpool_str287[sizeof("NEXTSTEP")];
    char stringpool_str289[sizeof("ECMA-114")];
    char stringpool_str290[sizeof("CSISOLATIN2")];
    char stringpool_str291[sizeof("GEORGIAN-ACADEMY")];
    char stringpool_str292[sizeof("PTCP154")];
    char stringpool_str295[sizeof("UCS-2LE")];
    char stringpool_str296[sizeof("CP1253")];
    char stringpool_str297[sizeof("UTF-8")];
    char stringpool_str298[sizeof("HP-ROMAN8")];
    char stringpool_str299[sizeof("ISO_646.IRV:1991")];
    char stringpool_str300[sizeof("CSASCII")];
    char stringpool_str303[sizeof("ECMA-118")];
    char stringpool_str304[sizeof("UCS-4-INTERNAL")];
    char stringpool_str305[sizeof("TCVN5712-1")];
    char stringpool_str307[sizeof("KOREAN")];
    char stringpool_str308[sizeof("CP850")];
    char stringpool_str309[sizeof("MS-CYRL")];
    char stringpool_str310[sizeof("CP950")];
    char stringpool_str313[sizeof("TIS620-0")];
    char stringpool_str319[sizeof("GREEK8")];
    char stringpool_str320[sizeof("UCS-2-INTERNAL")];
    char stringpool_str321[sizeof("TCVN-5712")];
    char stringpool_str323[sizeof("CP1133")];
    char stringpool_str324[sizeof("CP1250")];
    char stringpool_str327[sizeof("ISO-2022-CN")];
    char stringpool_str329[sizeof("UTF-16LE")];
    char stringpool_str335[sizeof("CYRILLIC-ASIAN")];
    char stringpool_str337[sizeof("ISO-10646-UCS-4")];
    char stringpool_str340[sizeof("ISO-2022-CN-EXT")];
    char stringpool_str342[sizeof("CP1257")];
    char stringpool_str345[sizeof("GB_2312-80")];
    char stringpool_str347[sizeof("JP")];
    char stringpool_str351[sizeof("EUCKR")];
    char stringpool_str353[sizeof("ISO-10646-UCS-2")];
    char stringpool_str354[sizeof("GB18030")];
    char stringpool_str356[sizeof("EUC-KR")];
    char stringpool_str357[sizeof("CSKOI8R")];
    char stringpool_str358[sizeof("CSBIG5")];
    char stringpool_str359[sizeof("ANSI_X3.4-1986")];
    char stringpool_str360[sizeof("CP367")];
    char stringpool_str361[sizeof("MACINTOSH")];
    char stringpool_str362[sizeof("CSISOLATIN3")];
    char stringpool_str363[sizeof("CN-BIG5")];
    char stringpool_str366[sizeof("CYRILLIC")];
    char stringpool_str369[sizeof("CSVISCII")];
    char stringpool_str370[sizeof("IBM850")];
    char stringpool_str372[sizeof("MACTHAI")];
    char stringpool_str374[sizeof("UNICODE-1-1")];
    char stringpool_str375[sizeof("ANSI_X3.4-1968")];
    char stringpool_str379[sizeof("TIS620.2529-1")];
    char stringpool_str380[sizeof("US-ASCII")];
    char stringpool_str381[sizeof("UTF-32")];
    char stringpool_str384[sizeof("CN-GB-ISOIR165")];
    char stringpool_str389[sizeof("MAC")];
    char stringpool_str393[sizeof("EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE")];
    char stringpool_str394[sizeof("CSISOLATINARABIC")];
    char stringpool_str395[sizeof("HZ-GB-2312")];
    char stringpool_str397[sizeof("ARMSCII-8")];
    char stringpool_str401[sizeof("CSISOLATINHEBREW")];
    char stringpool_str402[sizeof("VISCII1.1-1")];
    char stringpool_str405[sizeof("ISO-2022-KR")];
    char stringpool_str407[sizeof("WINDOWS-1256")];
    char stringpool_str408[sizeof("UHC")];
    char stringpool_str409[sizeof("WINDOWS-1251")];
    char stringpool_str411[sizeof("MS-HEBR")];
    char stringpool_str412[sizeof("ISO-CELTIC")];
    char stringpool_str413[sizeof("UTF-32LE")];
    char stringpool_str416[sizeof("WINDOWS-1254")];
    char stringpool_str418[sizeof("WINDOWS-1255")];
    char stringpool_str420[sizeof("SHIFT-JIS")];
    char stringpool_str421[sizeof("SHIFT_JIS")];
    char stringpool_str422[sizeof("IBM367")];
    char stringpool_str423[sizeof("WINDOWS-1258")];
    char stringpool_str424[sizeof("CSPTCP154")];
    char stringpool_str426[sizeof("GBK")];
    char stringpool_str428[sizeof("UNICODELITTLE")];
    char stringpool_str432[sizeof("WINDOWS-1252")];
    char stringpool_str433[sizeof("UTF-7")];
    char stringpool_str435[sizeof("KSC_5601")];
    char stringpool_str437[sizeof("ASMO-708")];
    char stringpool_str440[sizeof("CSISO2022CN")];
    char stringpool_str444[sizeof("BIGFIVE")];
    char stringpool_str447[sizeof("WINDOWS-936")];
    char stringpool_str448[sizeof("CSUCS4")];
    char stringpool_str449[sizeof("BIG-FIVE")];
    char stringpool_str453[sizeof("ISO646-JP")];
    char stringpool_str457[sizeof("CSISOLATINGREEK")];
    char stringpool_str458[sizeof("TIS620.2533-1")];
    char stringpool_str459[sizeof("CSISOLATINCYRILLIC")];
    char stringpool_str460[sizeof("UCS-4BE")];
    char stringpool_str462[sizeof("UNICODE-1-1-UTF-7")];
    char stringpool_str465[sizeof("EUCTW")];
    char stringpool_str468[sizeof("WINDOWS-1253")];
    char stringpool_str469[sizeof("CSHPROMAN8")];
    char stringpool_str470[sizeof("EUC-TW")];
    char stringpool_str472[sizeof("KS_C_5601-1989")];
    char stringpool_str476[sizeof("UCS-2BE")];
    char stringpool_str480[sizeof("GREEK")];
    char stringpool_str482[sizeof("WINDOWS-1250")];
    char stringpool_str483[sizeof("CSGB2312")];
    char stringpool_str486[sizeof("WINDOWS-874")];
    char stringpool_str487[sizeof("CSUNICODE11")];
    char stringpool_str489[sizeof("JAVA")];
    char stringpool_str491[sizeof("WINDOWS-1257")];
    char stringpool_str493[sizeof("CSUNICODE")];
    char stringpool_str500[sizeof("CSHALFWIDTHKATAKANA")];
    char stringpool_str502[sizeof("CSISO57GB1988")];
    char stringpool_str504[sizeof("MACICELAND")];
    char stringpool_str509[sizeof("CSIBM866")];
    char stringpool_str510[sizeof("UTF-16BE")];
    char stringpool_str513[sizeof("ARABIC")];
    char stringpool_str514[sizeof("CN-GB")];
    char stringpool_str518[sizeof("CSISO2022KR")];
    char stringpool_str520[sizeof("CSMACINTOSH")];
    char stringpool_str526[sizeof("JIS0208")];
    char stringpool_str528[sizeof("MACROMAN")];
    char stringpool_str531[sizeof("TIS620.2533-0")];
    char stringpool_str538[sizeof("KS_C_5601-1987")];
    char stringpool_str539[sizeof("CSSHIFTJIS")];
    char stringpool_str540[sizeof("HEBREW")];
    char stringpool_str541[sizeof("JIS_X0212")];
    char stringpool_str547[sizeof("MACCROATIAN")];
    char stringpool_str548[sizeof("ISO-2022-JP-1")];
    char stringpool_str549[sizeof("ISO_8859-4:1988")];
    char stringpool_str550[sizeof("EUCJP")];
    char stringpool_str551[sizeof("ISO_8859-5:1988")];
    char stringpool_str555[sizeof("EUC-JP")];
    char stringpool_str556[sizeof("ISO_8859-8:1988")];
    char stringpool_str560[sizeof("ISO_8859-9:1989")];
    char stringpool_str561[sizeof("CSISO58GB231280")];
    char stringpool_str562[sizeof("JIS_C6226-1983")];
    char stringpool_str566[sizeof("IBM-CP1133")];
    char stringpool_str568[sizeof("JIS_X0201")];
    char stringpool_str569[sizeof("MACCENTRALEUROPE")];
    char stringpool_str570[sizeof("CSISO159JISX02121990")];
    char stringpool_str571[sizeof("ISO-2022-JP-2")];
    char stringpool_str573[sizeof("CSUNICODE11UTF7")];
    char stringpool_str574[sizeof("UCS-4-SWAPPED")];
    char stringpool_str578[sizeof("UNICODEBIG")];
    char stringpool_str579[sizeof("CSISO14JISC6220RO")];
    char stringpool_str580[sizeof("JIS_C6220-1969-RO")];
    char stringpool_str586[sizeof("CSPC862LATINHEBREW")];
    char stringpool_str588[sizeof("BIG5HKSCS")];
    char stringpool_str590[sizeof("UCS-2-SWAPPED")];
    char stringpool_str593[sizeof("BIG5-HKSCS")];
    char stringpool_str594[sizeof("UTF-32BE")];
    char stringpool_str596[sizeof("JIS_X0208")];
    char stringpool_str597[sizeof("JISX0201-1976")];
    char stringpool_str601[sizeof("ISO_8859-3:1988")];
    char stringpool_str604[sizeof("ISO-2022-JP")];
    char stringpool_str606[sizeof("JIS_X0212-1990")];
    char stringpool_str607[sizeof("KOI8-U")];
    char stringpool_str608[sizeof("ISO_8859-6:1987")];
    char stringpool_str610[sizeof("ISO_8859-1:1987")];
    char stringpool_str612[sizeof("KOI8-RU")];
    char stringpool_str618[sizeof("MACROMANIA")];
    char stringpool_str633[sizeof("ISO_8859-2:1987")];
    char stringpool_str634[sizeof("CSISO87JISX0208")];
    char stringpool_str648[sizeof("CSEUCKR")];
    char stringpool_str649[sizeof("MACCYRILLIC")];
    char stringpool_str651[sizeof("MS-ARAB")];
    char stringpool_str656[sizeof("JIS_X0208-1983")];
    char stringpool_str657[sizeof("MS-GREEK")];
    char stringpool_str666[sizeof("CSKSC56011987")];
    char stringpool_str669[sizeof("ISO_8859-7:2003")];
    char stringpool_str670[sizeof("JIS_X0208-1990")];
    char stringpool_str683[sizeof("CSISO2022JP2")];
    char stringpool_str692[sizeof("ISO_8859-7:1987")];
    char stringpool_str717[sizeof("CSISO2022JP")];
    char stringpool_str721[sizeof("JOHAB")];
    char stringpool_str726[sizeof("JIS_X0212.1990-0")];
    char stringpool_str730[sizeof("MS_KANJI")];
    char stringpool_str737[sizeof("MACTURKISH")];
    char stringpool_str762[sizeof("CSEUCTW")];
    char stringpool_str763[sizeof("MACGREEK")];
    char stringpool_str774[sizeof("TCVN5712-1:1993")];
    char stringpool_str776[sizeof("WINBALTRIM")];
    char stringpool_str790[sizeof("MS-TURK")];
    char stringpool_str792[sizeof("MACUKRAINE")];
    char stringpool_str796[sizeof("MACARABIC")];
    char stringpool_str802[sizeof("CSPC850MULTILINGUAL")];
    char stringpool_str845[sizeof("MACHEBREW")];
    char stringpool_str997[sizeof("CSEUCPKDFMTJAPANESE")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "L6",
    "L1",
    "HZ",
    "L4",
    "L5",
    "R8",
    "L8",
    "866",
    "L2",
    "SJIS",
    "ISO-IR-6",
    "ISO-IR-166",
    "LATIN6",
    "LATIN1",
    "ISO-IR-14",
    "L3",
    "LATIN4",
    "ISO-IR-165",
    "LATIN5",
    "ISO-IR-126",
    "862",
    "ISO-IR-144",
    "LATIN8",
    "ISO-IR-58",
    "ISO-IR-148",
    "L7",
    "LATIN-9",
    "ISO-IR-149",
    "ISO-IR-159",
    "ISO-IR-226",
    "LATIN2",
    "ISO8859-6",
    "ISO-IR-199",
    "ISO8859-1",
    "ISO-8859-6",
    "ISO_8859-6",
    "ISO8859-16",
    "PT154",
    "ISO-8859-1",
    "ISO_8859-1",
    "ISO8859-11",
    "ISO-8859-16",
    "ISO_8859-16",
    "CN",
    "ISO-8859-11",
    "ISO_8859-11",
    "ISO8859-4",
    "ISO_8859-16:2001",
    "ISO8859-5",
    "ISO-8859-4",
    "ISO_8859-4",
    "ISO8859-14",
    "ISO-IR-101",
    "ISO-8859-5",
    "ISO_8859-5",
    "ISO8859-15",
    "ISO-8859-14",
    "ISO_8859-14",
    "ISO8859-8",
    "ISO-8859-15",
    "ISO_8859-15",
    "ISO8859-9",
    "ISO-8859-8",
    "ISO_8859-8",
    "CP866",
    "ISO-IR-138",
    "ISO-8859-9",
    "ISO_8859-9",
    "ISO_8859-14:1998",
    "ISO_8859-15:1998",
    "ELOT_928",
    "TCVN",
    "C99",
    "ISO8859-2",
    "X0212",
    "CP154",
    "ISO-8859-2",
    "ISO_8859-2",
    "ISO-IR-109",
    "L10",
    "CHAR",
    "CP1256",
    "ISO-IR-179",
    "ISO646-CN",
    "ASCII",
    "CP1251",
    "LATIN3",
    "850",
    "GB2312",
    "CP819",
    "X0201",
    "CP1254",
    "CP949",
    "CP1255",
    "CP862",
    "US",
    "CP1361",
    "CP1258",
    "ISO-IR-110",
    "IBM866",
    "CP936",
    "GEORGIAN-PS",
    "LATIN10",
    "X0208",
    "CHINESE",
    "CP1252",
    "LATIN7",
    "ISO_8859-10:1992",
    "ISO-IR-57",
    "TIS620",
    "ISO8859-3",
    "UCS-4",
    "ISO-IR-87",
    "TIS-620",
    "ISO-IR-157",
    "ISO-8859-3",
    "ISO_8859-3",
    "ISO8859-13",
    "CSISOLATIN6",
    "BIG5",
    "ISO-8859-13",
    "ISO_8859-13",
    "CSISOLATIN1",
    "KOI8-R",
    "BIG-5",
    "IBM819",
    "ISO-IR-127",
    "CP874",
    "ISO646-US",
    "VISCII",
    "MS-EE",
    "MS-ANSI",
    "CSISOLATIN4",
    "IBM862",
    "CP932",
    "CSISOLATIN5",
    "UCS-2",
    "ISO8859-10",
    "MS936",
    "WCHAR_T",
    "ISO-8859-10",
    "ISO_8859-10",
    "UTF-16",
    "EUCCN",
    "ROMAN8",
    "ISO-IR-203",
    "ISO8859-7",
    "KOI8-T",
    "EUC-CN",
    "UCS-4LE",
    "ISO-IR-100",
    "ISO-8859-7",
    "ISO_8859-7",
    "MULELAO-1",
    "GB_1988-80",
    "NEXTSTEP",
    "ECMA-114",
    "CSISOLATIN2",
    "GEORGIAN-ACADEMY",
    "PTCP154",
    "UCS-2LE",
    "CP1253",
    "UTF-8",
    "HP-ROMAN8",
    "ISO_646.IRV:1991",
    "CSASCII",
    "ECMA-118",
    "UCS-4-INTERNAL",
    "TCVN5712-1",
    "KOREAN",
    "CP850",
    "MS-CYRL",
    "CP950",
    "TIS620-0",
    "GREEK8",
    "UCS-2-INTERNAL",
    "TCVN-5712",
    "CP1133",
    "CP1250",
    "ISO-2022-CN",
    "UTF-16LE",
    "CYRILLIC-ASIAN",
    "ISO-10646-UCS-4",
    "ISO-2022-CN-EXT",
    "CP1257",
    "GB_2312-80",
    "JP",
    "EUCKR",
    "ISO-10646-UCS-2",
    "GB18030",
    "EUC-KR",
    "CSKOI8R",
    "CSBIG5",
    "ANSI_X3.4-1986",
    "CP367",
    "MACINTOSH",
    "CSISOLATIN3",
    "CN-BIG5",
    "CYRILLIC",
    "CSVISCII",
    "IBM850",
    "MACTHAI",
    "UNICODE-1-1",
    "ANSI_X3.4-1968",
    "TIS620.2529-1",
    "US-ASCII",
    "UTF-32",
    "CN-GB-ISOIR165",
    "MAC",
    "EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE",
    "CSISOLATINARABIC",
    "HZ-GB-2312",
    "ARMSCII-8",
    "CSISOLATINHEBREW",
    "VISCII1.1-1",
    "ISO-2022-KR",
    "WINDOWS-1256",
    "UHC",
    "WINDOWS-1251",
    "MS-HEBR",
    "ISO-CELTIC",
    "UTF-32LE",
    "WINDOWS-1254",
    "WINDOWS-1255",
    "SHIFT-JIS",
    "SHIFT_JIS",
    "IBM367",
    "WINDOWS-1258",
    "CSPTCP154",
    "GBK",
    "UNICODELITTLE",
    "WINDOWS-1252",
    "UTF-7",
    "KSC_5601",
    "ASMO-708",
    "CSISO2022CN",
    "BIGFIVE",
    "WINDOWS-936",
    "CSUCS4",
    "BIG-FIVE",
    "ISO646-JP",
    "CSISOLATINGREEK",
    "TIS620.2533-1",
    "CSISOLATINCYRILLIC",
    "UCS-4BE",
    "UNICODE-1-1-UTF-7",
    "EUCTW",
    "WINDOWS-1253",
    "CSHPROMAN8",
    "EUC-TW",
    "KS_C_5601-1989",
    "UCS-2BE",
    "GREEK",
    "WINDOWS-1250",
    "CSGB2312",
    "WINDOWS-874",
    "CSUNICODE11",
    "JAVA",
    "WINDOWS-1257",
    "CSUNICODE",
    "CSHALFWIDTHKATAKANA",
    "CSISO57GB1988",
    "MACICELAND",
    "CSIBM866",
    "UTF-16BE",
    "ARABIC",
    "CN-GB",
    "CSISO2022KR",
    "CSMACINTOSH",
    "JIS0208",
    "MACROMAN",
    "TIS620.2533-0",
    "KS_C_5601-1987",
    "CSSHIFTJIS",
    "HEBREW",
    "JIS_X0212",
    "MACCROATIAN",
    "ISO-2022-JP-1",
    "ISO_8859-4:1988",
    "EUCJP",
    "ISO_8859-5:1988",
    "EUC-JP",
    "ISO_8859-8:1988",
    "ISO_8859-9:1989",
    "CSISO58GB231280",
    "JIS_C6226-1983",
    "IBM-CP1133",
    "JIS_X0201",
    "MACCENTRALEUROPE",
    "CSISO159JISX02121990",
    "ISO-2022-JP-2",
    "CSUNICODE11UTF7",
    "UCS-4-SWAPPED",
    "UNICODEBIG",
    "CSISO14JISC6220RO",
    "JIS_C6220-1969-RO",
    "CSPC862LATINHEBREW",
    "BIG5HKSCS",
    "UCS-2-SWAPPED",
    "BIG5-HKSCS",
    "UTF-32BE",
    "JIS_X0208",
    "JISX0201-1976",
    "ISO_8859-3:1988",
    "ISO-2022-JP",
    "JIS_X0212-1990",
    "KOI8-U",
    "ISO_8859-6:1987",
    "ISO_8859-1:1987",
    "KOI8-RU",
    "MACROMANIA",
    "ISO_8859-2:1987",
    "CSISO87JISX0208",
    "CSEUCKR",
    "MACCYRILLIC",
    "MS-ARAB",
    "JIS_X0208-1983",
    "MS-GREEK",
    "CSKSC56011987",
    "ISO_8859-7:2003",
    "JIS_X0208-1990",
    "CSISO2022JP2",
    "ISO_8859-7:1987",
    "CSISO2022JP",
    "JOHAB",
    "JIS_X0212.1990-0",
    "MS_KANJI",
    "MACTURKISH",
    "CSEUCTW",
    "MACGREEK",
    "TCVN5712-1:1993",
    "WINBALTRIM",
    "MS-TURK",
    "MACUKRAINE",
    "MACARABIC",
    "CSPC850MULTILINGUAL",
    "MACHEBREW",
    "CSEUCPKDFMTJAPANESE"
  };
#define stringpool ((const char *) &stringpool_contents)

static const struct alias aliases[] =
  {
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 134 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str13, ei_iso8859_10},
    {-1},
#line 60 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str15, ei_iso8859_1},
    {-1}, {-1},
#line 325 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str18, ei_hz},
    {-1}, {-1}, {-1},
#line 84 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str22, ei_iso8859_4},
    {-1},
#line 126 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str24, ei_iso8859_9},
    {-1},
#line 226 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str26, ei_hp_roman8},
    {-1}, {-1},
#line 151 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str29, ei_iso8859_14},
    {-1},
#line 207 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str31, ei_cp866},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 68 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str38, ei_iso8859_2},
    {-1}, {-1}, {-1},
#line 303 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str42, ei_sjis},
#line 16 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str43, ei_ascii},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1},
#line 247 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str55, ei_tis620},
    {-1},
#line 133 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str57, ei_iso8859_10},
    {-1}, {-1}, {-1},
#line 59 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str61, ei_iso8859_1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 259 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str68, ei_iso646_jp},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 76 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str74, ei_iso8859_3},
#line 83 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str75, ei_iso8859_4},
    {-1},
#line 289 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str77, ei_isoir165},
    {-1},
#line 125 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str79, ei_iso8859_9},
#line 107 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str80, ei_iso8859_7},
#line 203 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str81, ei_cp862},
#line 90 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str82, ei_iso8859_5},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 150 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str89, ei_iso8859_14},
    {-1},
#line 286 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str91, ei_gb2312},
    {-1}, {-1}, {-1}, {-1},
#line 124 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str96, ei_iso8859_9},
#line 144 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str97, ei_iso8859_13},
#line 158 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str98, ei_iso8859_15},
    {-1},
#line 294 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str100, ei_ksc5601},
    {-1},
#line 278 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str102, ei_jisx0212},
#line 163 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str103, ei_iso8859_16},
    {-1}, {-1}, {-1},
#line 67 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str107, ei_iso8859_2},
#line 102 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str108, ei_iso8859_6},
#line 149 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str109, ei_iso8859_14},
    {-1}, {-1},
#line 62 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str112, ei_iso8859_1},
#line 94 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str113, ei_iso8859_6},
#line 95 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str114, ei_iso8859_6},
#line 166 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str115, ei_iso8859_16},
#line 233 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str116, ei_pt154},
#line 53 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str117, ei_iso8859_1},
#line 54 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str118, ei_iso8859_1},
#line 139 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str119, ei_iso8859_11},
#line 160 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str120, ei_iso8859_16},
#line 161 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str121, ei_iso8859_16},
    {-1},
#line 283 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str123, ei_iso646_cn},
#line 137 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str124, ei_iso8859_11},
#line 138 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str125, ei_iso8859_11},
#line 86 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str126, ei_iso8859_4},
    {-1},
#line 162 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str128, ei_iso8859_16},
    {-1},
#line 93 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str130, ei_iso8859_5},
#line 79 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str131, ei_iso8859_4},
#line 80 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str132, ei_iso8859_4},
#line 153 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str133, ei_iso8859_14},
#line 66 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str134, ei_iso8859_2},
#line 87 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str135, ei_iso8859_5},
#line 88 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str136, ei_iso8859_5},
#line 159 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str137, ei_iso8859_15},
#line 146 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str138, ei_iso8859_14},
#line 147 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str139, ei_iso8859_14},
#line 120 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str140, ei_iso8859_8},
    {-1},
#line 154 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str142, ei_iso8859_15},
#line 155 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str143, ei_iso8859_15},
#line 128 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str144, ei_iso8859_9},
#line 114 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str145, ei_iso8859_8},
#line 115 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str146, ei_iso8859_8},
#line 205 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str147, ei_cp866},
#line 117 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str148, ei_iso8859_8},
#line 121 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str149, ei_iso8859_9},
#line 122 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str150, ei_iso8859_9},
#line 148 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str151, ei_iso8859_14},
    {-1},
#line 156 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str153, ei_iso8859_15},
    {-1},
#line 109 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str155, ei_iso8859_7},
#line 253 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str156, ei_tcvn},
#line 51 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str157, ei_c99},
#line 70 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str158, ei_iso8859_2},
    {-1}, {-1},
#line 277 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str161, ei_jisx0212},
#line 235 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str162, ei_pt154},
#line 63 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str163, ei_iso8859_2},
#line 64 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str164, ei_iso8859_2},
    {-1},
#line 74 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str166, ei_iso8859_3},
    {-1},
#line 165 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str168, ei_iso8859_16},
#line 348 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str169, ei_local_char},
    {-1}, {-1}, {-1}, {-1},
#line 189 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str174, ei_cp1256},
#line 142 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str175, ei_iso8859_13},
#line 281 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str176, ei_iso646_cn},
#line 13 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str177, ei_ascii},
#line 174 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str178, ei_cp1251},
#line 75 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str179, ei_iso8859_3},
    {-1},
#line 199 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str181, ei_cp850},
    {-1},
#line 314 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str183, ei_euc_cn},
    {-1},
#line 57 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str185, ei_iso8859_1},
    {-1}, {-1},
#line 264 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str188, ei_jisx0201},
    {-1}, {-1}, {-1},
#line 183 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str192, ei_cp1254},
    {-1},
#line 342 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str194, ei_cp949},
    {-1},
#line 186 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str196, ei_cp1255},
#line 201 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str197, ei_cp862},
#line 21 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str198, ei_ascii},
    {-1}, {-1}, {-1}, {-1},
#line 345 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str203, ei_johab},
    {-1}, {-1},
#line 195 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str206, ei_cp1258},
#line 82 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str207, ei_iso8859_4},
    {-1},
#line 206 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str209, ei_cp866},
#line 318 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str210, ei_ces_gbk},
#line 231 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str211, ei_georgian_ps},
    {-1}, {-1},
#line 164 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str214, ei_iso8859_16},
    {-1},
#line 270 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str216, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 288 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str222, ei_gb2312},
    {-1},
#line 177 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str224, ei_cp1252},
#line 143 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str225, ei_iso8859_13},
#line 131 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str226, ei_iso8859_10},
#line 282 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str227, ei_iso646_cn},
#line 242 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str228, ei_tis620},
    {-1},
#line 78 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str230, ei_iso8859_3},
#line 33 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str231, ei_ucs4},
#line 271 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str232, ei_jisx0208},
#line 241 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str233, ei_tis620},
#line 132 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str234, ei_iso8859_10},
#line 71 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str235, ei_iso8859_3},
#line 72 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str236, ei_iso8859_3},
#line 145 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str237, ei_iso8859_13},
    {-1}, {-1},
#line 135 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str240, ei_iso8859_10},
#line 330 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str241, ei_ces_big5},
#line 140 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str242, ei_iso8859_13},
#line 141 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str243, ei_iso8859_13},
#line 61 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str244, ei_iso8859_1},
#line 167 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str245, ei_koi8_r},
#line 331 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str246, ei_ces_big5},
#line 58 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str247, ei_iso8859_1},
#line 97 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str248, ei_iso8859_6},
#line 248 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str249, ei_cp874},
    {-1},
#line 14 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str251, ei_ascii},
#line 250 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str252, ei_viscii},
#line 173 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str253, ei_cp1250},
    {-1}, {-1},
#line 179 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str256, ei_cp1252},
    {-1},
#line 85 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str258, ei_iso8859_4},
#line 202 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str259, ei_cp862},
#line 306 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str260, ei_cp932},
    {-1},
#line 127 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str262, ei_iso8859_9},
#line 24 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str263, ei_ucs2},
    {-1},
#line 136 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str265, ei_iso8859_10},
#line 319 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str266, ei_ces_gbk},
#line 349 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str267, ei_local_wchar_t},
    {-1}, {-1},
#line 129 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str270, ei_iso8859_10},
#line 130 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str271, ei_iso8859_10},
#line 38 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str272, ei_utf16},
#line 313 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str273, ei_euc_cn},
#line 225 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str274, ei_hp_roman8},
#line 157 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str275, ei_iso8859_15},
#line 113 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str276, ei_iso8859_7},
#line 232 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str277, ei_koi8_t},
#line 312 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str278, ei_euc_cn},
#line 37 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str279, ei_ucs4le},
#line 56 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str280, ei_iso8859_1},
#line 103 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str281, ei_iso8859_7},
#line 104 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str282, ei_iso8859_7},
#line 238 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str283, ei_mulelao},
#line 280 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str284, ei_iso646_cn},
    {-1}, {-1},
#line 228 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str287, ei_nextstep},
    {-1},
#line 98 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str289, ei_iso8859_6},
#line 69 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str290, ei_iso8859_2},
#line 230 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str291, ei_georgian_academy},
#line 234 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str292, ei_pt154},
    {-1}, {-1},
#line 31 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str295, ei_ucs2le},
#line 180 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str296, ei_cp1253},
#line 23 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str297, ei_utf8},
#line 224 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str298, ei_hp_roman8},
#line 15 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str299, ei_ascii},
#line 22 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str300, ei_ascii},
    {-1}, {-1},
#line 108 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str303, ei_iso8859_7},
#line 49 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str304, ei_ucs4internal},
#line 255 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str305, ei_tcvn},
    {-1},
#line 296 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str307, ei_ksc5601},
#line 197 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str308, ei_cp850},
#line 176 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str309, ei_cp1251},
#line 336 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str310, ei_cp950},
    {-1}, {-1},
#line 243 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str313, ei_tis620},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 110 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str319, ei_iso8859_7},
#line 47 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str320, ei_ucs2internal},
#line 254 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str321, ei_tcvn},
    {-1},
#line 239 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str323, ei_cp1133},
#line 171 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str324, ei_cp1250},
    {-1}, {-1},
#line 322 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str327, ei_iso2022_cn},
    {-1},
#line 40 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str329, ei_utf16le},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 236 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str335, ei_pt154},
    {-1},
#line 34 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str337, ei_ucs4},
    {-1}, {-1},
#line 324 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str340, ei_iso2022_cn_ext},
    {-1},
#line 192 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str342, ei_cp1257},
    {-1}, {-1},
#line 285 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str345, ei_gb2312},
    {-1},
#line 260 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str347, ei_iso646_jp},
    {-1}, {-1}, {-1},
#line 340 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str351, ei_euc_kr},
    {-1},
#line 25 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str353, ei_ucs2},
#line 321 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str354, ei_gb18030},
    {-1},
#line 339 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str356, ei_euc_kr},
#line 168 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str357, ei_koi8_r},
#line 335 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str358, ei_ces_big5},
#line 18 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str359, ei_ascii},
#line 19 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str360, ei_ascii},
#line 210 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str361, ei_mac_roman},
#line 77 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str362, ei_iso8859_3},
#line 334 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str363, ei_ces_big5},
    {-1}, {-1},
#line 91 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str366, ei_iso8859_5},
    {-1}, {-1},
#line 252 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str369, ei_viscii},
#line 198 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str370, ei_cp850},
    {-1},
#line 223 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str372, ei_mac_thai},
    {-1},
#line 29 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str374, ei_ucs2be},
#line 17 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str375, ei_ascii},
    {-1}, {-1}, {-1},
#line 244 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str379, ei_tis620},
#line 12 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str380, ei_ascii},
#line 41 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str381, ei_utf32},
    {-1}, {-1},
#line 290 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str384, ei_isoir165},
    {-1}, {-1}, {-1}, {-1},
#line 211 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str389, ei_mac_roman},
    {-1}, {-1}, {-1},
#line 299 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str393, ei_euc_jp},
#line 101 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str394, ei_iso8859_6},
#line 326 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str395, ei_hz},
    {-1},
#line 229 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str397, ei_armscii_8},
    {-1}, {-1}, {-1},
#line 119 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str401, ei_iso8859_8},
#line 251 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str402, ei_viscii},
    {-1}, {-1},
#line 346 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str405, ei_iso2022_kr},
    {-1},
#line 190 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str407, ei_cp1256},
#line 343 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str408, ei_cp949},
#line 175 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str409, ei_cp1251},
    {-1},
#line 188 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str411, ei_cp1255},
#line 152 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str412, ei_iso8859_14},
#line 43 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str413, ei_utf32le},
    {-1}, {-1},
#line 184 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str416, ei_cp1254},
    {-1},
#line 187 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str418, ei_cp1255},
    {-1},
#line 302 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str420, ei_sjis},
#line 301 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str421, ei_sjis},
#line 20 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str422, ei_ascii},
#line 196 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str423, ei_cp1258},
#line 237 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str424, ei_pt154},
    {-1},
#line 317 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str426, ei_ces_gbk},
    {-1},
#line 32 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str428, ei_ucs2le},
    {-1}, {-1}, {-1},
#line 178 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str432, ei_cp1252},
#line 44 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str433, ei_utf7},
    {-1},
#line 291 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str435, ei_ksc5601},
    {-1},
#line 99 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str437, ei_iso8859_6},
    {-1}, {-1},
#line 323 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str440, ei_iso2022_cn},
    {-1}, {-1}, {-1},
#line 333 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str444, ei_ces_big5},
    {-1}, {-1},
#line 320 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str447, ei_ces_gbk},
#line 35 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str448, ei_ucs4},
#line 332 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str449, ei_ces_big5},
    {-1}, {-1}, {-1},
#line 258 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str453, ei_iso646_jp},
    {-1}, {-1}, {-1},
#line 112 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str457, ei_iso8859_7},
#line 246 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str458, ei_tis620},
#line 92 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str459, ei_iso8859_5},
#line 36 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str460, ei_ucs4be},
    {-1},
#line 45 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str462, ei_utf7},
    {-1}, {-1},
#line 328 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str465, ei_euc_tw},
    {-1}, {-1},
#line 181 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str468, ei_cp1253},
#line 227 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str469, ei_hp_roman8},
#line 327 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str470, ei_euc_tw},
    {-1},
#line 293 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str472, ei_ksc5601},
    {-1}, {-1}, {-1},
#line 27 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str476, ei_ucs2be},
    {-1}, {-1}, {-1},
#line 111 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str480, ei_iso8859_7},
    {-1},
#line 172 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str482, ei_cp1250},
#line 316 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str483, ei_euc_cn},
    {-1}, {-1},
#line 249 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str486, ei_cp874},
#line 30 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str487, ei_ucs2be},
    {-1},
#line 52 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str489, ei_java},
    {-1},
#line 193 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str491, ei_cp1257},
    {-1},
#line 26 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str493, ei_ucs2},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 265 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str500, ei_jisx0201},
    {-1},
#line 284 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str502, ei_iso646_cn},
    {-1},
#line 214 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str504, ei_mac_iceland},
    {-1}, {-1}, {-1}, {-1},
#line 208 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str509, ei_cp866},
#line 39 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str510, ei_utf16be},
    {-1}, {-1},
#line 100 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str513, ei_iso8859_6},
#line 315 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str514, ei_euc_cn},
    {-1}, {-1}, {-1},
#line 347 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str518, ei_iso2022_kr},
    {-1},
#line 212 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str520, ei_mac_roman},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 269 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str526, ei_jisx0208},
    {-1},
#line 209 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str528, ei_mac_roman},
    {-1}, {-1},
#line 245 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str531, ei_tis620},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 292 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str538, ei_ksc5601},
#line 305 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str539, ei_sjis},
#line 118 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str540, ei_iso8859_8},
#line 274 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str541, ei_jisx0212},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 215 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str547, ei_mac_croatian},
#line 309 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str548, ei_iso2022_jp1},
#line 81 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str549, ei_iso8859_4},
#line 298 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str550, ei_euc_jp},
#line 89 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str551, ei_iso8859_5},
    {-1}, {-1}, {-1},
#line 297 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str555, ei_euc_jp},
#line 116 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str556, ei_iso8859_8},
    {-1}, {-1}, {-1},
#line 123 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str560, ei_iso8859_9},
#line 287 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str561, ei_gb2312},
#line 272 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str562, ei_jisx0208},
    {-1}, {-1}, {-1},
#line 240 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str566, ei_cp1133},
    {-1},
#line 262 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str568, ei_jisx0201},
#line 213 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str569, ei_mac_centraleurope},
#line 279 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str570, ei_jisx0212},
#line 310 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str571, ei_iso2022_jp2},
    {-1},
#line 46 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str573, ei_utf7},
#line 50 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str574, ei_ucs4swapped},
    {-1}, {-1}, {-1},
#line 28 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str578, ei_ucs2be},
#line 261 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str579, ei_iso646_jp},
#line 257 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str580, ei_iso646_jp},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 204 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str586, ei_cp862},
    {-1},
#line 338 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str588, ei_big5hkscs},
    {-1},
#line 48 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str590, ei_ucs2swapped},
    {-1}, {-1},
#line 337 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str593, ei_big5hkscs},
#line 42 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str594, ei_utf32be},
    {-1},
#line 266 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str596, ei_jisx0208},
#line 263 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str597, ei_jisx0201},
    {-1}, {-1}, {-1},
#line 73 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str601, ei_iso8859_3},
    {-1}, {-1},
#line 307 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str604, ei_iso2022_jp},
    {-1},
#line 276 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str606, ei_jisx0212},
#line 169 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str607, ei_koi8_u},
#line 96 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str608, ei_iso8859_6},
    {-1},
#line 55 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str610, ei_iso8859_1},
    {-1},
#line 170 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str612, ei_koi8_ru},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 216 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str618, ei_mac_romania},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 65 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str633, ei_iso8859_2},
#line 273 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str634, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 341 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str648, ei_euc_kr},
#line 217 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str649, ei_mac_cyrillic},
    {-1},
#line 191 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str651, ei_cp1256},
    {-1}, {-1}, {-1}, {-1},
#line 267 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str656, ei_jisx0208},
#line 182 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str657, ei_cp1253},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 295 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str666, ei_ksc5601},
    {-1}, {-1},
#line 106 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str669, ei_iso8859_7},
#line 268 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str670, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1},
#line 311 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str683, ei_iso2022_jp2},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 105 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str692, ei_iso8859_7},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 308 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str717, ei_iso2022_jp},
    {-1}, {-1}, {-1},
#line 344 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str721, ei_johab},
    {-1}, {-1}, {-1}, {-1},
#line 275 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str726, ei_jisx0212},
    {-1}, {-1}, {-1},
#line 304 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str730, ei_sjis},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 220 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str737, ei_mac_turkish},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 329 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str762, ei_euc_tw},
#line 219 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str763, ei_mac_greek},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1},
#line 256 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str774, ei_tcvn},
    {-1},
#line 194 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str776, ei_cp1257},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 185 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str790, ei_cp1254},
    {-1},
#line 218 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str792, ei_mac_ukraine},
    {-1}, {-1}, {-1},
#line 222 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str796, ei_mac_arabic},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 200 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str802, ei_cp850},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 221 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str845, ei_mac_hebrew},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 300 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str997, ei_euc_jp}
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
