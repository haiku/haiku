/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Weirauch, dev@m-phasis.de
 */
#ifndef DESKBAR_REPLICANT_H
#define DESKBAR_REPLICANT_H


#include <View.h>


extern const char* kDeskbarItemName;


class DeskbarReplicant : public BView {
	public:
		DeskbarReplicant(BRect frame, int32 resizingMode);
		DeskbarReplicant(BMessage* archive);
		virtual ~DeskbarReplicant();

		static	DeskbarReplicant* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	AttachedToWindow();

		virtual	void	Draw(BRect updateRect);

		virtual	void	MessageReceived(BMessage* message);
		virtual	void	MouseDown(BPoint where);

	private:
		void			_Init();
		void			_OpenBluetoothPreferences();
		
		void			_ShowBluetoothServerConsole();
		void			_QuitBluetoothServer();
		
		void			_ShowErrorAlert(BString msg, status_t status);

		BBitmap*		fIcon;
};

#endif	// DESKBAR_REPLICANT_H
