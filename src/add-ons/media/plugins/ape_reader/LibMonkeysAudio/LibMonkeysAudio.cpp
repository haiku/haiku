#include <string>
#include <InterfaceDefs.h>

/*============================================================================*/
const char*	gAppName = "Lib Monkey's Audio";
const char*	gAppVer = "Ver 1.65";
const char*	gCright = "Copyright "B_UTF8_COPYRIGHT" 2003-2008 by SHINTA";
const char*	gOriginal = "MAC library Copyright "B_UTF8_COPYRIGHT" by Matthew T. Ashland";
const char*	gAppSignature = "application/x-vnd.SHINTA-LibMonkeysAudio";
/*============================================================================*/

/*=== Memo =====================================================================
==============================================================================*/

//------------------------------------------------------------------------------
#include "LibMonkeysAudio.h"
//------------------------------------------------------------------------------
// BeOS
// C++
// Add2
//===========================================================================
CAPETag*	create_capetag_1(CIO* oIO, BOOL oAnalyze)
{
	return new CAPETag(oIO, oAnalyze);
}
//------------------------------------------------------------------------------
CAPETag*	create_capetag_2(const char* oFilename, BOOL oAnalyze)
{
	return new CAPETag(oFilename, oAnalyze);
}
//------------------------------------------------------------------------------
void	destroy_capetag(CAPETag* oAPETag)
{
	delete oAPETag;
}
//------------------------------------------------------------------------------
const char*	lib_monkeys_audio_components()
{
	return gOriginal;
}
//------------------------------------------------------------------------------
const char*	lib_monkeys_audio_copyright()
{
	static std::string	saCright;

	saCright = std::string(gCright)+"\n"+gOriginal;
	return saCright.c_str();
}
//------------------------------------------------------------------------------
const char*	lib_monkeys_audio_name()
{
	return gAppName;
}
//------------------------------------------------------------------------------
const char*	lib_monkeys_audio_version()
{
	return gAppVer;
}
//===========================================================================
