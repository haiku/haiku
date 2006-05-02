/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */
#ifndef DATA_TRANSLATIONS_H
#define DATA_TRANSLATIONS_H


#include "DataTranslationsWindow.h"
#include "DataTranslationsSettings.h"

#include <Application.h>


class DataTranslationsApplication : public BApplication {
	public:
		DataTranslationsApplication();
		virtual ~DataTranslationsApplication();
	
		virtual void RefsReceived(BMessage *message);
		virtual void AboutRequested(void);

		BPoint WindowCorner() const {return fSettings->WindowCorner(); }
		void SetWindowCorner(BPoint corner);

	private:
		void _InstallError(const char* name, status_t status); 
		status_t _Install(BDirectory& target, BEntry& entry);
		void _NoTranslatorError(const char* name); 

		DataTranslationsSettings* fSettings;
};

#endif	// DATA_TRANSLATIONS_H
