/********************************************************************************
/
/      File:           TranslatorAddOn.h
/
/      Description:    This header file defines the interface that should be 
/                      implemented and exported by a Tanslation Kit add-on.
/                      You will only need to include this header when building 
/                      an actual add-on, not when just using the kit.
/
/      Copyright 1998, Be Incorporated, All Rights Reserved.
/      Copyright 1995-1997, Jon Watte
/
********************************************************************************/

#ifndef _TRANSLATOR_ADD_ON_H
#define _TRANSLATOR_ADD_ON_H

#include <TranslationDefs.h>


class BView;
class BRect;
class BPositionIO;


/*	This is the 4.0 and 4.5 compatible API. For later versions, use the		*/
/*	BTranslator-based API (make_nth_translator)								*/

/*	These variables and functions should be exported by a translator add-on	*/

extern "C" {

_EXPORT	extern	char translatorName[];		/*	required, C string, ex "Jon's Sound"	*/
_EXPORT	extern	char translatorInfo[];		/*	required, descriptive C string, ex "Jon's Sound Translator (shareware $5: user@mail.net) v1.00"	*/
_EXPORT	extern	int32 translatorVersion;		/*	required, integer, ex 100	*/

_EXPORT	extern	translation_format inputFormats[];		/*	optional (else Identify is always called)	*/
_EXPORT	extern	translation_format outputFormats[];	/*	optional (else Translate is called anyway)	*/
												/*	Translators that don't export outputFormats 	*/
												/*	will not be considered by files looking for 	*/
												/*	specific output formats.	*/

	/*	Return B_NO_TRANSLATOR if not handling this data.	*/
	/*	Even if inputFormats exists, may be called for data without hints	*/
	/*	Ff outType is not 0, must be able to export in wanted format	*/

_EXPORT	extern	status_t Identify(	/*	required	*/
				BPositionIO * inSource,
				const translation_format * inFormat,	/*	can beNULL	*/
				BMessage * ioExtension,	/*	can be NULL	*/
				translator_info * outInfo,
				uint32 outType);

	/*	Return B_NO_TRANSLATOR if not handling the output format	*/
	/*	If outputFormats exists, will only be called for those formats	*/

_EXPORT	extern	status_t Translate(	/*	required	*/
				BPositionIO * inSource,
				const translator_info * inInfo,
				BMessage * ioExtension,	/*	can be NULL	*/
				uint32 outType,
				BPositionIO * outDestination);

	/*	The view will get resized to what the parent thinks is 	*/
	/*	reasonable. However, it will still receive MouseDowns etc.	*/
	/*	Your view should change settings in the translator immediately, 	*/
	/*	taking care not to change parameters for a translation that is 	*/
	/*	currently running. Typically, you'll have a global struct for 	*/
	/*	settings that is atomically copied into the translator function 	*/
	/*	as a local when translation starts.	*/
	/*	Store your settings wherever you feel like it.	*/

_EXPORT	extern	status_t MakeConfig(	/*	optional	*/
				BMessage * ioExtension,	/*	can be NULL	*/
				BView * * outView,
				BRect * outExtent);

	/*	Copy your current settings to a BMessage that may be passed 	*/
	/*	to BTranslators::Translate at some later time when the user wants to 	*/
	/*	use whatever settings you're using right now.	*/

_EXPORT	extern	status_t GetConfigMessage(	/*	optional	*/
				BMessage * ioExtension);


}



#endif /* _TRANSLATOR_ADD_ON_H	*/


