// This manager holds all of the vpages.
// It creates them from the vpage pool
// and ensures that no two vpages point to
// the same vnode without also pointing to 
// the same physical memory. Thus, cached disk
// blocks and mmapped memory always stay in sync. 
// mmap seems to not want this to happen for a 
// couple of cases - MAP_PRIVATE and MAP_COPY.
// If we decided to ever build these, we would
// have to make some changes.

class vpageManager
{
	private:

	public:
		vpage *getVpage(vnode &vn);
		void putVpage(vpage &vp);
}
