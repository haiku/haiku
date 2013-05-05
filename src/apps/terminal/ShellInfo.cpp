/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ShellInfo.h"

#include <CharacterSet.h>
#include <CharacterSetRoster.h>

#include "TermConst.h"

using namespace BPrivate ; // BCharacterSet stuff


ShellInfo::ShellInfo()
	:
	fProcessID(-1),
	fIsDefaultShell(true),
	fEncoding(M_UTF8),
	fEncodingName("UTF-8")
{
}


void
ShellInfo::SetEncoding(int encoding)
{
	fEncoding = encoding;

	const BCharacterSet* charset
		= BCharacterSetRoster::GetCharacterSetByConversionID(fEncoding);
	fEncodingName = charset ? charset->GetName() : "UTF-8";
}
