/*	MediaFiles.h	*/
/*	Copyright 1998 Be Incorporated. All rights reserved.	*/

#if !defined(_MEDIA_FILES_H)
#define _MEDIA_FILES_H

#include <MediaDefs.h>
#include <List.h>

struct entry_ref;

#include <String.h>

class BMediaFiles {
public:

		BMediaFiles();
virtual		~BMediaFiles();

virtual		status_t RewindTypes();
virtual		status_t GetNextType(
				BString * out_type);

virtual		status_t RewindRefs(
				const char * type);
virtual		status_t GetNextRef(
				BString * out_type,
				entry_ref * out_ref = NULL);

virtual		status_t GetRefFor(
				const char * type,
				const char * item,
				entry_ref * out_ref);
virtual		status_t SetRefFor(
				const char * type,
				const char * item,
				const entry_ref & ref);
virtual		status_t RemoveRefFor(		//	This might better be called "ClearRefFor"
				const char * type,		//	but it's too late now...
				const char * item,
				const entry_ref & ref);

static const char B_SOUNDS[];	/* for "types" */

virtual		status_t RemoveItem(		//	new in 4.1, removes the whole item.
				const char * type,
				const char * item);


private:

			status_t _Reserved_MediaFiles_0(void *, ...);
virtual		status_t _Reserved_MediaFiles_1(void *, ...);
virtual		status_t _Reserved_MediaFiles_2(void *, ...);
virtual		status_t _Reserved_MediaFiles_3(void *, ...);
virtual		status_t _Reserved_MediaFiles_4(void *, ...);
virtual		status_t _Reserved_MediaFiles_5(void *, ...);
virtual		status_t _Reserved_MediaFiles_6(void *, ...);
virtual		status_t _Reserved_MediaFiles_7(void *, ...);

		BList m_types;
		int m_type_index;
		BString m_cur_type;
		BList m_items;
		int m_item_index;
};

#endif /* _MEDIA_FILES_H */

