myfs_info *myfs_create_fs(char *device, char *volname,
                          int block_size, char *opts);
int        myfs_mount(nspace_id nsid, const char *device, ulong flags,
                      void *parms, size_t len, void **data, vnode_id *vnid);
int        myfs_unmount(void *ns);
