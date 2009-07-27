/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUB_WINDOW_H
#define SUB_WINDOW_H

#include <Window.h>

#include <util/OpenHashTable.h>


class SubWindowManager;


class SubWindowKey {
public:
	virtual						~SubWindowKey();

	virtual	bool				Equals(const SubWindowKey* other) const = 0;
	virtual	size_t				HashCode() const = 0;
};


class ObjectSubWindowKey : public SubWindowKey {
public:
								ObjectSubWindowKey(void* object);

	virtual	bool				Equals(const SubWindowKey* other) const;
	virtual	size_t				HashCode() const;

private:
			void*				fObject;
};


class SubWindow : public BWindow {
public:
								SubWindow(SubWindowManager* manager,
									BRect frame, const char* title,
									window_type type, uint32 flags,
									uint32 workspace = B_CURRENT_WORKSPACE);
	virtual						~SubWindow();

	inline	SubWindowKey*		GetSubWindowKey() const;

			bool				AddToSubWindowManager(SubWindowKey* key);
			bool				RemoveFromSubWindowManager();

protected:
			SubWindowManager*	fSubWindowManager;
			SubWindowKey*		fSubWindowKey;

public:
			SubWindow*			fNext;
};


SubWindowKey*
SubWindow::GetSubWindowKey() const
{
	return fSubWindowKey;
}


#endif	// SUB_WINDOW_H
