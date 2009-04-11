#ifndef	___LibMonkeysAudio_H_
#define	___LibMonkeysAudio_H_
//---------------------------------------------------------------------------
// BeOS Header
// C++ Header
// Project Header
#include "APETag.h"
//===========================================================================
extern "C"
{
	_EXPORT CAPETag*	create_capetag_1(CIO* oIO, BOOL oAnalyze = TRUE);
	_EXPORT CAPETag*	create_capetag_2(const char* oFilename, BOOL oAnalyze = TRUE);
	_EXPORT void		destroy_capetag(CAPETag* oAPETag);
	_EXPORT const char*	lib_monkeys_audio_components();
	_EXPORT const char*	lib_monkeys_audio_copyright();
	_EXPORT const char*	lib_monkeys_audio_name();
	_EXPORT const char*	lib_monkeys_audio_version();
}
//===========================================================================
#endif	// ___LibMonkeysAudio_H_
