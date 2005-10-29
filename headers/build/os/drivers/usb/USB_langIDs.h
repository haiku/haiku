#ifndef USB_LANGIDS_H
#define USB_LANGIDS_H

// USB Language Identifiers, 3/29/2000, version 1.0
// Reference: http://www.usb.org/developers/docs/USB_LANGIDs.pdf

#define USB_LANGIDS_VERSION		0x0100	// version 1.0

// descriptor string index 0 = supported LANGIDs array
#define USB_LANGIDS_STRING_INDEX	(0)

#define USB_PRIMARY_LANGID_MASK		(0x003F)		// 9 to 0 bits 
#define USB_SUB_LANGID_MASK			(0xFC00)		// 15 to 10 bits			

#define USB_PRIMARY_LANGID(id)		(id & USB_PRIMARY_LANGID_MASK)
#define USB_SUB_LANGID(id)			((id & USB_SUB_LANGID_MASK) >> 10)

enum {
	USB_LANGID_AFRIKAANS				= 0x0436,	// Afrikaans
	USB_LANGID_ALBANIAN					= 0x041c,	// Albanian
	USB_LANGID_ARABIC_SAUDI_ARABIA		= 0x0401,	// Arabic (Saudi Arabia)
	USB_LANGID_ARABIC_IRAK				= 0x0801,	// Arabic (Irak)
	USB_LANGID_ARABIC_EGYPT				= 0x0c01,	// Arabic (Egypt)
	USB_LANGID_ARABIC_LYBYA				= 0x1001,	// Arabic (Libya)
	USB_LANGID_ARABIC_ALGERIA			= 0x1401,	// Arabic (Algeria)
	USB_LANGID_ARABIC_MOROCCO			= 0x1801,	// Arabic (Morocco)
	USB_LANGID_ARABIC_TUNISIA			= 0x1c01,	// Arabic (Tunisia)
	USB_LANGID_ARABIC_OMAN				= 0x2001,	// Arabic (Oman)
	USB_LANGID_ARABIC_YEMEN				= 0x2401,	// Arabic (Yemen)
	USB_LANGID_ARABIC_SYRIA				= 0x2801,	// Arabic (Syria)
	USB_LANGID_ARABIC_JORDAN			= 0x2c01,	// Arabic (Jordan)
	USB_LANGID_ARABIC_LEBANON			= 0x3001,	// Arabic (Lebanon)
	USB_LANGID_ARABIC_KUWAIT			= 0x3401,	// Arabic (Kuwait)
	USB_LANGID_ARABIC_UAE				= 0x3801,	// Arabic (U.A.E.)
	USB_LANGID_ARABIC_BAHRAIN			= 0x3c01,	// Arabic (Bahrain)
	USB_LANGID_ARABIC_QATAR				= 0x4001,	// Arabic (Qatar)
	USB_LANGID_ARMENIAN					= 0x042b,	// Armenian
	USB_LANGID_ASSAMESE					= 0x044d,	// Assamese
	USB_LANGID_AZERI_LATIN				= 0x042c,	// Azeri (Latin)
	USB_LANGID_AZERI_CYRILLIC			= 0x082c,	// Azeri (Cyrillic)
	USB_LANGID_BASQUE					= 0x042d,	// Basque
	USB_LANGID_BELARUSSIAN				= 0x0423,	// Belarussian
	USB_LANGID_BENGALI					= 0x0445,	// Bengali
	USB_LANGID_BULGARIAN				= 0x0402,	// Bulgarian
	USB_LANGID_BURMESE					= 0x0455,	// Burmese
	USB_LANGID_CATALAN					= 0x0403,	// Catalan
	USB_LANGID_CHINESE_TAIWAN			= 0x0404,	// Chinese (Taiwan)
	USB_LANGID_CHINESE_PRC				= 0x0804,	// Chinese (PRC = People Republic of Chinese)
	USB_LANGID_CHINESE_HONG_KONG		= 0x0c04,	// Chinese (Hong Kong)
	USB_LANGID_CHINESE_SINGAPORE		= 0x1004,	// Chinese (Singapore)
	USB_LANGID_CHINESE_MACAU_SAR		= 0x1404,	// Chinese (Macau SAR)
	USB_LANGID_CROATIAN					= 0x041a,	// Croatian
	USB_LANGID_CZECH					= 0x0405,	// Czech
	USB_LANGID_DANISH					= 0x0406,	// Danish
	USB_LANGID_DUTCH_NETHERLANDS		= 0x0413,	// Dutch (Netherlands)
	USB_LANGID_DUTCH_BELGIUM			= 0x0813,	// Dutch (Belgium)
	USB_LANGID_ENGLISH_UNITED_STATES	= 0x0409,	// English (United States)	
	USB_LANGID_ENGLISH_UNITED_KINGDOM	= 0x0809,	// English (United Kingdom)	
	USB_LANGID_ENGLISH_AUSTRALIAN		= 0x0c09,	// English (Australian)	
	USB_LANGID_ENGLISH_CANADIAN			= 0x1009,	// English (Canadian)	
	USB_LANGID_ENGLISH_NEW_ZEALAND		= 0x1409,	// English (New Zealand)	
	USB_LANGID_ENGLISH_IRELAND			= 0x1809,	// English (Ireland)	
	USB_LANGID_ENGLISH_SOUTH_AFRICA		= 0x1c09,	// English (South Africa)	
	USB_LANGID_ENGLISH_JAMAICA			= 0x2009,	// English (Jamaica)	
	USB_LANGID_ENGLISH_CARIBBEAN		= 0x2409,	// English (Caribbean)	
	USB_LANGID_ENGLISH_BELIZE			= 0x2809,	// English (Belize)	
	USB_LANGID_ENGLISH_TRINIDAD			= 0x2c09,	// English (Trinidad)	
	USB_LANGID_ENGLISH_ZIMBABWE			= 0x3009,	// English (Zimbabwe)	
	USB_LANGID_ENGLISH_PHLIPPINES		= 0x3409,	// English (Philippines)	
	USB_LANGID_ESTONIAN					= 0x0425,	// Estonian
	USB_LANGID_FAEROESE					= 0x0438,	// Faeroese
	USB_LANGID_FARSI					= 0x0429,	// Farsi
	USB_LANGID_FINNISH					= 0x040b,	// Finnish
	USB_LANGID_FRENCH_STANDARD			= 0x040c,	// French (Standard)
	USB_LANGID_FRENCH_BELGIAN			= 0x080c,	// French (Belgian)
	USB_LANGID_FRENCH_CANADIAN			= 0x0c0c,	// French (Canadian)
	USB_LANGID_FRENCH_SWITZERLAND		= 0x100c,	// French (Switzerland)
	USB_LANGID_FRENCH_LUXEMBOURG		= 0x140c,	// French (Luxembourg)
	USB_LANGID_FRENCH_MONACO			= 0x180c,	// French (Monaco)
	USB_LANGID_GEORGIAN					= 0x0437,	// Georgian
	USB_LANGID_GERMAN_STANDARD			= 0x0407,	// German (Standard)
	USB_LANGID_GERMAN_SWITZERLAND		= 0x0807,	// German (Switzerland)
	USB_LANGID_GERMAN_AUSTRIA			= 0x0c07,	// German (Austria)
	USB_LANGID_GERMAN_LUXEMBOURG		= 0x1007,	// German (Luxembourg)
	USB_LANGID_GERMAN_LIECHTENSTEIN		= 0x1407,	// German (Liechtenstein)
	USB_LANGID_GREEK					= 0x0408,	// Greek
	USB_LANGID_GUJARATI					= 0x0447,	// Gujarati
	USB_LANGID_HEBREW					= 0x040d,	// Hebrew
	USB_LANGID_HINDI					= 0x0439,	// Hindi
	USB_LANGID_HUNGARIAN				= 0x040e,	// Hungarian
	USB_LANGID_ICELANDIC				= 0x040f,	// Icelandic
	USB_LANGID_INDONESIAN				= 0x0421,	// Indonesian
	USB_LANGID_ITALIAN_STANDARD			= 0x0410,	// Italian (Standard)
	USB_LANGID_ITALIAN_SWITZERLAND		= 0x0810,	// Italian (Switzerland)
	USB_LANGID_JAPANESE					= 0x0411,	// Japanese
	USB_LANGID_KANNADA					= 0x044b,	// Kannada
	USB_LANGID_KASHMIRI_INDIA			= 0x0860,	// Kashmiri (India)
	USB_LANGID_KAZAKH					= 0x043f,	// Kazakh
	USB_LANGID_KONKANI					= 0x0457,	// Konkani
	USB_LANGID_KOREAN_JOHAB				= 0x0412,	// Korean (Johab)
	USB_LANGID_LATVIAN					= 0x0426,	// Latvian
	USB_LANGID_LITHUANIAN				= 0x0427,	// Lithuanian
	USB_LANGID_LITHUANIAN_CLASSIC		= 0x0827,	// Lithuanian (Classic)
	USB_LANGID_MACEDONIAN				= 0x042f,	// Macedonian
	USB_LANGID_MALAY_MALAYSIAN			= 0x043e,	// Malay (Malaysian)
	USB_LANGID_MALAY_BRUNEI_DARUSSALAM	= 0x083e,	// Malay (Brunei Darussalam)
	USB_LANGID_MALAYALAM				= 0x044c,	// Malayalam
	USB_LANGID_MANIPURI					= 0x0458,	// Manipuri
	USB_LANGID_MARATHI					= 0x044e,	// Marathi
	USB_LANGID_NEPALI_INDIA				= 0x0861,	// Nepali (India)
	USB_LANGID_NORWEGIAN_BOKMAL			= 0x0414,	// Norwegian (Bokmal)
	USB_LANGID_NORWEGIAN_NYNORSK		= 0x0814,	// Norwegian (Nynorsk)
	USB_LANGID_ORIYA					= 0x0448,	// Oriya
	USB_LANGID_POLISH					= 0x0415,	// Polish
	USB_LANGID_PORTUGUESE_BRAZIL		= 0x0416,	// Portuguese (Brazil)
	USB_LANGID_PORTUGUESE_STANDARD		= 0x0816,	// Portuguese (Standard)
	USB_LANGID_PUNJABI					= 0x0446,	// Punjabi
	USB_LANGID_ROMANIAN					= 0x0418,	// Romanian
	USB_LANGID_RUSSIAN					= 0x0419,	// Russian
	USB_LANGID_SANSKRIT					= 0x044f,	// Sanskrit
	USB_LANGID_SERBIAN_CYRILLIC			= 0x0c1a,	// Serbian (Cyrillic)
	USB_LANGID_SERBIAN_LATIN			= 0x081a,	// Serbian (Latin)
	USB_LANGID_SINDHI					= 0x0459,	// Sindhi
	USB_LANGID_SLOVAK					= 0x041b,	// Slovak
	USB_LANGID_SLOVENIAN				= 0x0424,	// Slovenian
	USB_LANGID_SPANNISH_TRADITIONAL_SORT= 0x040a,	// Spannish (Traditional Sort)
	USB_LANGID_SPANNISH_MEXICAN			= 0x080a,	// Spannish (Mexican)
	USB_LANGID_SPANNISH_MODERN_SORT		= 0x0c0a,	// Spannish (Modern Sort)
	USB_LANGID_SPANNISH_GUATEMALA		= 0x100a,	// Spannish (Guatemala)
	USB_LANGID_SPANNISH_COSTA_RICA		= 0x140a,	// Spannish (Costa Rica)
	USB_LANGID_SPANNISH_PANAMA			= 0x180a,	// Spannish (Panama)
	USB_LANGID_SPANNISH_DOMINICAN_REPUBLIC	= 0x1c0a,	// Spannish (Dominican Republic)
	USB_LANGID_SPANNISH_VENEZUELA		= 0x200a,	// Spannish (Venezuela)
	USB_LANGID_SPANNISH_COLOMBIA		= 0x240a,	// Spannish (Colombia)
	USB_LANGID_SPANNISH_PERU			= 0x280a,	// Spannish (Peru)
	USB_LANGID_SPANNISH_ARGENTINA		= 0x2c0a,	// Spannish (Argentina)
	USB_LANGID_SPANNISH_ECUADOR			= 0x300a,	// Spannish (Ecuador)
	USB_LANGID_SPANNISH_CHILE			= 0x340a,	// Spannish (Chile)
	USB_LANGID_SPANNISH_URUGUAY			= 0x380a,	// Spannish (Uruguay)
	USB_LANGID_SPANNISH_PARAGUAY		= 0x3c0a,	// Spannish (Paraguay)
	USB_LANGID_SPANNISH_BOLIVIA			= 0x400a,	// Spannish (Bolivia)
	USB_LANGID_SPANNISH_EL_SALVADOR		= 0x440a,	// Spannish (El Salvador)
	USB_LANGID_SPANNISH_HONDURAS		= 0x480a,	// Spannish (Honduras)
	USB_LANGID_SPANNISH_NICARAGUA		= 0x4c0a,	// Spannish (Nicaragua)
	USB_LANGID_SPANNISH_PUERTO_RICO		= 0x500a,	// Spannish (Puerto Rico)
	USB_LANGID_SUTU						= 0x0430,	// Sutu
	USB_LANGID_SWAHILI_KENYA			= 0x0441,	// Swahili (Kenya)
	USB_LANGID_SWEDISH					= 0x041d,	// Swedish
	USB_LANGID_SWEDISH_FINLAND			= 0x081d,	// Swedish (Finland)
	USB_LANGID_TAMIL					= 0x0449,	// Tamil
	USB_LANGID_TATAR_TATARSTAN			= 0x0444,	// Tatar (Tatarstan)
	USB_LANGID_TELUGU					= 0x044a,	// Telugu
	USB_LANGID_THAI						= 0x041e,	// Thai
	USB_LANGID_TURKISH					= 0x041f,	// Turkish
	USB_LANGID_UKRAINIAN				= 0x0422,	// Ukrainian
	USB_LANGID_URDU_PAKISTAN			= 0x0420,	// Urdu (Pakistan)
	USB_LANGID_URDU_INDIA				= 0x0820,	// Urdu (India)
	USB_LANGID_UZBEK_LATIN				= 0x0443,	// Uzbek (Latin)
	USB_LANGID_UZBEK_CYRILLIC			= 0x0843,	// Uzbek (Cyrillic)
	USB_LANGID_VIETNAMESE				= 0x042a,	// Vietnamese

	USB_LANGID_HID_UDD					= 0x04ff,	// HID (Usage Data Descriptor)
	USB_LANGID_HID1						= 0xf0ff,	// HID (Vendor Defined 1)
	USB_LANGID_HID2						= 0xf4ff,	// HID (Vendor Defined 2)
	USB_LANGID_HID3						= 0xf8ff,	// HID (Vendor Defined 3)
	USB_LANGID_HID4						= 0xfcff,	// HID (Vendor Defined 4)

	USB_LANGID_INVALID					= 0x0000	// Invalid LANG ID
};

#endif	// USB_LANGIDS_H


