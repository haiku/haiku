#ifndef HEY_H
#define HEY_H

#include <SupportDefs.h>



__declspec(dllexport) int32 HeyInterpreterThreadHook(void* arg);

__declspec(dllexport) status_t Hey(BMessenger* target, const char* arg, BMessage* reply);
__declspec(dllexport) bool isSpace(char c);
__declspec(dllexport) status_t Hey(BMessenger* target, char* argv[], int32* argx, int32 argc, BMessage* reply);
__declspec(dllexport) status_t add_specifier(BMessage *to_message, char *argv[], int32 *argx, int32 argc);
__declspec(dllexport) status_t add_data(BMessage *to_message, char *argv[], int32 *argx);
__declspec(dllexport) status_t add_with(BMessage *to_message, char *argv[], int32 *argx, int32 argc);
__declspec(dllexport) void add_message_contents(BList *textlist, BMessage *msg, int32 level);
__declspec(dllexport) char *get_datatype_string(int32 type);
__declspec(dllexport) char *format_data(int32 type, char *ptr, long size);
__declspec(dllexport) void print_message(BMessage *message);
__declspec(dllexport) char *id_to_string(long ID, char *here);
__declspec(dllexport) bool is_valid_char(uint8 c);


#endif
