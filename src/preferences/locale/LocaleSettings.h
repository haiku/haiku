/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#ifndef __LOCALE_SETTINGS_H__
#define __LOCALE_SETTINGS_H__


#include <File.h>
#include <Message.h>


class LocaleSettings {
	public:
					LocaleSettings();

		status_t	Load();
		status_t	Save();

		bool 		operator==(const LocaleSettings& other);

		void		UpdateFrom(BMessage* message);

		/*
		void		SetPreferredLanguages();
		void		GetPreferredLanguages();
		void		SetCountry();
		void		GetCountry();

		void		SetDateFormat();
		void		GetDateFormat();
		void		SetTimeFormat();
		void		GetTimeFormat();
		void		SetNumberFormat();
		void		GetNumberFormat();
		void		SetMoneyFormat();
		void		GetMoneyFormat();
		*/

	private:
		status_t	_Open(BFile* file, int32 mode);

		BMessage	fMessage;
};


#endif

