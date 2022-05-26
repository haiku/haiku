Debug Utility functions
#######################

These functions are used to make print-based debugging easier.

- BString TranslateStatusToBString(status_t value)
- BString TranslateColorSpaceToBString(color_space value)
- BString TranslateMessageCodeToBString(int32 value)
- BString TranslateStatusToBString(status_t value)
- BString TranslateColorSpaceToBString(color_space value)
- BString TranslateMessageCodeToBString(int32 value)
- const char \* TranslateStatusToString(status_t value)
- const char \* TranslateColorSpaceToString(color_space value)
- const char \* TranslateMessageCodeToString(int32 value)

All of these functions are essentially big switch() statements which
assign an appropriate string for the passed parameter and return the
assigned string. This way the string can be printed or otherwise
easily used.

