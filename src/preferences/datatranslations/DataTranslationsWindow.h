/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */
#ifndef DATA_TRANSLATIONS_WINDOW_H
#define DATA_TRANSLATIONS_WINDOW_H


//#include "DataTranslationsSettings.h"
#include "IconView.h"

#include <Window.h>

class BBox;
class BView;

class IconView;
class TranslatorListView;


class DataTranslationsWindow : public BWindow {
	public:
		DataTranslationsWindow();
		~DataTranslationsWindow();

		virtual	bool QuitRequested();
		virtual void MessageReceived(BMessage* message);

	private:
		status_t _GetTranslatorInfo(int32 id, const char *&name, const char *&info,
			int32 &version, BPath &path);
		void _ShowInfoAlert(int32 id);
		status_t _ShowConfigView(int32 id);
		status_t _PopulateListView();
		void _SetupViews();

		TranslatorListView *fTranslatorListView;

		BBox *fRightBox;
			// Box hosting fConfigView, fIconView,
			// fTranNameView and the Info button

		BView *fConfigView;
			// the translator config view

		IconView *fIconView;
		BStringView *fTranslatorNameView;
};

#endif	// DATA_TRANSLATIONS_WINDOW_H

