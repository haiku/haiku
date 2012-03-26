/*
 * Copyright 2001-2008, pinc Software. All Rights Reserved.
 */
#ifndef INODE_H
#define INODE_H


#include <SupportDefs.h>

#include "Disk.h"

class BPlusTree;
class Directory;


class Inode {
	public:
		Inode(Disk* disk, bfs_inode* inode, bool ownBuffer = true);
		Inode(const Inode &inode);
		virtual ~Inode();

		status_t SetTo(bfs_inode *inode);
		virtual status_t InitCheck();

		bool			IsFile() const { return S_ISREG(fInode->mode); }
		bool			IsDirectory() const { return S_ISDIR(fInode->mode); }
		bool			IsSymlink() const { return S_ISLNK(fInode->mode); }
		bool			IsIndex() const { return S_ISINDEX(fInode->mode); }
		bool			IsAttribute() const
							{ return (fInode->mode & S_ATTR) != 0; }
		bool			IsAttributeDirectory() const
							{ return (fInode->mode & S_ATTR_DIR) != 0; }
		bool			IsRoot() const { return BlockRun() == fDisk->Root(); }

		int32			Mode() const { return fInode->mode; }
		int32			Flags() const { return fInode->flags; }
		off_t			Size() const { return fInode->data.size; }

		off_t			Offset() const
							{ return fDisk->ToOffset(BlockRun()); }
		off_t			Block() const { return fDisk->ToBlock(BlockRun()); }
		const block_run& BlockRun() const { return fBlockRun; }
		block_run		Parent() const { return fInode->parent; }
		block_run		Attributes() const { return fInode->attributes; }

		const bfs_inode* InodeBuffer() const { return fInode; }
		status_t		CopyBuffer();

		void			ReleaseBuffer();
		status_t		AcquireBuffer();
		void			BufferClobbered();

		void			SetParent(const block_run& run);
		void			SetBlockRun(const block_run& run);
		void			SetMode(uint32 mode);

		status_t		SetName(const char* name);
		const char*		Name() const;
		status_t		GetNextSmallData(small_data** smallData);

		status_t		RewindAttributes();
		status_t		GetNextAttribute(char* name, uint32* type, void** data,
							size_t* length);

		class Source;
		const char*		Path(Inode::Source* source = NULL);
		virtual status_t CopyTo(const char* path, bool fullPath = true,
							Inode::Source* source = NULL);
		status_t		CopyAttributesTo(BNode* node);

		static Inode*	Factory(Disk* disk, bfs_inode* inode,
							bool ownBuffer = true);
		static Inode*	Factory(Disk* disk, block_run run);
		static Inode*	Factory(Disk* disk, Inode* inode,
							bool copyBuffer = true);

		static Inode*	EmptyInode(Disk* disk,const char* name, int32 mode);

		class Source {
			public:
				virtual Inode *InodeAt(block_run run) = 0;
		};

	protected:
		static bool	_LowMemory();
		void		_Unset();
		status_t	_FindPath(Inode::Source *source = NULL);

		Disk		*fDisk;
		bfs_inode	*fInode;
		bool		fOwnBuffer;
		char		*fPath;
		block_run	fBlockRun;
		int32		fRefCount;

		small_data	*fCurrentSmallData;
		Directory	*fAttributes;
		void		*fAttributeBuffer;
};


class DataStream : public Inode, public BPositionIO {
	public:
		DataStream(Disk *disk, bfs_inode *inode, bool ownBuffer = true);
		DataStream(const Inode &inode);
		~DataStream();

		status_t			FindBlockRun(off_t pos);

		virtual ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
		virtual ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

		virtual off_t		Seek(off_t position, uint32 seek_mode);
		virtual off_t		Position() const;

		virtual status_t	SetSize(off_t size);
	
	private:
		int32		fCurrent;
		int32		fLevel;
		block_run	fRun;
		off_t		fRunFileOffset;
		off_t		fRunBlockEnd;
		off_t		fPosition;
};


class File : public DataStream {
	public:
		File(Disk *disk, bfs_inode *inode, bool ownBuffer = true);
		File(const Inode &inode);
		~File();

		virtual status_t	InitCheck();
		virtual status_t	CopyTo(const char *path, bool fullPath = true,
								Inode::Source *source = NULL);
};


class Attribute : public File {
	public:
		Attribute(Disk *disk, bfs_inode *inode, bool ownBuffer = true);
		Attribute(const Inode &inode);
		~Attribute();

		virtual status_t	InitCheck();
		virtual status_t	CopyTo(const char *path, bool fullPath = true,
								Inode::Source *source = NULL);

		uint32 Type() const { return fInode->type; }
};


class Directory : public DataStream {
	public:
		Directory(Disk *disk, bfs_inode *inode, bool ownBuffer = true);
		Directory(const Inode &inode);
		~Directory();
		
		virtual status_t	InitCheck();
		virtual status_t	CopyTo(const char *path, bool fullPath = true,
								Inode::Source *source = NULL);

		virtual status_t	Rewind();
		virtual status_t	GetNextEntry(char *name, block_run *run);
		virtual status_t	GetNextEntry(block_run *run);

		virtual status_t	Contains(const block_run *run);
		virtual status_t	Contains(const Inode *inode);
		virtual status_t	FindEntry(const char *name, block_run *run);

		virtual status_t	AddEntry(Inode *inode);

		status_t			GetTree(BPlusTree **tree);

	private:
		virtual status_t	CreateTree();

		BPlusTree	*fTree;
};


class Symlink : public Inode {
	public:
		Symlink(Disk *disk, bfs_inode *inode, bool ownBuffer = true);
		Symlink(const Inode &inode);
		~Symlink();

		virtual status_t	InitCheck();
		virtual status_t	CopyTo(const char *path, bool fullPath = true,
								Inode::Source *source = NULL);

		status_t			LinksTo(char *to, size_t maxLength);

	private:
		char	*fTo;
};

#endif	/* INODE_H */
