// LazyInitializable.h

#ifndef USERLAND_FS_LAZY_INITIALIZABLE_H
#define USERLAND_FS_LAZY_INITIALIZABLE_H

#include <OS.h>

namespace UserlandFSUtil {

class LazyInitializable {
public:
								LazyInitializable();
								LazyInitializable(bool init);
	virtual						~LazyInitializable();

			status_t			Access();
			status_t			InitCheck() const;

protected:
	virtual	status_t			FirstTimeInit() = 0;

protected:
			status_t			fInitStatus;
			sem_id				fInitSemaphore;
};

} // namespace UserlandFSUtil

using UserlandFSUtil::LazyInitializable;

#endif	// USERLAND_FS_LAZY_INITIALIZABLE_H
