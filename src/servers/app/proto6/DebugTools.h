#ifndef _DEBUGTOOLS_H_
#define _DEBUGTOOLS_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <String.h>

void PrintColorSpaceToStream(color_space value);
void TranslateMessageCodeToStream(int32 code);
void PrintMessageCode(int32 code);
BString TranslateStatusToString(status_t value);

#endif