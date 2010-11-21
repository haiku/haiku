/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_ROSTER_H
#define TERMINAL_ROSTER_H


#include <Clipboard.h>
#include <Handler.h>
#include <Locker.h>

#include <ObjectList.h>


/*!	Provides access to the (virtual) global terminal roster.

	Register() registers our terminal with the roster and establishes the link
	to the roster. From then on CountTerminals() and TerminalAt() provide
	access to the terminal list and allows to post our window status via
	SetWindowInfo(). The list is automatically kept up to date. Unregister()
	will remove our terminal from the roster and terminate the connection.

	A single Listener can be set. It is notified whenever the terminal list
	has changed.
*/
class TerminalRoster : private BHandler {
public:
			class Listener;

public:
			struct Info {
				int32			id;
				team_id			team;
				uint32			workspaces;
				bool			minimized;

			public:
								Info(int32 id, team_id team);
								Info(const BMessage& archive);

				status_t		Archive(BMessage& archive) const;

				bool			operator==(const Info& other) const;
				bool			operator!=(const Info& other) const
									{ return !(*this == other); }
			};

public:
								TerminalRoster();

			bool				Lock();
			void				Unlock();

			status_t			Register(team_id teamID, BLooper* looper);
			void				Unregister();
			bool				IsRegistered() const
									{ return fOurInfo != NULL; }

			int32				ID() const;

			void				SetWindowInfo(bool minimized,
									uint32 workspaces);

			// roster must be locked
			int32				CountTerminals() const
									{ return fInfos.CountItems(); }
			const Info*			TerminalAt(int32 index) const
									{ return fInfos.ItemAt(index); }

			void				SetListener(Listener* listener)
									{ fListener = listener; }

private:
	// BHandler
	virtual	void				MessageReceived(BMessage* message);

private:
			typedef BObjectList<Info> InfoList;

private:
			status_t			_UpdateInfos(bool checkApps);
			status_t			_UpdateClipboard();

			void				_NotifyListener();

	static	int					_CompareInfos(const Info* a, const Info* b);
	static	int					_CompareIDInfo(const int32* id,
									const Info* info);

			bool				_TeamIsRunning(team_id teamID);

private:
			BLocker				fLock;
			BClipboard			fClipboard;
			InfoList			fInfos;
			Info*				fOurInfo;
			bigtime_t			fLastCheckedTime;
			Listener*			fListener;
			bool				fInfosUpdated;
};


class TerminalRoster::Listener {
public:
	virtual						~Listener();

	virtual	void				TerminalInfosUpdated(TerminalRoster* roster)
									= 0;
};


#endif	// TERMINAL_ROSTER_H
