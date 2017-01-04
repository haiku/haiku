/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef REPO_ROW_H
#define REPO_ROW_H


#include <ColumnListView.h>
#include <String.h>


enum {
	kEnabledColumn,
	kNameColumn,
	kUrlColumn
};


class RepoRow : public BRow {
public:
								RepoRow(const char* repo_name,
									const char* repo_url, bool enabled);

			const char*			Name() const { return fName.String(); }
			void				SetName(const char* name);
			const char*			Url() const { return fUrl.String(); }
			void				SetEnabled(bool enabled);
			void				RefreshEnabledField();
			bool				IsEnabled() { return fEnabled; }
			void				SetTaskState(uint32 state);
			uint32				TaskState() { return fTaskState; }
			void				SetHasSiblings(bool hasSiblings)
									{ fHasSiblings = hasSiblings; }
			bool				HasSiblings() { return fHasSiblings; }

private:
			BString				fName;
			BString				fUrl;
			bool				fEnabled;
			uint32				fTaskState;
			bool				fHasSiblings;
};


#endif
