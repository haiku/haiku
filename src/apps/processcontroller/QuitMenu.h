/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _QUIT_MENU_H_
#define _QUIT_MENU_H_


#include "Utilities.h"

#include <Menu.h>
#include <Messenger.h>


class QuitMenu : public BMenu {
	public:
		QuitMenu(const char* title, info_pack* infos, int infosCount);
		virtual	void	AttachedToWindow();
		virtual void	DetachedFromWindow();
		virtual void	MessageReceived(BMessage *msg);
		void			AddTeam(team_id tmid);

	private:
		const info_pack* fInfos;
		int				fInfosCount;
		BMessenger*		fMe;
};

#endif // _QUIT_MENU_H_
