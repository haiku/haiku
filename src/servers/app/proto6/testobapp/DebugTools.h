#ifndef _DEBUGTOOLS_H_
#define _DEBUGTOOLS_H_

#include <SupportDefs.h>
#include <GraphicsDefs.h>

void PrintStatusToStream(status_t value);
void PrintColorSpaceToStream(color_space value);
void TranslateMessageCodeToStream(int32 code);
void PrintMessageCode(int32 code);

#endif