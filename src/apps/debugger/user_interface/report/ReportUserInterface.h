/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef REPORT_USER_INTERFACE_H
#define REPORT_USER_INTERFACE_H


#include <ObjectList.h>
#include <String.h>

#include "UserInterface.h"
#include "Team.h"


class ReportUserInterface : public UserInterface,
	private Team::Listener {
public:
								ReportUserInterface(thread_id targetThread,
									const char* reportPath);
	virtual						~ReportUserInterface();

	virtual	const char*			ID() const;

	virtual	status_t			Init(Team* team,
									UserInterfaceListener* listener);
	virtual	void				Show();
	virtual	void				Terminate();

	virtual	bool				IsInteractive() const;

	virtual status_t			LoadSettings(const TeamUiSettings* settings);
	virtual status_t			SaveSettings(TeamUiSettings*& settings)	const;

	virtual	void				NotifyUser(const char* title,
									const char* message,
									user_notification_type type);
	virtual	void				NotifyBackgroundWorkStatus(
									const char* message);
	virtual	int32				SynchronouslyAskUser(const char* title,
									const char* message, const char* choice1,
									const char* choice2, const char* choice3);
	virtual	status_t			SynchronouslyAskUserForFile(entry_ref* _ref);

			void				Run();

	// Team::Listener
	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadStateChanged(
									const Team::ThreadEvent& event);
	virtual void				DebugReportChanged(
									const Team::DebugReportEvent& event);

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			thread_id			fTargetThread;
			const char*			fReportPath;
			sem_id				fShowSemaphore;
			sem_id				fReportSemaphore;
			bool				fShown;
	volatile bool				fTerminating;
};


#endif	// REPORT_USER_INTERFACE_H
