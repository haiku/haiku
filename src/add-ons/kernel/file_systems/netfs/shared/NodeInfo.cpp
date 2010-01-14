// NodeInfo.cpp

#include "NodeInfo.h"

// ShowAround
void
NodeInfo::ShowAround(RequestMemberVisitor* visitor)
{
	// stat members
	visitor->Visit(this, st.st_dev);
	visitor->Visit(this, st.st_ino);
	visitor->Visit(this, *(int32*)&st.st_mode);
	visitor->Visit(this, *(int32*)&st.st_nlink);
	visitor->Visit(this, *(int32*)&st.st_uid);
	visitor->Visit(this, *(int32*)&st.st_gid);
	visitor->Visit(this, st.st_size);
	visitor->Visit(this, st.st_rdev);
	visitor->Visit(this, *(int32*)&st.st_blksize);
	visitor->Visit(this, st.st_atime);
	visitor->Visit(this, st.st_mtime);
	visitor->Visit(this, st.st_ctime);
	visitor->Visit(this, st.st_crtime);
// TODO: Would this be needed as well?
//	visitor->Visit(this, *(uint32*)&st.st_type);
//	visitor->Visit(this, *(int64*)&st.st_blocks);

	// revision
	visitor->Visit(this, revision);
}
