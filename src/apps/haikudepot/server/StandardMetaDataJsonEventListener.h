/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef STANDARD_META_DATA_JSON_EVENT_LISTENER_H
#define STANDARD_META_DATA_JSON_EVENT_LISTENER_H

#include "StandardMetaData.h"

#include <JsonEventListener.h>
#include <String.h>
#include <StringList.h>


class SmdStackedEventListener;


class StandardMetaDataJsonEventListener : public BJsonEventListener {
friend class SmdStackedEventListener;
public:
								StandardMetaDataJsonEventListener(
									const BString& jsonPath,
									StandardMetaData& metaData);
		virtual					~StandardMetaDataJsonEventListener();

			bool				Handle(const BJsonEvent& event);
			void				HandleError(status_t status, int32 line,
								const char* message);
			void				Complete();
			status_t			ErrorStatus();

protected:
			void				SetStackedListener(SmdStackedEventListener *listener);

private:
			void				SetJsonPath(const BString& jsonPath);
			StandardMetaData*	MetaData();

			BStringList			fJsonPathObjectNames;
			StandardMetaData*	fMetaData;
			status_t			fErrorStatus;
			SmdStackedEventListener*
								fStackedListener;
 };


 #endif // STANDARD_META_DATA_JSON_EVENT_LISTENER_H