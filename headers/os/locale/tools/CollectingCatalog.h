/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TOOLS_COLLECTING_CATALOG_H_
#define _TOOLS_COLLECTING_CATALOG_H_


// Translation macros used when executing collectcatkeys
#undef B_TRANSLATE
#define B_TRANSLATE(string) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT)

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT(string, context) \
	B_CATKEY((string), (context))

#undef B_TRANSLATE_COMMENT
#define B_TRANSLATE_COMMENT(string, comment) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT, (comment))

#undef B_TRANSLATE_ALL
#define B_TRANSLATE_ALL(string, context, comment) \
	B_CATKEY((string), (context), (comment))

#undef B_TRANSLATE_ID
#define B_TRANSLATE_ID(id) \
	B_CATKEY((id))

#undef B_TRANSLATE_SYSTEM_NAME
#define B_TRANSLATE_SYSTEM_NAME(string) \
	B_CATKEY((string), B_TRANSLATION_SYSTEM_NAME_CONTEXT)

#undef B_TRANSLATE_MARK
#define B_TRANSLATE_MARK(string) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT)

#undef B_TRANSLATE_MARK_COMMENT
#define B_TRANSLATE_MARK_COMMENT(string, comment) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT, (comment))

#undef B_TRANSLATE_MARK_ALL
#define B_TRANSLATE_MARK_ALL(string, context, comment) \
	B_CATKEY((string), (context), (comment))

#undef B_TRANSLATE_MARK_ID
#define B_TRANSLATE_MARK_ID(id) \
	B_CATKEY((id))

#undef B_TRANSLATE_MARK_SYSTEM_NAME
#define B_TRANSLATE_MARK_SYSTEM_NAME(string) \
	B_CATKEY((string), B_TRANSLATION_SYSTEM_NAME_CONTEXT, "")

#undef B_TRANSLATE_MARK_VOID
#define B_TRANSLATE_MARK_VOID(string) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT)

#undef B_TRANSLATE_MARK_COMMENT_VOID
#define B_TRANSLATE_MARK_COMMENT_VOID(string, comment) \
	B_CATKEY((string), B_TRANSLATION_CONTEXT, (comment))

#undef B_TRANSLATE_MARK_ALL_VOID
#define B_TRANSLATE_MARK_ALL_VOID(string, context, comment) \
	B_CATKEY((string), (context), (comment))

#undef B_TRANSLATE_MARK_ID_VOID
#define B_TRANSLATE_MARK_ID_VOID(id) \
	B_CATKEY((id))

#undef B_TRANSLATE_MARK_SYSTEM_NAME_VOID
#define B_TRANSLATE_MARK_SYSTEM_NAME_VOID(string) \
	B_CATKEY((string), B_TRANSLATION_SYSTEM_NAME_CONTEXT, "")

#undef B_TRANSLATE_NOCOLLECT
#define B_TRANSLATE_NOCOLLECT(string)

#undef B_TRANSLATE_NOCOLLECT_COMMENT
#define B_TRANSLATE_NOCOLLECT_COMMENT(string, comment)

#undef B_TRANSLATE_NOCOLLECT_ALL
#define B_TRANSLATE_NOCOLLECT_ALL(string, context, comment)

#undef B_TRANSLATE_NOCOLLECT_ID
#define B_TRANSLATE_NOCOLLECT_ID(id)

#undef B_TRANSLATE_NOCOLLECT_SYSTEM_NAME
#define B_TRANSLATE_NOCOLLECT_SYSTEM_NAME(string)


#endif /* _COLLECTING_CATALOG_H_ */
