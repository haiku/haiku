// Blocker.h

#ifndef BLOCKER_H
#define BLOCKER_H

#include <OS.h>

class Blocker {
public:
								Blocker();
								Blocker(sem_id semaphore);
								Blocker(const Blocker& other);
								~Blocker();

			status_t			InitCheck() const;

			status_t			PrepareForUse();

			status_t			Block(int32* userData = NULL);
			status_t			Unblock(int32 userData = 0);

			Blocker&			operator=(const Blocker& other);
			bool				operator==(const Blocker& other) const;
			bool				operator!=(const Blocker& other) const;

private:
			void				_Unset();

private:
			struct Data;

			Data*				fData;
};

#endif	// BLOCKER_H
